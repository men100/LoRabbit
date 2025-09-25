#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <LoRabbit.h>
#include <ArducamCamera.h>
#include "r_sci_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart0;

// Camera インスタンス
ArducamCamera myCamera;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

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
    .stksz      = 1024,
    .task       = task_2,
    .tskatr     = TA_HLNG | TA_RNG3,
};

// 送信用バッファ、作業用バッファ
#define BUFFER_SIZE 512
uint8_t send_buffer[BUFFER_SIZE];
uint8_t work_buffer[BUFFER_SIZE];

void camera_init(void) {
    // SPI CS (Chip Select) ピン
    const int cs = P413_SPI0_CS;

    // カメラライブラリを初期化
    arducamCameraInit(&myCamera, cs);

    if (myCamera.arducamCameraOp->begin(&myCamera) != CAM_ERR_SUCCESS) {
        LOG("ERROR: Camera initialization failed.\n");
        while(1); // 初期化に失敗したら停止
    }
    LOG("Camera initialization successful.\n");

    // 動作確認として Sensor ID を表示
    LOG("Sensor ID: 0x%02X (Expected: 0x81 for 5MP, 0x82 for 3MP (legacy models)\n", myCamera.cameraId);
}

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
    // 初期化
    for (int i = 0; i < BUFFER_SIZE; i++) {
        send_buffer[i] = i;
    }

    // 通常モードに移行して送信開始
    tm_putstring((UB*)"Switching to Normal Mode.\n");
    LoRabbit_SwitchToNormalMode(&s_lora_handle);

    while(1) {
        LOG("task 2\n");

        LOG("Sending\n");
        int result = LoRabbit_SendCompressedData(&s_lora_handle,
                                                 0x00,
                                                 0,
                                                 send_buffer,
                                                 BUFFER_SIZE,
                                                 true,
                                                 work_buffer,
                                                 BUFFER_SIZE);
        if (result == 0) {
            LOG("LoRa_SendCompressedData success\n");
        } else {
            LOG("LoRa_SendCompressedData failed with code: %d\n", result);
            return;
        }
        tk_dly_tsk(5000);
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
    LOG("Start User-main program.\n");

    LOG("LoRa send test\n");

    // カメラライブラリを初期化
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

    /* Create & Start Tasks */
    tskid_1 = tk_cre_tsk(&ctsk_1);
    tk_sta_tsk(tskid_1, 0);

    tskid_2 = tk_cre_tsk(&ctsk_2);
    tk_sta_tsk(tskid_2, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
