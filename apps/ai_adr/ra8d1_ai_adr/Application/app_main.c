#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <LoRabbit.h>
#include <LoRabbit_ai_adr.h>
#include "tglib.h"
#include "r_sci_b_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// サーバーのアドレスとチャンネル
#define SERVER_ADDRESS 0x2000
#define SERVER_CHANNEL 0x02

// --- アプリケーション層のパケット種別定義 ---
#define APP_CMD_TEST_DATA       0x30 // データ収集用のテストデータ
#define APP_CMD_AI_CONFIG_REQ   0x40 // Client→Server: AIによる設定変更要求
#define APP_CMD_AI_CONFIG_ACK   0x41 // Server→Client: 設定変更要求へのACK

static ID s_button_flg_id;
#define FLAG_CHANGE_SETTING (1 << 0) // 設定変更ボタン(SW1)
#define FLAG_SEND_DATA      (1 << 1) // データ送信ボタン(SW2)

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
    T_RFLG rflg;
    ER err;

    // 現在のイベントフラグの状態を参照
    err = tk_ref_flg(s_button_flg_id, &rflg);

    if (err >= E_OK) {
        // 対象のフラグビットがまだセットされていないかチェック
        if ((rflg.flgptn & FLAG_SEND_DATA) == 0) {
            // フラグがクリアされている場合のみ、セットする
            tk_set_flg(s_button_flg_id, FLAG_SEND_DATA);
        }
        // 既にフラグがセットされている場合は、チャタリングとみなし何もしない
    }
}

void button_s1_irq_callback(external_irq_callback_args_t *p_args) {
    T_RFLG rflg;
    ER err;

    // 現在のイベントフラグの状態を参照
    err = tk_ref_flg(s_button_flg_id, &rflg);

    if (err >= E_OK) {
        // 対象のフラグビットがまだセットされていないかチェック
        if ((rflg.flgptn & FLAG_CHANGE_SETTING) == 0) {
            // フラグがクリアされている場合のみ、セットする
            tk_set_flg(s_button_flg_id, FLAG_CHANGE_SETTING);
        }
        // 既にフラグがセットされている場合は、チャタリングとみなし何もしない
    }
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

    LoRabbit_SwitchToNormalMode(&s_lora_handle);
    LOG("AI-ADR Client Task Started.\n");
    LOG("Press S1 Button for AI recommendation, S2 Button to send test data.\n");

    for (;;) {
        UINT flag_pattern = 0;
        tk_wai_flg(s_button_flg_id, FLAG_CHANGE_SETTING | FLAG_SEND_DATA, TWF_ORW | TWF_BITCLR, &flag_pattern, TMO_FEVR);

        if (flag_pattern & FLAG_CHANGE_SETTING) {
            // --- AIによる設定変更フロー (SW1) ---
            LOG("--- Starting AI Recommendation Flow ---\n");
            LoRaRecommendedConfig_t recommend;
            ER err = LoRabbit_Get_AI_Recommendation(&s_lora_handle, &recommend);

            if (err == LORABBIT_OK) {
                LOG("AI recommends: SF%d, %s\n",
                    get_spreading_factor_from_air_data_rate(recommend.air_data_rate),
                    (recommend.transmitting_power == LORA_TRANSMITTING_POWER_13_DBM ? "13dBm" : "7dBm"));

                // 設定変更要求パケットを組み立てる
                uint8_t req_payload[1 + sizeof(LoRaRecommendedConfig_t)];
                req_payload[0] = APP_CMD_AI_CONFIG_REQ;
                memcpy(&req_payload[1], &recommend, sizeof(LoRaRecommendedConfig_t));

                // サーバーにコマンドをSendDataで送信（ACK付き）
                err = LoRabbit_SendData(&s_lora_handle, SERVER_ADDRESS, SERVER_CHANNEL, req_payload, sizeof(req_payload), true);

                if (err == LORABBIT_OK) {
                    LOG("Server acknowledged config change. Updating own config...\n");
                    // サーバーがACKを返したら、自分も設定変更
                    LoraConfigItem_t new_config = s_lora_handle.current_config;
                    new_config.air_data_rate = recommend.air_data_rate;
                    new_config.transmitting_power = recommend.transmitting_power;
                    LoRabbit_InitModule(&s_lora_handle, &new_config);
                    LoRabbit_SwitchToNormalMode(&s_lora_handle);
                    LOG("Client config updated successfully.\n");
                } else {
                    LOG("Failed to send config change to server (err: %d).\n", err);
                }
            } else {
                LOG("Failed to get AI recommendation (err: %d). Not enough history?\n", err);
            }
        }

        if (flag_pattern & FLAG_SEND_DATA) {
            // テストデータ送信フロー (S2)
            LOG("--- Sending Test Data for current settings ---\n");
            uint8_t test_payload[180];
            test_payload[0] = APP_CMD_TEST_DATA; // 先頭にコマンドバイト
            uint32_t payload_size = 20 + (rand() % 160);

            ER err = LoRabbit_SendData(&s_lora_handle, SERVER_ADDRESS, SERVER_CHANNEL, test_payload, payload_size + 1, true);

            if (err == LORABBIT_OK) {
                LOG("SendData OK.\n");
                LoRabbit_DumpHistory(&s_lora_handle); // 成功したら履歴を表示
            } else {
                LOG("SendData FAIL (err: %d)\n", err);
            }
        }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
	LOG("Start User-main program.\n");
    LOG("ra8d1_ai_adr\n");

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
