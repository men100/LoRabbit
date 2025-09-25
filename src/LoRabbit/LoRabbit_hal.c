#include "LoRabbit.h"
#include "LoRabbit_hal.h"
#include "LoRabbit_config.h"
#include "LoRabbit_internal.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>

// Configuration Mode 時の Baudrate
#define LORA_CONFIGURATION_MODE_UART_BPS 9600

void LoRabbit_UartCallbackHandler(LoraHandle_t * p_handle, uart_callback_args_t * p_args) {
    if (UART_EVENT_RX_CHAR == p_args->event) {
        // リングバッファをハンドルから取得する
        uint16_t next_head = (p_handle->rx_head + 1) % LORA_RX_BUFFER_SIZE;
        if (next_head != p_handle->rx_tail) {
            p_handle->rx_buffer[p_handle->rx_head] = (uint8_t)p_args->data;
            p_handle->rx_head = next_head;
        }
    }
}

#ifdef LORABBIT_USE_AUX_IRQ
void LoRabbit_AuxCallbackHandler(LoraHandle_t *p_handle, external_irq_callback_args_t *p_args) {
    bsp_io_level_t pin_level;

    // 現在のピンレベルを読み取る
    R_IOPORT_PinRead(&g_ioport_ctrl, p_handle->hw_config.aux, &pin_level);
    switch (p_handle->state) {
        case LORA_STATE_WAITING_TX:
            // 送信完了待ちの状態で、ピンがHighになった (立ち上がり)
            if (BSP_IO_LEVEL_HIGH == pin_level) {
                p_handle->state = LORA_STATE_IDLE;
                tk_sig_sem(p_handle->tx_done_sem_id, 1);
            }
            break;

        case LORA_STATE_WAITING_RX:
            // 受信開始待ちの状態で、ピンがLowになった (立ち下がり)
            if (BSP_IO_LEVEL_LOW == pin_level) {
                p_handle->state = LORA_STATE_IDLE;
                tk_sig_sem(p_handle->rx_start_sem_id, 1);
            }
            break;

        default:
            // アイドル時など、予期しない割り込みは無視
            break;
    }
}
#endif

// air_data_rate から Spreading Factor を返す
int get_spreading_factor_from_air_data_rate(LoraAirDateRate_t air_data_rate) {
    switch (air_data_rate) {
        case LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125:
        case LORA_AIR_DATA_RATE_31250_BPS_SF_5_BW_250:
        case LORA_AIR_DATA_RATE_62500_BPS_SF_5_BW_500:
            return 5;

        case LORA_AIR_DATA_RATE_9375_BPS_SF_6_BW_125:
        case LORA_AIR_DATA_RATE_18750_BPS_SF_6_BW_250:
        case LORA_AIR_DATA_RATE_37500_BPS_SF_6_BW_500:
            return 6;

        case LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125:
        case LORA_AIR_DATA_RATE_10938_BPS_SF_7_BW_250:
        case LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500:
            return 7;

        case LORA_AIR_DATA_RATE_3125_BPS_SF_8_BW_125:
        case LORA_AIR_DATA_RATE_6250_BPS_SF_8_BW_250:
        case LORA_AIR_DATA_RATE_12500_BPS_SF_8_BW_500:
            return 8;

        case LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125:
        case LORA_AIR_DATA_RATE_3516_BPS_SF_9_BW_250:
        case LORA_AIR_DATA_RATE_7031_BPS_SF_9_BW_500:
            return 9;

        case LORA_AIR_DATA_RATE_1953_BPS_SF_10_BW_250:
        case LORA_AIR_DATA_RATE_3906_BPS_SF_10_BW_500:
            return 10;

        case LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500:
            return 11;

        default:
            return 11;
    }
}

