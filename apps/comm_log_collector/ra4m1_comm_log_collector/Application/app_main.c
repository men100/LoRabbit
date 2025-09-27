#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <LoRabbit.h>
#include "r_sci_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// 設定変更ボタン用のセマフォ
static ID s_change_setting_sem_id;

// システム起動時、ボタン用の callback が呼ばれる対策
static bool s_is_started_lora_task = false;

// テストするデータレートのリスト（代表的なものに絞る）
static const LoraAirDateRate_t ADR_TEST_RATES[] = {
    LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500, // 高速・近距離向け
    LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125,  // 中速・中距離 （LoRaWANで一般的）
    LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125,  // デフォルト設定
    LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500, // 低速・長距離向け
};
#define NUM_ADR_TEST_RATES (sizeof(ADR_TEST_RATES) / sizeof(ADR_TEST_RATES[0]))

#define DATA_BUFFER_SIZE 1024
static uint8_t s_received_data_buffer[DATA_BUFFER_SIZE];

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

// ボタン割り込みから呼び出される callback
void g_irq1_callback(external_irq_callback_args_t *p_args) {
    if (s_is_started_lora_task) {
        tk_sig_sem(s_change_setting_sem_id, 1);
    }
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
    .own_address              = 0x2000,
    .baud_rate                = LORA_UART_BAUD_RATE_115200_BPS,
    .air_data_rate            = LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125,
    .payload_size             = LORA_PAYLOAD_SIZE_200_BYTE,
    .rssi_ambient_noise_flag  = LORA_FLAG_ENABLED,
    .transmitting_power       = LORA_TRANSMITTING_POWER_13_DBM,
    .own_channel              = 0x02,
    .rssi_byte_flag           = LORA_FLAG_ENABLED,
    .transmission_method_type = LORA_TRANSMISSION_METHOD_TYPE_FIXED,
    .wor_cycle                = LORA_WOR_CYCLE_2000_MS,
    .encryption_key           = 0x0000,
};

LOCAL void lora_server_task(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_lora_server_task;    // Task ID number
LOCAL T_CTSK ctsk_lora_server_task = { // Task creation information
    .itskpri    = 10,
    .stksz      = 2048,
    .task       = lora_server_task,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void lora_server_task(INT stacd, void *exinf)
{
    T_CSEM csem = { .exinf = 0, .sematr = TA_TFIFO, .isemcnt = 0, .maxsem = 1 };
    s_change_setting_sem_id = tk_cre_sem(&csem);

    int current_setting_index = -1;

    // 通常モードに移行して受信開始
    tm_putstring((UB*)"Switching to Normal Mode.\n");
    LoRabbit_SwitchToNormalMode(&s_lora_handle);

    LOG("ADR Test Server Task Started.\n");
    LOG("Press Button on this board to cycle LoRa settings.\n");

    // これ以降はボタン用 callback で signal できる
    s_is_started_lora_task = true;

    for (;;) {
        // データ受信を試みる
        // 5秒間の時限付きで、クライアントからの大容量データ送信を待つ
        uint32_t received_len = 0;
        ER err = LoRabbit_ReceiveData(&s_lora_handle,
                                   s_received_data_buffer,
                                   sizeof(s_received_data_buffer),
                                   &received_len,
                                   5000); // 5秒タイムアウト
        if (err == LORABBIT_OK) {
            LOG("Successfully received large data from client. Size: %lu bytes\n", received_len);
        } else if (err != LORABBIT_ERROR_TIMEOUT) {
            // タイムアウト以外のエラーは表示
            LOG("Failed to receive large data. Error: %d\n", err);
        }

        // 設定変更ボタンが押されたかチェックする ---
        // TMO_POL を使うことで、セマフォを待たずに状態だけを確認（ノンブロッキング）
        if (tk_wai_sem(s_change_setting_sem_id, 1, TMO_POL) == E_OK) {
             current_setting_index = (current_setting_index + 1) % NUM_ADR_TEST_RATES;

             LoraConfigItem_t new_config = s_lora_handle.current_config;
             new_config.air_data_rate = ADR_TEST_RATES[current_setting_index];

             LOG("Setting Changed by Button\n");
             // 自分自身のLoRa設定を変更
             LoRabbit_InitModule(&s_lora_handle, &new_config);
             LoRabbit_SwitchToNormalMode(&s_lora_handle);

             int sf = get_spreading_factor_from_air_data_rate(new_config.air_data_rate);
             int bw = get_bandwidth_khz_from_air_data_rate(new_config.air_data_rate);
             LOG("====== Setting Changed: Server is now set to SF=%d, BW=%d.\n", sf, bw);
        }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
    LOG("Start User-main program.\n");
    LOG("ra4m1_comm_log_collector\n");

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
    tskid_lora_server_task = tk_cre_tsk(&ctsk_lora_server_task);
    tk_sta_tsk(tskid_lora_server_task, 0);

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
