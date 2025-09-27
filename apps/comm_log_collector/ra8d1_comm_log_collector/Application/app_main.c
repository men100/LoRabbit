#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <LoRabbit.h>
#include "tglib.h"
#include "r_sci_b_uart.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// サーバーのアドレスとチャンネル
#define SERVER_ADDRESS 0x2000
#define SERVER_CHANNEL 0x02

// 要求プロトコルの定義
#define REQUEST_PACKET_SIZE   4
#define REQUEST_FLAG_GET_DATA 0x01

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart2;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

// ボタン押下をタスクに通知するためのセマフォ
static ID s_button_s2_press_sem_id;

// LCD 描画の更新をリクエストするためのセマフォ
static ID s_lcd_update_sem_id;

// 現在の転送状態
static LoRabbit_TransferStatus_t s_transfer_status;

// 正しく受信できたかどうか
static bool is_receive_success = false;

// サーバーから受信した大容量データを格納するバッファ
#define DATA_BUFFER_SIZE 3072
static uint8_t s_received_data_buffer[DATA_BUFFER_SIZE];

void g_uart2_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_UartCallbackHandler(&s_lora_handle, p_args);
}

void g_irq1_callback(external_irq_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_AuxCallbackHandler(&s_lora_handle, p_args);
}

void button_irq_callback(external_irq_callback_args_t *p_args) {
    tk_sig_sem(s_button_s2_press_sem_id, 1);
}