// air_data_rate から Bandwidth kHz を返す
double get_bandwidth_khz_from_air_data_rate(LoraAirDateRate_t air_data_rate) {
    switch (air_data_rate) {
        case LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125:
        case LORA_AIR_DATA_RATE_9375_BPS_SF_6_BW_125:
        case LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125:
        case LORA_AIR_DATA_RATE_3125_BPS_SF_8_BW_125:
        case LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125:
            return 125.0;

        case LORA_AIR_DATA_RATE_31250_BPS_SF_5_BW_250:
        case LORA_AIR_DATA_RATE_18750_BPS_SF_6_BW_250:
        case LORA_AIR_DATA_RATE_10938_BPS_SF_7_BW_250:
        case LORA_AIR_DATA_RATE_6250_BPS_SF_8_BW_250:
        case LORA_AIR_DATA_RATE_3516_BPS_SF_9_BW_250:
        case LORA_AIR_DATA_RATE_1953_BPS_SF_10_BW_250:
            return 250.0;

        case LORA_AIR_DATA_RATE_62500_BPS_SF_5_BW_500:
        case LORA_AIR_DATA_RATE_37500_BPS_SF_6_BW_500:
        case LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500:
        case LORA_AIR_DATA_RATE_12500_BPS_SF_8_BW_500:
        case LORA_AIR_DATA_RATE_7031_BPS_SF_9_BW_500:
        case LORA_AIR_DATA_RATE_3906_BPS_SF_10_BW_500:
        case LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500:
            return 500.0;

        default:
            return 125.0;
    }
}

// LoraUartBaudRate_t (enum) を FSPのボーレート値 (uint32_t) に変換する
uint32_t lora_enum_to_fsp_baud(LoraUartBaudRate_t rate_enum) {
    switch (rate_enum) {
        case LORA_UART_BAUD_RATE_1200_BPS: return 1200;
        case LORA_UART_BAUD_RATE_2400_BPS: return 2400;
        case LORA_UART_BAUD_RATE_4800_BPS: return 4800;
        case LORA_UART_BAUD_RATE_9600_BPS: return 9600;
        case LORA_UART_BAUD_RATE_19200_BPS: return 19200;
        case LORA_UART_BAUD_RATE_38400_BPS: return 38400;
        case LORA_UART_BAUD_RATE_57600_BPS: return 57600;
        case LORA_UART_BAUD_RATE_115200_BPS: return 115200;
        default: return 9600;
    }
}

static int lora_available(LoraHandle_t *p_handle) {
    return (p_handle->rx_head - p_handle->rx_tail + LORA_RX_BUFFER_SIZE) % LORA_RX_BUFFER_SIZE;
}

static int lora_read(LoraHandle_t *p_handle) {
    if (p_handle->rx_head == p_handle->rx_tail) {
        return LORABBIT_ERROR_UART_READ_FAILED;
    }

    uint8_t data = p_handle->rx_buffer[p_handle->rx_tail];
    p_handle->rx_tail = (p_handle->rx_tail + 1) % LORA_RX_BUFFER_SIZE;
    return data;
}

static int lora_wait_for_tx_done(const LoraHandle_t *p_handle, int payload_size) {
#ifdef LORABBIT_USE_AUX_IRQ
    if (LORA_PIN_UNDEFINED == p_handle->hw_config.aux) {
        int time = LoRabbit_GetTimeOnAirMsec(p_handle->current_config.air_data_rate, payload_size);
        tk_dly_tsk(time + 10); // 計算結果に少しマージンを追加して待機
        return LORABBIT_OK;
    }

    // タイムアウトを6秒に設定してセマフォを待つ
    ER err = tk_wai_sem(p_handle->tx_done_sem_id, 1, 6000);

    return err; // LORABBIT_OK:成功, LORABBIT_ERROR_TIMEOUT:タイムアウト
#else
    int time = get_time_on_air_msec(p_config->air_data_rate, p_config->payload_size);
    R_BSP_SoftwareDelay(time, BSP_DELAY_UNITS_MILLISECONDS);
    return LORABBIT_OK;
#endif
}

