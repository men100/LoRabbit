#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <LoRabbit.h>
#include "camera.h"
#include "r_sci_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

#define REQUEST_PACKET_MIN_SIZE 4
#define REQUEST_FLAG_GET_DATA   0x01

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart0;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

// 96x95 から 32x32 に縮小されたカメラデータ (RGB565)
extern uint16_t resized_image[DST_WIDTH * DST_HEIGHT];

void g_uart0_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_UartCallbackHandler(&s_lora_handle, p_args);
}

void g_irq0_callback(external_irq_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_AuxCallbackHandler(&s_lora_handle, p_args);
}

int my_sci_uart_baud_set_helper(LoraHandle_t *p_handle, uint32_t baudrate) {
    fsp_err_t err = FSP_SUCCESS;
    uart_instance_t const *p_uart = p_handle->hw_config.p_uart;

    // ドライバ固有のBaudCalculate関数を呼び出す
    baud_setting_t baud_setting;
    memset(&baud_setting, 0x0, sizeof(baud_setting_t));

    err = R_SCI_UART_BaudCalculate(baudrate,
                                   false, // Bitrate Modulation 無効
                                   5000,  // 許容エラー率 (5%)
                                   &baud_setting);
    if (FSP_SUCCESS != err) {
        LOG("R_SCI_UART_BaudCalculate failed\n");
        // 計算失敗
        return -1;
    }

    // mddr が 0x80 になっているが、Bitrate Modulation (brme) が 0 になっているので無視される
    LOG("baudrate=%dbps, baud_setting.(semr_baudrate_bits, cks, brr, mddr)=(0x%02x, %d, 0x%02x, 0x%02x)\n",
            baudrate, baud_setting.semr_baudrate_bits, baud_setting.cks, baud_setting.brr, baud_setting.mddr);

    // 計算結果を void* にキャストして、抽象APIである baudSet に渡す
    err = p_uart->p_api->baudSet(p_uart->p_ctrl, (void*)&baud_setting);

    return (FSP_SUCCESS == err) ? 0 : -1;
}

// LoRa設定値
LoraConfigItem_t s_config = {
    .own_address              = 0x0000,
    .baud_rate                = LORA_UART_BAUD_RATE_115200_BPS,
    .air_data_rate            = LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125,
    .payload_size             = LORA_PAYLOAD_SIZE_200_BYTE,
    .rssi_ambient_noise_flag  = LORA_FLAG_ENABLED,
    .transmitting_power       = LORA_TRANSMITTING_POWER_13_DBM,
    .own_channel              = 0,
    .rssi_byte_flag           = LORA_FLAG_ENABLED,
    .transmission_method_type = LORA_TRANSMISSION_METHOD_TYPE_FIXED,
    .wor_cycle                = LORA_WOR_CYCLE_2000_MS,
    .encryption_key           = 0x0000,
};

LOCAL void task_1(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_1;            // Task ID number
LOCAL T_CTSK ctsk_1 = {             // Task creation information
    .itskpri    = 10,
    .stksz      = 1024,
    .task       = task_1,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void task_2(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_2;            // Task ID number
LOCAL T_CTSK ctsk_2 = {             // Task creation information
    .itskpri    = 10,
    .stksz      = 2048,
    .task       = task_2,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void task_1(INT stacd, void *exinf)
{
    LoRabbit_TransferStatus_t status;

    while(1) {
        tm_printf((UB*)"task 1\n");
        if (LoRabbit_GetTransferStatus(&s_lora_handle, &status) == LORABBIT_OK) {
            if (status.is_active) {
                tm_printf((UB*)"LoRa Transferring: Packet %d / %d\n",
                          status.current_packet_index + 1,
                          status.total_packets);
            } else {
                tm_printf((UB*)"LoRa Idle.\n");
            }
        }
        tk_dly_tsk(1000);
    }
}

LOCAL void task_2(INT stacd, void *exinf)
{
    // 通常モードに移行して受信開始
    tm_putstring((UB*)"Switching to Normal Mode.\n");
    LoRabbit_SwitchToNormalMode(&s_lora_handle);

    // 受信した要求パケットを格納するフレーム
    RecvFrameE220900T22SJP_t request_frame;

    // サーバーとして無限ループ
    while(1) {
        LOG("Waiting for a data request...\n");

       // クライアントからのデータ要求を待つ
       int recv_len = LoRabbit_ReceiveFrame(&s_lora_handle, &request_frame, TMO_FEVR);

       // パケット長をチェック
       if (recv_len < REQUEST_PACKET_MIN_SIZE) {
           if (recv_len <= 0) {
               LOG("ReceiveFrame error or timeout. Restarting wait.\n");
           } else {
               LOG("Received a packet, but too short. Ignoring.\n");
           }
           continue;
       }

       // 要求パケットから送信元（クライアント）と要求内容を特定
       uint16_t client_address = (uint16_t)(request_frame.recv_data[0] << 8) | request_frame.recv_data[1];
       uint8_t  client_channel = request_frame.recv_data[2];
       uint8_t  request_flag   = request_frame.recv_data[3];

       // 要求フラグを検証
       if (request_flag == REQUEST_FLAG_GET_DATA) {
           LOG("Received data request from client 0x%04X on channel 0x%02X.\n", client_address, client_channel);

           // 撮影を行う
           camera_take_picture();

           LOG("Sending large data payload (%u bytes) to Address:0x%04x, Channel:0x%02x\n",
                   sizeof(resized_image), client_address, client_channel);

           // 大容量データを、パースした送信元に対して送信する
           ER err = LoRabbit_SendData(&s_lora_handle,
                                      client_address, // パースしたアドレスを使用
                                      client_channel, // パースしたチャンネルを使用
                                      (uint8_t*)resized_image,
                                      sizeof(resized_image),
                                      true); // ACKを要求

           if (err == LORABBIT_OK) {
               LOG("Successfully sent large data to 0x%04X.\n", client_address);
           } else {
               LOG("Failed to send large data to 0x%04X. Error code: %d\n", client_address, err);
           }
       }
       else
       {
           LOG("Received packet with unknown request flag (0x%02X). Ignoring.\n", request_flag);
       }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
    LOG("Start User-main program.\n");
    LOG("ra4m1_remote_camer_capture\n");

    // カメラの初期化 (失敗したらここで停止)
    camera_init();

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart0,  // FSPで生成されたUARTインスタンス
        .m0     = P303_GPIO, // FSPで定義したM0ピン
        .m1     = P304_GPIO, // FSPで定義したM1ピン
        .aux    = P105_INT,  // FSPで定義したAUXピン

        // baudrate 変更の際のヘルパ関数を登録
        .pf_baud_set_helper = my_sci_uart_baud_set_helper,
    };

    // LoRaライブラリを初期化
    LoRabbit_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRabbit_InitModule(&s_lora_handle, &s_config) != 0) {
        LOG("LoRa Init Failed!\n");
        while(1);
    }
    LOG("LoRa Init Success!\n");

    // Create & Start Tasks
    tskid_1 = tk_cre_tsk(&ctsk_1);
    tk_sta_tsk(tskid_1, 0);

    tskid_2 = tk_cre_tsk(&ctsk_2);
    tk_sta_tsk(tskid_2, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
