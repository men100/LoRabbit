#include "LoRabbit.h"
#include "LoRabbit_config.h"
#include "LoRabbit_internal.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>

// Configuration Mode 時の Baudrate
#define LORA_CONFIGURATION_MODE_UART_BPS 9600

// heatshrink encoder & decoder
static heatshrink_encoder s_hse;
static heatshrink_decoder s_hsd;

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

// テクニカルデータシートより引用
int get_time_on_air_msec(LoraAirDateRate_t air_data_rate, LoraPayloadSize_t payload_size) {
    switch (air_data_rate) {
        case LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 54;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 81;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 136;
                default:
                    return 197;
            }
        case LORA_AIR_DATA_RATE_9375_BPS_SF_6_BW_125:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 85;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 126;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 207;
                default:
                    return 300;
            }
        case LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 146;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 213;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 346;
                default:
                    return 489;
            }
        case LORA_AIR_DATA_RATE_3125_BPS_SF_8_BW_125:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 241;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 353;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 568;
                default:
                    return 814;
            }
        case LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 399;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 604;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 972;
                default:
                    return 1382;
            }
        case LORA_AIR_DATA_RATE_31250_BPS_SF_5_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 29;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 42;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 70;
                default:
                    return 100;
            }
        case LORA_AIR_DATA_RATE_18750_BPS_SF_6_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 45;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 66;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 107;
                default:
                    return 152;
            }
        case LORA_AIR_DATA_RATE_10938_BPS_SF_7_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 78;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 112;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 178;
                default:
                    return 250;
            }
        case LORA_AIR_DATA_RATE_6250_BPS_SF_8_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 131;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 187;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 295;
                default:
                    return 418;
            }
        case LORA_AIR_DATA_RATE_3516_BPS_SF_9_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 220;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 323;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 507;
                default:
                    return 712;
            }
        case LORA_AIR_DATA_RATE_1953_BPS_SF_10_BW_250:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 378;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 542;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 870;
                default:
                    return 1239;
            }
        case LORA_AIR_DATA_RATE_62500_BPS_SF_5_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 15;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 22;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 36;
                default:
                    return 51;
            }
        case LORA_AIR_DATA_RATE_37500_BPS_SF_6_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 24;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 34;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 55;
                default:
                    return 78;
            }
        case LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 42;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 59;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 92;
                default:
                    return 128;
            }
        case LORA_AIR_DATA_RATE_12500_BPS_SF_8_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 71;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 99;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 153;
                default:
                    return 214;
            }
        case LORA_AIR_DATA_RATE_7031_BPS_SF_9_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 121;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 172;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 264;
                default:
                    return 366;
            }
        case LORA_AIR_DATA_RATE_3906_BPS_SF_10_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 210;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 292;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 466;
                default:
                    return 640;
            }
        case LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500:
            switch (payload_size) {
                case LORA_PAYLOAD_SIZE_32_BYTE:
                    return 358;
                case LORA_PAYLOAD_SIZE_64_BYTE:
                    return 501;
                case LORA_PAYLOAD_SIZE_128_BYTE:
                    return 788;
                default:
                    return 1116;
            }
        default:
            return 1382;
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
        return -1;
    }

    uint8_t data = p_handle->rx_buffer[p_handle->rx_tail];
    p_handle->rx_tail = (p_handle->rx_tail + 1) % LORA_RX_BUFFER_SIZE;
    return data;
}

static int lora_wait_for_tx_done(const LoraHandle_t *p_handle, const LoraConfigItem_t *p_config) {
#ifdef LORABBIT_USE_AUX_IRQ
    if (LORA_PIN_UNDEFINED == p_handle->hw_config.aux) {
        int time = get_time_on_air_msec(p_config->air_data_rate, p_config->payload_size);
        tk_dly_tsk(time);
        return E_OK;
    }

    // タイムアウトを6秒に設定してセマフォを待つ
    ER err = tk_wai_sem(p_handle->tx_done_sem_id, 1, 6000);

    return err; // E_OK:成功, E_TMOUT:タイムアウト
#else
    int time = get_time_on_air_msec(p_config->air_data_rate, p_config->payload_size);
    R_BSP_SoftwareDelay(time, BSP_DELAY_UNITS_MILLISECONDS);
    return E_OK;
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

    return E_OK; // 書き込み成功
}