// 新しい設定を書き込む
static ER lora_write_config(LoraHandle_t *p_handle, LoraConfigItem_t *p_config) {
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    uint8_t command[11] = {0xC0, 0x00, 0x08}; // ヘッダ(3) + パラメータ(8)
    uint8_t response[11] = {0};
    uint8_t response_len = 0;

    // command配列の組み立て
    command[3] = p_config->own_address >> 8;
    command[4] = p_config->own_address & 0xff;
    command[5] = (p_config->baud_rate << 5) | (p_config->air_data_rate);
    command[6] = (p_config->payload_size << 6) | (p_config->rssi_ambient_noise_flag << 5) |
                 (p_config->transmitting_power);
    command[7] = p_config->own_channel;
    command[8] = (p_config->rssi_byte_flag << 7) | (p_config->transmission_method_type << 6) |
                 (p_config->wor_cycle);
    command[9] = p_config->encryption_key >> 8;
    command[10] = p_config->encryption_key & 0xff;

    LORA_PRINTF("# Command Request\n");
    for (size_t i = 0; i < sizeof(command); i++) {
        LORA_PRINTF("0x%02x ", command[i]);
    }
    LORA_PRINTF("\n");

    p_uart->p_api->write(p_uart->p_ctrl, command, sizeof(command));
    tk_dly_tsk(100);

    while (lora_available(p_handle) && response_len < sizeof(response)) {
        response[response_len++] = lora_read(p_handle);
    }

    LORA_PRINTF("# Command Response\n");
    for (size_t i = 0; i < response_len; i++) {
        LORA_PRINTF("0x%02x ", response[i]);
    }
    LORA_PRINTF("\n");

    // TODO: 詳細チェックを実施する
    if (response_len != sizeof(command) || response[0] != 0xC1) {
        return E_TMOUT; // 書き込み失敗
    }

    return LORABBIT_OK; // 書き込み成功
}

static int lora_set_mcu_baud_rate(LoraHandle_t *p_handle, uint32_t new_baud_rate) {
    if (NULL != p_handle->hw_config.pf_baud_set_helper) {
        return p_handle->hw_config.pf_baud_set_helper(p_handle, new_baud_rate);
    } else {
        return LORABBIT_OK;
    }
}

// =====================================