void button_s1_irq_callback(external_irq_callback_args_t *p_args) {
    LOG("s1 pressed\n");
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

LOCAL void lcd_task(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_lcd;            // Task ID number
LOCAL T_CTSK ctsk_lcd = {             // Task creation information
    .itskpri    = 10,
    .stksz      = 1024,
    .task       = lcd_task,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void task_1(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_1;			// Task ID number
LOCAL T_CTSK ctsk_1 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_1,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_2(INT stacd, void *exinf);  // task execution function
LOCAL ID    tskid_2;            // Task ID number
LOCAL T_CTSK ctsk_2 = {             // Task creation information
    .itskpri    = 10,
    .stksz      = 1024,
    .task       = task_2,
    .tskatr     = TA_HLNG | TA_RNG3,
};

LOCAL void lcd_task(INT stacd, void *exinf) {
    LoRabbit_TransferStatus_t *p_status = &s_transfer_status;
    char buffer[50];

    // LCD 更新要求用セマフォを生成
    T_CSEM csem = { .exinf = 0, .sematr = TA_TFIFO, .isemcnt = 0, .maxsem = 1 };
    s_lcd_update_sem_id = tk_cre_sem(&csem);


    while(1) {
        LOG("===== lcd_task =====\n");

        tglib_clear_scr(TLIBLCD_COLOR_BLACK);

        if (p_status->is_active) {
            sprintf(buffer, "Packets: %d / %d", p_status->current_packet_index, p_status->total_packets);
        } else {
            sprintf(buffer, "LoRa is idle");
        }
        tglib_draw_string_scaled(buffer, 10, 10, TLIBLCD_COLOR_WHITE, 2);

        if (!p_status->is_active && is_receive_success) {
            tglib_draw_buffer_scaled((UH*)s_received_data_buffer, 112, 100, 32, 32, 8);
        }

        tk_wai_sem(s_lcd_update_sem_id, 1, TMO_FEVR);
    }
}

LOCAL void task_1(INT stacd, void *exinf)
{
    LoRabbit_TransferStatus_t *p_status = &s_transfer_status;
    LoRabbit_TransferStatus_t oldStatus;
    memset(&oldStatus, 0x0, sizeof(oldStatus));

    while(1) {
        tm_printf((UB*)"task 1\n");

        // Inverts the LED on the board.
        out_h(PORT_PODR(6), in_h(PORT_PODR(6))^(1<<0));     // BLUE
        out_h(PORT_PODR(4), in_h(PORT_PODR(4))^(1<<14));    // GREEN
        out_h(PORT_PODR(1), in_h(PORT_PODR(1))^(1<<7));     // RED

        if (LoRabbit_GetTransferStatus(&s_lora_handle, p_status) == LORABBIT_OK) {
            if (p_status->is_active) {
                LOG("LoRa Transferring: Packet %d / %d\n",
                          p_status->total_packets == 0 ? 0 : p_status->current_packet_index + 1,
                          p_status->total_packets);
            } else {
                LOG("LoRa Idle.\n");
            }

            if (oldStatus.is_active != p_status->is_active ||
                oldStatus.current_packet_index != p_status->current_packet_index ||
                oldStatus.total_packets != p_status->total_packets) {
                tk_sig_sem(s_lcd_update_sem_id, 1);
            }
            oldStatus = *p_status;
        }
        tk_dly_tsk(1000);
    }
}

LOCAL void task_2(INT stacd, void *exinf)
{
    // ボタン通知用セマフォを生成
    T_CSEM csem = { .exinf = 0, .sematr = TA_TFIFO, .isemcnt = 0, .maxsem = 1 };
    s_button_s2_press_sem_id = tk_cre_sem(&csem);

    // 通常モードに移行して送信開始
    tm_putstring((UB*)"Switching to Normal Mode.\n");
    LoRabbit_SwitchToNormalMode(&s_lora_handle);

    LOG("LoRa Client Task Started.\n");

    while(1) {
        LOG("\n### Press the user button (SW2) to request data from the server. ###\n");

        // ボタンが押されるのをセマフォで待つ
        tk_wai_sem(s_button_s2_press_sem_id, 1, TMO_FEVR);

        LOG("Button pressed! Sending data request to server 0x%04X...\n", SERVER_ADDRESS);

        // サーバーへの要求パケットを組み立てる
        uint8_t request_packet[REQUEST_PACKET_SIZE];
        // 自分のアドレスとチャンネルをペイロードに含める
        request_packet[0] = s_lora_handle.current_config.own_address >> 8;
        request_packet[1] = s_lora_handle.current_config.own_address & 0xFF;
        request_packet[2] = s_lora_handle.current_config.own_channel;
        request_packet[3] = REQUEST_FLAG_GET_DATA;

        // 要求パケットを送信 (短いデータなのでSendFrameを使用)
        ER err = LoRabbit_SendFrame(&s_lora_handle,
                                    SERVER_ADDRESS,
                                    SERVER_CHANNEL,
                                    request_packet,
                                    sizeof(request_packet));
        if (err != LORABBIT_OK) {
            LOG("Failed to send request packet. Error: %d\n", err);
            continue; // エラーなら最初に戻る
        }

        LOG("Request sent. Waiting for large data response...\n");

        // サーバーからの大容量データの応答を受信する
        uint32_t received_len = 0;
        err = LoRabbit_ReceiveData(&s_lora_handle,
                                   s_received_data_buffer,
                                   sizeof(s_received_data_buffer),
                                   &received_len,
                                   15000); // 15秒のタイムアウト

        // 受信結果を処理
        if (err == LORABBIT_OK) {
            LOG("Successfully received large data! Size: %lu bytes\n", received_len);
            is_receive_success = true;
        } else {
            LOG("Failed to receive large data. Error: %d\n", err);
            is_receive_success = false;
        }
    }
}

/* usermain関数 */
EXPORT INT usermain(void)
{
	tm_putstring((UB*)"Start User-main program.\n");

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart2,      // FSPで生成されたUARTインスタンス
        .m0     = PMOD2_9_GPIO,  // FSPで定義したM0ピン
        .m1     = PMOD2_10_GPIO, // FSPで定義したM1ピン
        .aux    = PMOD2_7_INT,   // FSPで定義したAUXピン

        // baudrate 変更の際のヘルパ関数を登録
        .pf_baud_set_helper = my_sci_b_uart_baud_set_helper,
    };

    // tglib を初期化
    tglib_init();
    tglib_onoff_lcd(LCD_ON);

    // LoRaライブラリを初期化
    LoRabbit_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRabbit_InitModule(&s_lora_handle, &s_config) != 0) {
        tm_putstring((UB*)"LoRa Init Failed!\n");
        R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED3_RED, BSP_IO_LEVEL_HIGH);
        while(1);
    }
    tm_putstring((UB*)"LoRa Init Success!\n");

    // Create & Start Tasks
    tskid_lcd = tk_cre_tsk(&ctsk_lcd);
    tk_sta_tsk(tskid_lcd, 0);

	tskid_1 = tk_cre_tsk(&ctsk_1);
	tk_sta_tsk(tskid_1, 0);

    tskid_2 = tk_cre_tsk(&ctsk_2);
    tk_sta_tsk(tskid_2, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
