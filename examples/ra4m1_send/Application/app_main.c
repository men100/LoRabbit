#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <SecureLoRa-TK.h>

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart0;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

void g_uart0_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRa_UartCallbackHandler(&s_lora_handle, p_args);
}

// LoRa設定値
LoRaConfigItem_t s_config = {
    .own_address              = 0x0000,
    .baud_rate                = LORA_UART_BAUD_RATE_9600_BPS,
    .air_data_rate            = LORA_AIR_DATA_RATE_1758_BPS_SF_8_BW_125,
    .payload_size             = LORA_PAYLOAD_SIZE_200_BYTE,
    .rssi_ambient_noise_flag  = LORA_FLAG_ENABLED,
    .transmitting_power       = LORA_TRANSMITTING_POWER_13_DBM,
    .own_channel              = 0,
    .rssi_byte_flag           = LORA_FLAG_ENABLED,
    .transmission_method_type = LORA_TRANSMISSION_METHOD_TYPE_FIXED,
    .wor_cycle                = LORA_WOR_CYCLE_2000_MS,
    .encryption_key           = 0x0000,
    .target_address           = 0x0000,
    .target_channel           = 0
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
        tk_dly_tsk(500);
    }
}

LOCAL void task_2(INT stacd, void *exinf)
{
    // カウンタ
    int counter = 0;

    while(1) {
        tm_printf((UB*)"task 2\n");

        // 通常モードに移行して送信開始
        tm_putstring((UB*)"Switching to Normal Mode.\n");
        LoRa_SwitchToNormalMode(&s_lora_handle);

        uint8_t send_buffer[50];
        int len = snprintf((char*)send_buffer, sizeof(send_buffer), "RMC-RA4M1 Packet #%d", counter++);
        tm_printf((UB*)"Sending: %s\n", send_buffer);
        int result = LoRa_SendFrame(&s_lora_handle, &s_config, send_buffer, len);
        if (result != 0) {
            tm_printf((UB*)"LoRa_SendFrame failed with code: %d\n", result);
            return;
        }
        tk_dly_tsk(5000);
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
    tm_putstring((UB*)"Start User-main program.\n");

    tm_putstring((UB*)"LoRa send test\n");

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart0,  // FSPで生成されたUARTインスタンス
        .m0     = P303_GPIO, // FSPで定義したM0ピン
        .m1     = P304_GPIO  // FSPで定義したM1ピン
    };

    // LoRaライブラリを初期化
    LoRa_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRa_InitModule(&s_lora_handle, &s_config) != 0) {
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