int LoRabbit_Init(LoraHandle_t * p_handle, LoraHwConfig_t const * p_hw_config) {
    if (NULL == p_handle || NULL == p_hw_config) {
        return LORABBIT_ERROR_INVALID_ARGUMENT;
    }

    // ハンドルにハードウェア構成をコピー
    memcpy(&p_handle->hw_config, p_hw_config, sizeof(LoraHwConfig_t));

    // リングバッファを初期化
    p_handle->rx_head = 0;
    p_handle->rx_tail = 0;

#ifdef LORABBIT_USE_AUX_IRQ
    // AUXピンが設定されている場合のみセマフォを生成
    if (LORA_PIN_UNDEFINED != p_handle->hw_config.aux) {
        // セマフォ生成パケットを定義
        T_CSEM csem;
        csem.exinf   = 0;                   // 拡張情報 (未使用)
        csem.sematr  = TA_TFIFO | TA_FIRST; // FIFO順の待機キュー
        csem.isemcnt = 0;                   // 初期セマフォカウント
        csem.maxsem  = 1;                   // 最大セマフォカウント (バイナリセマフォ)

        // 送信完了用セマフォを生成
        p_handle->tx_done_sem_id = tk_cre_sem(&csem);
        if (p_handle->tx_done_sem_id < LORABBIT_OK) {
            // エラー処理 (IDが負の値で返る)
            LORA_PRINTF("LoRa_Init: tk_cre_sem failed(%d)\n", p_handle->tx_done_sem_id);
            return p_handle->tx_done_sem_id;
        }

        // 受信開始用セマフォを生成
        p_handle->rx_start_sem_id = tk_cre_sem(&csem);
        if (p_handle->rx_start_sem_id < LORABBIT_OK) {
            // エラー処理 (IDが負の値で返る)
            LORA_PRINTF("LoRa_Init: tk_cre_sem failed(%d)\n", p_handle->rx_start_sem_id);
            return p_handle->rx_start_sem_id;
        }

        // ステートを初期化
        p_handle->state = LORA_STATE_IDLE;
    }
#endif
    T_CSEM csem_mutex;
    csem_mutex.exinf = 0;                    // 拡張情報 (未使用)
    csem_mutex.sematr = TA_TFIFO | TA_FIRST; // FIFO順の待機キュー
    csem_mutex.isemcnt = 1,                  // 初期セマフォカウント
    csem_mutex.maxsem = 1;                   // 最大セマフォカウント (バイナリセマフォ)

    // 転送状態とミューテックスを初期化
    memset((void*)&p_handle->transfer_status, 0, sizeof(LoRabbit_TransferStatus_t));
    p_handle->status_mutex_id = tk_cre_sem(&csem_mutex);
    if (p_handle->status_mutex_id < LORABBIT_OK) {
        LORA_PRINTF("LoRa_Init: tk_cre_sem for status_mutex_id failed(%d)\n", p_handle->status_mutex_id);
        return p_handle->status_mutex_id;
    }

    // エンコーダ用ミューテックス
    p_handle->encoder_mutex_id = tk_cre_sem(&csem_mutex);
    if (p_handle->encoder_mutex_id < LORABBIT_OK) {
        LORA_PRINTF("LoRa_Init: tk_cre_sem for encoder_mutex_id failed(%d)\n", p_handle->rx_start_sem_id);
        return p_handle->encoder_mutex_id;
    }

    // デコーダ用ミューテックス
    p_handle->decoder_mutex_id = tk_cre_sem(&csem_mutex);
    if (p_handle->decoder_mutex_id < LORABBIT_OK) {
        LORA_PRINTF("LoRa_Init: tk_cre_sem for decoder_mutex_id failed(%d)\n", p_handle->rx_start_sem_id);
        return p_handle->decoder_mutex_id;
    }

    return LORABBIT_OK;
}

int LoRabbit_InitModule(LoraHandle_t *p_handle, LoraConfigItem_t *p_config) {
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
        return -1; // Not initialized
    }

    LORA_PRINTF("switch to configuration mode\n");
    LoRabbit_SwitchToConfigurationMode(p_handle);
    tk_dly_tsk(100);

    // 設定を書き込む
    ER err = lora_write_config(p_handle, p_config); // (0xC0コマンドを送信する内部関数)
    if (err != LORABBIT_OK) {
        LORA_PRINTF("LoRa_InitModule: Failed to write config.\n");
        return -1;
    }

    // 成功したら、ハンドルに「現在の設定」として保存
    memcpy(&p_handle->current_config, p_config, sizeof(LoraConfigItem_t));

    LORA_PRINTF("LoRa_InitModule: Configuration updated.\n");
    return 0;
}

