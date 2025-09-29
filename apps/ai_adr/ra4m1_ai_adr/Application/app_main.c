#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <LoRabbit.h>
#include "r_sci_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// --- アプリケーション層のパケット種別定義 ---
#define APP_CMD_TEST_DATA       0x30 // データ収集用のテストデータ
#define APP_CMD_AI_CONFIG_REQ   0x40 // Client→Server: AIによる設定変更要求
#define APP_CMD_AI_CONFIG_ACK   0x41 // Server→Client: 設定変更要求へのACK

// 設定変更ボタン用のセマフォ
static ID s_change_setting_sem_id;

// システム起動時、ボタン用の callback が呼ばれる対策
static bool s_is_started_lora_task = false;

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
    LoRabbit_SwitchToNormalMode(&s_lora_handle);
    LOG("AI-ADR Server Task Started. Waiting for data or commands...\n");

    for (;;) {
        uint32_t received_len = 0;
        int err = LoRabbit_ReceiveData(&s_lora_handle,
                                   s_received_data_buffer,
                                   sizeof(s_received_data_buffer),
                                   &received_len,
                                   TMO_FEVR);
        if (err != LORABBIT_OK) {
            LOG("ReceiveData failed with error: %d. Restarting wait.\n", err);
            continue;
        }
        if (received_len == 0) {
            continue;
        }

        // 受信したデータのコマンドバイトを解釈
        uint8_t command_type = s_received_data_buffer[0];
        switch (command_type)
        {
            case APP_CMD_TEST_DATA:
                LOG("Received ADR test data. Size: %lu bytes.\n", received_len);
                // データ収集が目的なので、ACKはライブラリが返しておりここでは何もしない
                break;

            case APP_CMD_AI_CONFIG_REQ:
                // 設定変更コマンドの場合
                if (received_len == 1 + sizeof(LoRaRecommendedConfig_t)) {
                    LOG("Received AI_CONFIG_REQ command.\n");

                    // 遅い設定のときは受信が完了していない時があるため、少し待つ
                    // TODO: LoRabbit_Switch* 系の関数でそこをケアすべき
                    tk_dly_tsk(200); // 200msの待機を追加

                    LoRaRecommendedConfig_t rec;
                    memcpy(&rec, &s_received_data_buffer[1], sizeof(LoRaRecommendedConfig_t));

                    LoraConfigItem_t new_config = s_lora_handle.current_config;
                    new_config.air_data_rate = rec.air_data_rate;
                    new_config.transmitting_power = rec.transmitting_power;

                    // パラメータを更新
                    if (LoRabbit_InitModule(&s_lora_handle, &new_config) == LORABBIT_OK) {
                        LoRabbit_SwitchToNormalMode(&s_lora_handle);
                        LOG("Server config updated successfully by AI recommendation.\n");
                    }
                } else {
                    LOG("Received invalid AI_CONFIG_REQ command length.\n");
                }
                break;

            default:
                LOG("Received data with unknown command type: 0x%02X.\n", command_type);
                break;
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
