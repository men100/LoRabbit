#include "SecureLoRa-TK.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>

// デバッグ出力用マクロ
#define LORA_PRINTF(...) tm_printf((UB*)__VA_ARGS__)

void LoRa_UartCallbackHandler(LoraHandle_t * p_handle, uart_callback_args_t * p_args) {
    if (UART_EVENT_RX_CHAR == p_args->event) {
        // リングバッファをハンドルから取得する
        uint16_t next_head = (p_handle->rx_head + 1) % LORA_RX_BUFFER_SIZE;
        if (next_head != p_handle->rx_tail) {
            p_handle->rx_buffer[p_handle->rx_head] = (uint8_t)p_args->data;
            p_handle->rx_head = next_head;
        }
    }
}

#ifdef SECURELORA_TK_USE_AUX_IRQ
void LoRa_AuxCallbackHandler(LoraHandle_t *p_handle, external_irq_callback_args_t *p_args) {
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

static int lora_wait_for_tx_done(LoraHandle_t *p_handle, LoRaConfigItem_t *p_config) {
#ifdef SECURELORA_TK_USE_AUX_IRQ
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

// =====================================

int LoRa_Init(LoraHandle_t * p_handle, LoraHwConfig_t const * p_hw_config) {
    if (NULL == p_handle || NULL == p_hw_config) {
        return -1;
    }

    // ハンドルにハードウェア構成をコピー
    memcpy(&p_handle->hw_config, p_hw_config, sizeof(LoraHwConfig_t));

    // リングバッファを初期化
    p_handle->rx_head = 0;
    p_handle->rx_tail = 0;

#ifdef SECURELORA_TK_USE_AUX_IRQ
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

int LoRa_InitModule(LoraHandle_t *p_handle, LoRaConfigItem_t *p_config) {
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
        return -1; // Not initialized
    }

    int ret = 0;

    LORA_PRINTF("switch to configuration mode\n");
    LoRa_SwitchToConfigurationMode(p_handle);
    tk_dly_tsk(100);

    uint8_t command[11] = {0xC0, 0x00, 0x08}; // ヘッダ(3) + パラメータ(8)
    uint8_t response[11] = {0};
    uint8_t response_len = 0;

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

    if (response_len != sizeof(command)) {
        ret = 1;
    }
    return ret;
}

#define POST_RECEIVE_TIMEOUT_MS_DEFAULT 5
int LoRa_ReceiveFrame(LoraHandle_t *p_handle, RecvFrameE220900T22SJP_t *recv_frame, TMO timeout) {
    int len = 0;
    memset(recv_frame->recv_data, 0x00, sizeof(recv_frame->recv_data));

#if defined(SECURELORA_TK_USE_AUX_IRQ)
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

int LoRa_SendFrame(LoraHandle_t *p_handle, LoRaConfigItem_t *p_config, uint8_t *send_data, int size) {
    int err = 0;
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
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
    frame[0] = p_config->target_address >> 8;
    frame[1] = p_config->target_address & 0xff;
    frame[2] = p_config->target_channel;
    memcpy(frame + 3, send_data, size);
    int frame_size = 3 + size;

#if 1 /* print debug */
      for (int i = 0; i < 3 + size; i++) {
        if (i < 3) {
          LORA_PRINTF("%02x", frame[i]);
        } else {
            LORA_PRINTF("%c", frame[i]);
        }
      }
      LORA_PRINTF("\n");
#endif

#ifdef SECURELORA_TK_USE_AUX_IRQ
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

void LoRa_SwitchToNormalMode(LoraHandle_t *p_handle) {
    // (M0, M1) = (LOW, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(100);
}

void LoRa_SwitchToWORSendingMode(LoraHandle_t *p_handle) {
    // (M0, M1) = (HIGH, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    tk_dly_tsk(100);
}

void LoRa_SwitchToWORReceivingMode(LoraHandle_t *p_handle) {
    // (M0, M1) = (LOW, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(100);
}

void LoRa_SwitchToConfigurationMode(LoraHandle_t *p_handle) {
    // (M0, M1) = (HIGH, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    tk_dly_tsk(100);
}