#define POST_RECEIVE_TIMEOUT_MS_DEFAULT 5
int LoRabbit_ReceiveFrame(LoraHandle_t *p_handle, RecvFrameE220900T22SJP_t *recv_frame, TMO timeout) {
    int len = 0;
    memset(recv_frame->recv_data, 0x00, sizeof(recv_frame->recv_data));

#ifdef LORABBIT_USE_AUX_IRQ
    // AUXピンが未接続の場合は、このイベント駆動の受信はできない
    if (LORA_PIN_UNDEFINED == p_handle->hw_config.aux) {
        LORA_PRINTF((UB*)"# ERROR: LoRa_ReceiveFrame in IRQ mode requires AUX pin.\n");
        return LORABBIT_ERROR_UNSUPPORTED; // 未サポートエラー
    }

    // 受信開始前にステートを設定
    p_handle->state = LORA_STATE_WAITING_RX;

    // 受信開始(AUX Low)をセマフォで待つ
    ER err = tk_wai_sem(p_handle->rx_start_sem_id, 1, timeout);
    if (err != LORABBIT_OK) {
        p_handle->state = LORA_STATE_IDLE;
        return (err == E_TMOUT) ? 0 : err; // タイムアウトなら受信データなし(0)、それ以外はエラーを返す
    }

    // 受信が開始されたので、UARTバッファからデータを最後まで読み出す
    // (データ受信完了の明確な通知はないため、データ間が一定時間空いたら完了とみなす)
    int post_receive_timeout_ms = POST_RECEIVE_TIMEOUT_MS_DEFAULT; // データ間のタイムアウト
    while (post_receive_timeout_ms > 0) {
        if (lora_available(p_handle)) {
            recv_frame->recv_data[len] = lora_read(p_handle);
            len++;
            if (len >= (int)sizeof(recv_frame->recv_data) - 1) {
                break; // バッファ満杯
            }
            // タイムアウトをリセット
            post_receive_timeout_ms = POST_RECEIVE_TIMEOUT_MS_DEFAULT;
        } else {
            tk_dly_tsk(1); // 1ms待機
            post_receive_timeout_ms--;
        }
    }

    if (len > 0) {
        recv_frame->recv_data_len = len - 1;
        recv_frame->rssi = recv_frame->recv_data[len - 1] - 256;
    }
    return len > 0 ? (int)recv_frame->recv_data_len : 0;
#else
    // 従来のポーリング方式で受信を待つ
    while (1) {
        while (lora_available(p_handle)) {
            recv_frame->recv_data[len] = lora_read(p_handle);
            len++;
            if (len >= (int)sizeof(recv_frame->recv_data) -1) {
                // バッファが満杯になったら強制的に終了
                goto receive_complete;
            }
        }

        if ((lora_available(p_handle) == 0) && (len > 0)) {
            // データが来ていて、かつバッファが空になったら、少し待ってから再度確認
            // それでもデータがなければ受信完了とみなす
            tk_dly_tsk(10);
            if (lora_available(p_handle) == 0) {
                goto receive_complete;
            }
        }

        // 受信データがまだない場合、待つ
        tk_dly_tsk(100);
    }

receive_complete:
    if (len > 0) {
        recv_frame->recv_data_len = len - 1;
        recv_frame->rssi = recv_frame->recv_data[len - 1] - 256;
    }
    return (int)recv_frame->recv_data_len;
#endif
}

int LoRabbit_SendFrame(LoraHandle_t *p_handle, uint16_t target_address, uint8_t target_channel, uint8_t *p_send_data, int size) {
    int err = 0;
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    const LoraConfigItem_t *p_config = &p_handle->current_config;
    if (NULL == p_uart || NULL == p_config) {
        return LORABBIT_ERROR_INVALID_ARGUMENT;
    }

    uint8_t payload_size = 0;
    switch (p_config->payload_size) {
        case LORA_PAYLOAD_SIZE_200_BYTE: payload_size = 200; break;
        case LORA_PAYLOAD_SIZE_128_BYTE: payload_size = 128; break;
        case LORA_PAYLOAD_SIZE_64_BYTE: payload_size = 64; break;
        case LORA_PAYLOAD_SIZE_32_BYTE: payload_size = 32; break;
    }
    if (size > payload_size) {
        LORA_PRINTF("send data length too long\n");
        return LORABBIT_ERROR_INVALID_ARGUMENT;
    }

    uint8_t frame[3 + 197]; // 最大サイズでバッファ確保
    frame[0] = target_address >> 8;
    frame[1] = target_address & 0xff;
    frame[2] = target_channel;
    memcpy(frame + 3, p_send_data, size);
    int frame_size = 3 + size;

#if 0 // print debug
      for (int i = 0; i < 3 + size; i++) {
        if (i < 3) {
            LORA_PRINTF("%02x", frame[i]);
        } else {
            LORA_PRINTF("%c", frame[i]);
        }
      }
      LORA_PRINTF("\n");
#endif

#ifdef LORABBIT_USE_AUX_IRQ
    //  送信前にステートを設定 ★★★
    if (LORA_PIN_UNDEFINED != p_handle->hw_config.aux) {
        p_handle->state = LORA_STATE_WAITING_TX;
    }
#endif

    p_uart->p_api->write(p_uart->p_ctrl, frame, frame_size);
    err = lora_wait_for_tx_done(p_handle, frame_size);
    if (err < 0) {
        LORA_PRINTF("LoRa_SendFrame: lora_wait_for_tx_done timeout\n");
    }
    
    // 送信後にモジュールから応答データが返る場合があるため、バッファをクリア
    while (lora_available(p_handle)) {
        lora_read(p_handle);
    }

    return LORABBIT_OK;
}

