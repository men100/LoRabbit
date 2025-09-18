#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <LoRabbit.h>
#include "r_sci_uart.h"

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart0;

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
        tm_printf((UB*)"R_SCI_UART_BaudCalculate failed\n");
        // 計算失敗
        return -1;
    }

    // mddr が 0x80 になっているが、Bitrate Modulation (brme) が 0 になっているので無視される
    tm_printf((UB*)"baudrate=%dbps, baud_setting.(semr_baudrate_bits, cks, brr, mddr)=(0x%02x, %d, 0x%02x, 0x%02x)\n",
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

LOCAL void task_1(INT stacd, void *exinf)
{
    while(1) {
        tm_printf((UB*)"task 1\n");
        tk_dly_tsk(1000);
    }
}

LOCAL void task_2(INT stacd, void *exinf)
{
    RecvFrameE220900T22SJP_t recv_frame;

    // 通常モードに移行して受信開始
    tm_putstring((UB*)"Switching to Normal Mode.\n");
    LoRabbit_SwitchToNormalMode(&s_lora_handle);

    while(1) {
        tm_printf((UB*)"task 2\n");

        if (LoRabbit_ReceiveFrame(&s_lora_handle, &recv_frame, TMO_FEVR) > 0) {
            tm_printf((UB*)"Received! Length: %d, RSSI: %d dBm\n", recv_frame.recv_data_len, recv_frame.rssi);
            tm_printf((UB*)"Data: %.*s\n", recv_frame.recv_data_len, recv_frame.recv_data);
        }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
    tm_putstring((UB*)"Start User-main program.\n");

    tm_putstring((UB*)"LoRa recv test\n");

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
        tm_putstring((UB*)"LoRa Init Failed!\n");
        while(1);
    }
    tm_putstring((UB*)"LoRa Init Success!\n");

    /* Create & Start Tasks */
    tskid_1 = tk_cre_tsk(&ctsk_1);
    tk_sta_tsk(tskid_1, 0);

    tskid_2 = tk_cre_tsk(&ctsk_2);
    tk_sta_tsk(tskid_2, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
