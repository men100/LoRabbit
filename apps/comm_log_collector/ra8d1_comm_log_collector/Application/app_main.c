#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <LoRabbit.h>
#include "tglib.h"
#include "r_sci_b_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// サーバーのアドレスとチャンネル
#define SERVER_ADDRESS 0x2000
#define SERVER_CHANNEL 0x02

static ID s_button_flg_id;
#define FLAG_CHANGE_SETTING (1 << 0) // 設定変更ボタン(SW1)
#define FLAG_SEND_DATA      (1 << 1) // データ送信ボタン(SW2)

// テストするデータレートのリスト（代表的なものに絞る）
static const LoraAirDateRate_t ADR_TEST_RATES[] = {
    LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500, // 高速・近距離向け
    LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125,  // 中速・中距離 （LoRaWANで一般的）
    LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125,  // デフォルト設定
    LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500, // 低速・長距離向け
};
#define NUM_ADR_TEST_RATES (sizeof(ADR_TEST_RATES) / sizeof(ADR_TEST_RATES[0]))

// 1設定あたりに送信するテストパケット数
#define NUM_PACKETS_PER_SETTING 32

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart2;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

void g_uart2_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_UartCallbackHandler(&s_lora_handle, p_args);
}

void g_irq1_callback(external_irq_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_AuxCallbackHandler(&s_lora_handle, p_args);
}

void button_irq_callback(external_irq_callback_args_t *p_args) {
    tk_set_flg(s_button_flg_id, FLAG_SEND_DATA);
}

void button_s1_irq_callback(external_irq_callback_args_t *p_args) {
    tk_set_flg(s_button_flg_id, FLAG_CHANGE_SETTING);
}

int my_sci_b_uart_baud_set_helper(LoraHandle_t *p_handle, uint32_t baudrate) {
    fsp_err_t err = FSP_SUCCESS;
    uart_instance_t const *p_uart = p_handle->hw_config.p_uart;

    // ドライバ固有のBaudCalculate関数を呼び出す
    sci_b_baud_setting_t baud_setting;
    memset(&baud_setting, 0x0, sizeof(sci_b_baud_setting_t));
    err = R_SCI_B_UART_BaudCalculate(baudrate,
                                     false, // Bitrate Modulation 無効
                                     5000,  // 許容エラー率 (5%)
                                     &baud_setting);
    if (FSP_SUCCESS != err) {
        LOG("R_SCI_B_UART_BaudCalculate failed\n");
        // 計算失敗
        return -1;
    }

    // mddr (上位8bit) が 0x80 になっているが、Bitrate Modulation (brme) が 0 になっているので無視される
    LOG("baudrate=%dbps, baud_setting.baudrate_bits: 0x%08x\n",
            baudrate, baud_setting.baudrate_bits);

    // 計算結果を void* にキャストして、抽象APIである baudSet に渡す
    err = p_uart->p_api->baudSet(p_uart->p_ctrl, (void*)&baud_setting);

    return (FSP_SUCCESS == err) ? 0 : -1;
}

// LoRa設定値
LoraConfigItem_t s_config = {
    .own_address              = 0x1000,
    .baud_rate                = LORA_UART_BAUD_RATE_115200_BPS,
    .air_data_rate            = LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125,
    .payload_size             = LORA_PAYLOAD_SIZE_200_BYTE,
    .rssi_ambient_noise_flag  = LORA_FLAG_ENABLED,
    .transmitting_power       = LORA_TRANSMITTING_POWER_13_DBM,
    .own_channel              = 0x01,
    .rssi_byte_flag           = LORA_FLAG_ENABLED,
    .transmission_method_type = LORA_TRANSMISSION_METHOD_TYPE_FIXED,
    .wor_cycle                = LORA_WOR_CYCLE_2000_MS,
    .encryption_key           = 0x0000,
};