int LoRabbit_GetTimeOnAirMsec(LoraAirDateRate_t air_data_rate, uint8_t payload_size_bytes)
{
    // air_data_rate から SF と BW を抽出
    int spreading_factor = get_spreading_factor_from_air_data_rate(air_data_rate);
    double bandwidth_khz = get_bandwidth_khz_from_air_data_rate(air_data_rate);

    // LoRaの物理パラメータを定義
    const int preamble_len = 8;    // プリアンブル長
    const int coding_rate  = 1;    // コーディングレート (4/5)
    const bool has_crc     = true; // CRCは有効
    const bool explicit_header = true; // ヘッダは有効

    // 1シンボルあたりの時間を計算
    // T_sym = (2^SF) / BW
    double t_sym = pow(2, spreading_factor) / (bandwidth_khz * 1000.0);

    // プリアンブルの時間を計算
    double t_preamble = (preamble_len + 4.25) * t_sym;

    // ペイロードのシンボル数を計算 (LoRaの仕様書に基づく)
    double de = (spreading_factor >= 11) ? 1.0 : 0.0; // Low data rate optimize
    double h  = explicit_header ? 0.0 : 1.0;

    double term1 = 8.0 * payload_size_bytes - 4.0 * spreading_factor + 28.0 + (has_crc ? 16.0 : 0.0) - 20.0 * h;
    double term2 = 4.0 * (spreading_factor - 2.0 * de);
    if (term2 == 0) {
        term2 = 1; // ゼロ除算を避ける
    }

    double payload_symbol_count_part1 = 8.0 + fmax(0.0, ceil(term1 / term2) * (coding_rate + 4.0));

    // ペイロードの時間を計算
    double t_payload = payload_symbol_count_part1 * t_sym;

    // 合計時間を計算し、ミリ秒に変換して返す
    double total_time_sec = t_preamble + t_payload;
    return (int)ceil(total_time_sec * 1000.0);
}

void LoRabbit_SwitchToNormalMode(LoraHandle_t *p_handle) {
    uint32_t fsp_baud = lora_enum_to_fsp_baud(p_handle->current_config.baud_rate);
    lora_set_mcu_baud_rate(p_handle, fsp_baud);

    // (M0, M1) = (LOW, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(100);
}

void LoRabbit_SwitchToWORSendingMode(LoraHandle_t *p_handle) {
    uint32_t fsp_baud = lora_enum_to_fsp_baud(p_handle->current_config.baud_rate);
    lora_set_mcu_baud_rate(p_handle, fsp_baud);

    // (M0, M1) = (HIGH, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(100);
}

void LoRabbit_SwitchToWORReceivingMode(LoraHandle_t *p_handle) {
    uint32_t fsp_baud = lora_enum_to_fsp_baud(p_handle->current_config.baud_rate);
    lora_set_mcu_baud_rate(p_handle, fsp_baud);

    // (M0, M1) = (LOW, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(100);
}

void LoRabbit_SwitchToConfigurationMode(LoraHandle_t *p_handle) {
    // 設定モードは常に9600bps
    lora_set_mcu_baud_rate(p_handle, LORA_CONFIGURATION_MODE_UART_BPS);

    // (M0, M1) = (HIGH, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(100);
}