static int lora_set_mcu_baud_rate(LoraHandle_t *p_handle, uint32_t new_baud_rate) {
    if (NULL != p_handle->hw_config.pf_baud_set_helper) {
        return p_handle->hw_config.pf_baud_set_helper(p_handle, new_baud_rate);
    } else {
        return 0;
    }
}

// =====================================

int LoRabbit_Init(LoraHandle_t * p_handle, LoraHwConfig_t const * p_hw_config) {
    if (NULL == p_handle || NULL == p_hw_config) {
        return -1;
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
        if (p_handle->tx_done_sem_id < E_OK) {
            // エラー処理 (IDが負の値で返る)
            LORA_PRINTF("LoRa_Init: tk_cre_sem failed\n");
            return -2;
        }

        // 受信開始用セマフォを生成
        p_handle->rx_start_sem_id = tk_cre_sem(&csem);
        if (p_handle->rx_start_sem_id < E_OK) {
            // エラー処理 (IDが負の値で返る)
            LORA_PRINTF("LoRa_Init: tk_cre_sem failed\n");
            return -3;
        }

        // ステートを初期化
        p_handle->state = LORA_STATE_IDLE;
    }
#endif

    return 0;
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
    if (err != E_OK) {
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
        return -1; // 未サポートエラー
    }

    // 受信開始前にステートを設定
    p_handle->state = LORA_STATE_WAITING_RX;

    // 受信開始(AUX Low)をセマフォで待つ
    ER err = tk_wai_sem(p_handle->rx_start_sem_id, 1, timeout);
    if (err != E_OK) {
        p_handle->state = LORA_STATE_IDLE;
        return (err == E_TMOUT) ? 0 : -1; // タイムアウトなら受信データなし(0)、それ以外はエラー(-1)
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
        return -1; // Not initialized
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
        return 1;
    }

    uint8_t frame[3 + 200]; // 最大サイズでバッファ確保
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
    err = lora_wait_for_tx_done(p_handle, p_config);
    if (err < 0) {
        LORA_PRINTF("LoRa_SendFrame: lora_wait_for_tx_done timeout\n");
    }
    
    // 送信後にモジュールから応答データが返る場合があるため、バッファをクリア
    while (lora_available(p_handle)) {
        lora_read(p_handle);
    }

    return 0;
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

// 大容量伝送用の定義
#define LORABBIT_TP_HEADER_SIZE      8
#define LORABBIT_TP_MAX_PAYLOAD      (197 - LORABBIT_TP_HEADER_SIZE) // 189バイト
#define LORABBIT_TP_MAX_TOTAL_SIZE   (255 * LORABBIT_TP_MAX_PAYLOAD) // 48,195バイト

// コントロールバイトのフラグ定義
#define LORABBIT_TP_FLAG_ACK_REQUEST (1 << 7)
#define LORABBIT_TP_FLAG_IS_ACK      (1 << 6)
#define LORABBIT_TP_FLAG_EOT         (1 << 5)

// パケットヘッダ構造体（内部利用）
typedef struct {
    uint16_t source_address;
    uint8_t  source_channel;
    uint8_t  control_byte;
    uint8_t  transaction_id;
    uint8_t  total_packets;
    uint8_t  packet_index;
    uint8_t  payload_length;
} LoRabbitTP_Header_t;

// ヘッダを解析するヘルパー関数（内部利用）
static void lora_parse_header(uint8_t *raw_packet, LoRabbitTP_Header_t *p_header) {
    p_header->source_address = (raw_packet[0] << 8) | raw_packet[1];
    p_header->source_channel = raw_packet[2];
    p_header->control_byte   = raw_packet[3];
    p_header->transaction_id = raw_packet[4];
    p_header->total_packets  = raw_packet[5];
    p_header->packet_index   = raw_packet[6];
    p_header->payload_length = raw_packet[7];
}

// ACKパケットを送信するヘルパー関数（内部利用）
static int lora_send_ack(LoraHandle_t *p_handle, LoRabbitTP_Header_t *p_data_header) {
    uint8_t ack_payload[LORABBIT_TP_HEADER_SIZE];

    // ACKのヘッダを組み立てる
    ack_payload[0] = p_handle->current_config.own_address >> 8;
    ack_payload[1] = p_handle->current_config.own_address & 0xFF;
    ack_payload[2] = p_handle->current_config.own_channel;
    ack_payload[3] = LORABBIT_TP_FLAG_IS_ACK; // コントロールバイト
    ack_payload[4] = p_data_header->transaction_id;
    ack_payload[5] = p_data_header->total_packets;
    ack_payload[6] = p_data_header->packet_index;
    ack_payload[7] = 0; // ペイロード長

    // ACKを送信
    return LoRabbit_SendFrame(p_handle, p_data_header->source_address, p_data_header->source_channel, ack_payload, sizeof(ack_payload));
}

int LoRabbit_SendData(LoraHandle_t *p_handle,
                           uint16_t target_address,
                           uint8_t target_channel,
                           uint8_t *p_data,
                           uint32_t size,
                           bool request_ack)
{
    if (size > LORABBIT_TP_MAX_TOTAL_SIZE) {
        return -1; // サイズ超過
    }

    static uint8_t transaction_id_counter = 0;
    const uint8_t transaction_id = transaction_id_counter++;
    const uint8_t total_packets = (size + LORABBIT_TP_MAX_PAYLOAD - 1) / LORABBIT_TP_MAX_PAYLOAD;

    uint8_t packet_buffer[197];
    uint32_t sent_size = 0;

    for (uint8_t i = 0; i < total_packets; i++) {
        bool ack_received = false;
        for (uint8_t retry = 0; retry < LORABBIT_TP_RETRY_COUNT; retry++) {
            // ヘッダを組み立てる
            packet_buffer[0] = p_handle->current_config.own_address >> 8;
            packet_buffer[1] = p_handle->current_config.own_address & 0xFF;
            packet_buffer[2] = p_handle->current_config.own_channel;

            uint8_t control_byte = 0;
            if (request_ack) control_byte |= LORABBIT_TP_FLAG_ACK_REQUEST;
            if (i == total_packets - 1) control_byte |= LORABBIT_TP_FLAG_EOT;
            packet_buffer[3] = control_byte;

            packet_buffer[4] = transaction_id;
            packet_buffer[5] = total_packets;
            packet_buffer[6] = i;

            uint32_t remaining_size = size - sent_size;
            uint8_t payload_len = (remaining_size > LORABBIT_TP_MAX_PAYLOAD) ? LORABBIT_TP_MAX_PAYLOAD : remaining_size;
            packet_buffer[7] = payload_len;

            // ペイロードをコピー
            memcpy(&packet_buffer[LORABBIT_TP_HEADER_SIZE], &p_data[sent_size], payload_len);

            // 送信
            LoRabbit_SendFrame(p_handle, target_address, target_channel, packet_buffer, LORABBIT_TP_HEADER_SIZE + payload_len);

            if (!request_ack) {
                ack_received = true;
                break; // ACK不要ならリトライしない
            }

            // ACK待機
            RecvFrameE220900T22SJP_t ack_frame;
            int recv_len = LoRabbit_ReceiveFrame(p_handle, &ack_frame, LORABBIT_TP_ACK_TIMEOUT_MS);
            if (recv_len > 0) {
                LoRabbitTP_Header_t ack_header;
                lora_parse_header(ack_frame.recv_data, &ack_header);
                if ((ack_header.control_byte & LORABBIT_TP_FLAG_IS_ACK) &&
                    (ack_header.transaction_id == transaction_id) &&
                    (ack_header.packet_index == i))
                {
                    ack_received = true;
                    break; // 正しいACKを受信
                }
            }
        } // retry loop

        if (!ack_received) {
            return E_TMOUT; // ACKタイムアウト
        }
        sent_size += packet_buffer[7]; // payload_len
    } // main loop

    return 0; // 成功
}

int LoRabbit_ReceiveData(LoraHandle_t *p_handle,
                              uint8_t *p_buffer,
                              uint32_t buffer_size,
                              uint32_t *p_received_size,
                              TMO timeout)
{
    RecvFrameE220900T22SJP_t frame;
    LoRabbitTP_Header_t header;
    uint32_t written_size = 0;

    if(p_received_size) {
        *p_received_size = 0;
    }

    // 最初のパケットを待つ
    int recv_len = LoRabbit_ReceiveFrame(p_handle, &frame, timeout);
    if (recv_len <= 0) {
        return (recv_len == 0) ? E_TMOUT : E_SYS; // タイムアウト or エラー
    }
    lora_parse_header(frame.recv_data, &header);

    // 最初のパケットが正当かチェック
    if ((header.control_byte & LORABBIT_TP_FLAG_IS_ACK) || header.packet_index != 0) {
        return E_IO; // 不正なパケット
    }

    // バッファサイズをチェック
    if ((uint32_t)header.total_packets * LORABBIT_TP_MAX_PAYLOAD > buffer_size) {
        return -1; // バッファサイズ不足
    }

    // 最初のパケットを処理
    memcpy(p_buffer, &frame.recv_data[LORABBIT_TP_HEADER_SIZE], header.payload_length);
    written_size += header.payload_length;
    if (header.control_byte & LORABBIT_TP_FLAG_ACK_REQUEST) {
        lora_send_ack(p_handle, &header);
    }

    // 伝送完了チェック
    if (header.control_byte & LORABBIT_TP_FLAG_EOT) {
        if(p_received_size) {
            *p_received_size = written_size;
        }
        return 0; // 1パケットで完了
    }

    // 残りのパケットを受信
    for (uint8_t expected_index = 1; expected_index < header.total_packets; expected_index++) {
        // パケット間のタイムアウトは短く設定
        recv_len = LoRabbit_ReceiveFrame(p_handle, &frame, LORABBIT_TP_ACK_TIMEOUT_MS);
        if (recv_len <= 0) {
            return E_TMOUT;
        }

        LoRabbitTP_Header_t subsequent_header;
        lora_parse_header(frame.recv_data, &subsequent_header);

        // パケットが現在のトランザクションに属し、かつ期待したインデックスか検証
        if ((subsequent_header.control_byte & LORABBIT_TP_FLAG_IS_ACK) ||
            (subsequent_header.transaction_id != header.transaction_id) ||
            (subsequent_header.packet_index != expected_index))
        {
            // 不正なパケットは無視してタイムアウトまで待機を続けることもできるが、
            // ここではシンプルにエラーとして終了
            return E_IO;
        }

        // データをバッファにコピー
        memcpy(&p_buffer[written_size], &frame.recv_data[LORABBIT_TP_HEADER_SIZE], subsequent_header.payload_length);
        written_size += subsequent_header.payload_length;

        // ACKを返す
        if (subsequent_header.control_byte & LORABBIT_TP_FLAG_ACK_REQUEST) {
            lora_send_ack(p_handle, &subsequent_header);
        }

        // 伝送完了チェック
        if (subsequent_header.control_byte & LORABBIT_TP_FLAG_EOT) {
            if(p_received_size) *p_received_size = written_size;
            return 0; // 完了
        }
    }

    if (p_received_size) {
        *p_received_size = written_size;
    }
    return 0;
}

int LoRabbit_SendCompressedData(LoraHandle_t *p_handle,
                                uint16_t target_address,
                                uint8_t target_channel,
                                uint8_t *p_data,
                                uint32_t size,
                                bool request_ack,
                                uint8_t *p_work_buffer,
                                uint32_t work_buffer_size)
{
    // ワークバッファのサイズが十分かチェック
    // (heatshrinkは稀にデータサイズが増えるため、少しマージンを見るのが安全)
    if (work_buffer_size < size) {
        return -1;
    }

    uint32_t compressed_size = 0;

    // 圧縮処理
    heatshrink_encoder_reset(&s_hse);

    size_t total_sunk = 0;
    size_t total_polled = 0;

    while (total_sunk < size) {
        size_t sunk_count = 0;

        // 入力データをエンコーダに渡す
        heatshrink_encoder_sink(&s_hse, &p_data[total_sunk], size - total_sunk, &sunk_count);
        total_sunk += sunk_count;

        // 圧縮されたデータを出力バッファに取り出す
        HSE_poll_res pres;
        do {
            size_t polled_count = 0;

            // p_work_buffer に書き出す
            pres = heatshrink_encoder_poll(&s_hse, &p_work_buffer[total_polled], work_buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        } while (pres == HSER_POLL_MORE);
    }

    // 最後のデータを強制的に出力
    HSE_finish_res fres;
    do {
        size_t polled_count = 0;
        fres = heatshrink_encoder_finish(&s_hse);
        if (fres == HSER_FINISH_MORE) {
            heatshrink_encoder_poll(&s_hse, &p_work_buffer[total_polled], work_buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        }
    } while (fres == HSER_FINISH_MORE);

    // 最終的な圧縮サイズを更新
    compressed_size = total_polled;

    LORA_PRINTF("Original size: %lu, Compressed size: %lu\n", size, compressed_size);

    // 圧縮したデータを、既存の大容量伝送関数で送信
    return LoRabbit_SendData(p_handle,
                             target_address,
                             target_channel,
                             p_work_buffer,   // 圧縮済みデータを渡す
                             compressed_size, // 圧縮後のサイズを渡す
                             request_ack);
}

int LoRabbit_ReceiveCompressedData(LoraHandle_t *p_handle,
                                   uint8_t *p_buffer,
                                   uint32_t buffer_size,
                                   uint32_t *p_received_size,
                                   TMO timeout,
                                   uint8_t *p_work_buffer,
                                   uint32_t work_buffer_size)
{
    uint32_t compressed_size = 0;

    // 既存の大容量受信関数で、圧縮されたデータを受信する
    int result = LoRabbit_ReceiveData(p_handle,
                                      p_work_buffer,
                                      work_buffer_size,
                                      &compressed_size,
                                      timeout);
    if (result != 0) {
        return result; // エラーまたはタイムアウト
    }

    if (p_received_size) {
        *p_received_size = 0;
    }

    // 伸長処理
    heatshrink_decoder_reset(&s_hsd);

    size_t total_sunk = 0;
    size_t total_polled = 0;

    while (total_sunk < compressed_size) {
        size_t sunk_count = 0;

        // 受信した圧縮データをデコーダに渡す
        heatshrink_decoder_sink(&s_hsd, &p_work_buffer[total_sunk], compressed_size - total_sunk, &sunk_count);
        total_sunk += sunk_count;

        // 伸長されたデータを出力バッファ(p_buffer)に取り出す
        HSD_poll_res pres;
        do {
            size_t polled_count = 0;
            // バッファに空きがある場合のみpollを呼ぶ
            if (total_polled < buffer_size) {
                pres = heatshrink_decoder_poll(&s_hsd, &p_buffer[total_polled], buffer_size - total_polled, &polled_count);
                total_polled += polled_count;
            } else {
                // バッファが満杯になったら、pollせずにループを抜ける
                pres = HSDR_POLL_EMPTY;
            }
        } while (pres == HSDR_POLL_MORE);
    }

    HSD_finish_res fres;
    do {
        fres = heatshrink_decoder_finish(&s_hsd);
        if (fres == HSDR_FINISH_MORE) {
            // バッファが満杯なのに、まだ出力データがある場合はオーバーフロー
            if (total_polled >= buffer_size) {
                return -1;
            }
            size_t polled_count = 0;
            heatshrink_decoder_poll(&s_hsd, &p_buffer[total_polled], buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        }
    } while (fres == HSDR_FINISH_MORE);

    if (fres == HSDR_FINISH_DONE) {
        LORA_PRINTF("Compressed size: %lu, Original size: %lu\n", compressed_size, total_polled);
        if (p_received_size) {
            *p_received_size = total_polled; // 最終的な伸長サイズ
        }
        return 0; // 成功
    } else {
        return E_IO; // 伸長エラー
    }
}