LOCAL void lora_client_task(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_lora_client_task;    // Task ID number
LOCAL T_CTSK ctsk_lora_client_task = { // Task creation information
    .itskpri    = 10,
    .stksz      = 4096,
    .task       = lora_client_task,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void lora_client_task(INT stacd, void *exinf) {
    // イベントフラグを生成
    T_CFLG cflg = { .exinf = 0, .flgatr = TA_WMUL | TA_TFIFO, .iflgptn = 0 };
    s_button_flg_id = tk_cre_flg(&cflg);

    int current_setting_index = 0;
    LoRabbit_SwitchToNormalMode(&s_lora_handle);
    LOG("ADR Test Client Task Started.\n");
    LOG("Press SW1 to cycle settings, SW2 to send test data.\n");

    for (;;) {
        UINT flag_pattern = 0;
        // いずれかのボタンが押されるまで無限に待つ
        tk_wai_flg(s_button_flg_id, FLAG_CHANGE_SETTING | FLAG_SEND_DATA, TWF_ORW | TWF_BITCLR, &flag_pattern, TMO_FEVR);

        // どのボタンが押されたか判断
        if (flag_pattern & FLAG_CHANGE_SETTING) {
            // 設定変更ボタン (SW1)
            current_setting_index = (current_setting_index + 1) % NUM_ADR_TEST_RATES;

            LoraConfigItem_t new_config = s_lora_handle.current_config;
            new_config.air_data_rate = ADR_TEST_RATES[current_setting_index];

            // 自分自身のLoRa設定を変更
            LoRabbit_InitModule(&s_lora_handle, &new_config);
            LoRabbit_SwitchToNormalMode(&s_lora_handle);

            int sf = get_spreading_factor_from_air_data_rate(new_config.air_data_rate);
            int bw = get_bandwidth_khz_from_air_data_rate(new_config.air_data_rate);
            LOG("====== Setting Changed: Client is now set to SF=%d, BW=%d.\n", sf, bw);
        }

        if (flag_pattern & FLAG_SEND_DATA) {
            LoRabbit_ClearHistory(&s_lora_handle);

            // データ送信ボタン (SW2)
            LOG("Sending Test Data for current settings\n");
            for (int i = 0; i < NUM_PACKETS_PER_SETTING; i++) {
                uint8_t test_payload[180];
                uint32_t payload_size = 20 + (rand() % 160); // 20〜179バイトのランダムサイズ

                LOG("Sending %lu bytes... ", payload_size);
                ER err = LoRabbit_SendData(&s_lora_handle, SERVER_ADDRESS, SERVER_CHANNEL, test_payload, payload_size, true);

                if (err == LORABBIT_OK) {
                    LOG("OK.\n");
                } else {
                    LOG("FAIL (err: %d)\n", err);
                }
                tk_dly_tsk(1000); // 次の送信まで待機
            }
            LOG("Send Cycle Finished. Dumping History: ---\n");
            LoRabbit_ExportHistoryCSV(&s_lora_handle);
        }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
	LOG("Start User-main program.\n");
    LOG("ra8d1_comm_log_collector\n");

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart2,      // FSPで生成されたUARTインスタンス
        .m0     = PMOD2_9_GPIO,  // FSPで定義したM0ピン
        .m1     = PMOD2_10_GPIO, // FSPで定義したM1ピン
        .aux    = PMOD2_7_INT,   // FSPで定義したAUXピン

        // baudrate 変更の際のヘルパ関数を登録
        .pf_baud_set_helper = my_sci_b_uart_baud_set_helper,
    };

    // LoRaライブラリを初期化
    LoRabbit_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRabbit_InitModule(&s_lora_handle, &s_config) != 0) {
        LOG("LoRa Init Failed!\n");
        R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED3_RED, BSP_IO_LEVEL_HIGH);
        while(1);
    }
    LOG("LoRa Init Success!\n");

    // Create & Start Tasks
    tskid_lora_client_task = tk_cre_tsk(&ctsk_lora_client_task);
    tk_sta_tsk(tskid_lora_client_task, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
