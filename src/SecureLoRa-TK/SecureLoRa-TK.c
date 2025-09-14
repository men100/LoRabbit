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
    tk_sig_sem(p_handle->tx_done_sem_id, 1);
}
#endif

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

static int lora_wait_for_tx_done(LoraHandle_t *p_handle) {
#ifdef SECURELORA_TK_USE_AUX_IRQ
    // TODO: 送信時間に応じてある程度待機時間は算出可能
    // AUXピンが未接続の場合は、従来通り固定長ディレイで代用
    if (LORA_PIN_UNDEFINED == p_handle->hw_config.aux) {
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
        return E_OK;
    }

    // タイムアウトを6秒に設定してセマフォを待つ
    ER err = tk_wai_sem(p_handle->tx_done_sem_id, 1, 6000);

    return err; // E_OK:成功, E_TMOUT:タイムアウト
#else
    // 常に固定長ディレイ
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
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

        // セマフォを生成
        p_handle->tx_done_sem_id = tk_cre_sem(&csem);
        if (p_handle->tx_done_sem_id < E_OK) {
            // エラー処理 (IDが負の値で返る)
            LORA_PRINTF("LoRa_Init: tk_cre_sem failed\n");
            return -2; // セマフォ生成失敗
        }
    }
#endif

    return 0;
}

int LoRa_InitModule(LoraHandle_t * p_handle, LoRaConfigItem_t * p_config) {
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
        return -1; // Not initialized
    }

    int ret = 0;

    LORA_PRINTF("switch to configuration mode\n");
    LoRa_SwitchToConfigurationMode(p_handle);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    uint8_t command[11] = {0xC0, 0x00, 0x08}; // ヘッダ(3) + パラメータ(8)
    uint8_t response[11] = {0};
    uint8_t response_len = 0;

    command[3] = p_config->own_address >> 8;
    command[4] = p_config->own_address & 0xff;
    command[5] = (p_config->baud_rate << 5) | (p_config->air_data_rate);
    command[6] = (p_config->subpacket_size << 6) | (p_config->rssi_ambient_noise_flag << 5) |
                 (p_config->transmission_pause_flag << 4) | (p_config->transmitting_power);
    command[7] = p_config->own_channel;
    command[8] = (p_config->rssi_byte_flag << 7) | (p_config->transmission_method_type << 6) |
                 (p_config->lbt_flag << 4) | (p_config->wor_cycle);
    command[9] = p_config->encryption_key >> 8;
    command[10] = p_config->encryption_key & 0xff;

    LORA_PRINTF("# Command Request\n");
    for (size_t i = 0; i < sizeof(command); i++) {
        LORA_PRINTF("0x%02x ", command[i]);
    }
    LORA_PRINTF("\n");

    p_uart->p_api->write(p_uart->p_ctrl, command, sizeof(command));
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

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

int LoRa_ReceiveFrame(LoraHandle_t * p_handle, RecvFrameE220900T22SJP_t *recv_frame) {
    int len = 0;
    memset(recv_frame->recv_data, 0x00, sizeof(recv_frame->recv_data));

    while (1) {
        while (lora_available(p_handle)) {
            recv_frame->recv_data[len] = lora_read(p_handle);
            len++;
            if (len >= (int)sizeof(recv_frame->recv_data) -1) return 1;
        }

        if ((lora_available(p_handle) == 0) && (len > 0)) {
            R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            if (lora_available(p_handle) == 0) {
                recv_frame->recv_data_len = len - 1;
                recv_frame->rssi = recv_frame->recv_data[len - 1] - 256;
                break;
            }
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return 0;
}

int LoRa_SendFrame(LoraHandle_t * p_handle, LoRaConfigItem_t *config, uint8_t *send_data, int size) {
    int err = 0;
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
        return -1; // Not initialized
    }

    uint8_t subpacket_size = 200;
    switch (config->subpacket_size) {
        case 0b01: subpacket_size = 128; break;
        case 0b10: subpacket_size = 64; break;
        case 0b11: subpacket_size = 32; break;
    }
    if (size > subpacket_size) {
        LORA_PRINTF("send data length too long\n");
        return 1;
    }

    uint8_t frame[3 + 200]; // 最大サイズでバッファ確保
    frame[0] = config->target_address >> 8;
    frame[1] = config->target_address & 0xff;
    frame[2] = config->target_channel;
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

    p_uart->p_api->write(p_uart->p_ctrl, frame, frame_size);
    err = lora_wait_for_tx_done(p_handle);
    if (err < 0) {
        LORA_PRINTF("LoRa_SendFrame: lora_wait_for_tx_done timeout\n");
    }
    
    // 送信後にモジュールから応答データが返る場合があるため、バッファをクリア
    while (lora_available(p_handle)) {
        lora_read(p_handle);
    }

    return 0;
}

void LoRa_SwitchToNormalMode(LoraHandle_t * p_handle) {
    // (M0, M1) = (LOW, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORSendingMode(LoraHandle_t * p_handle) {
    // (M0, M1) = (HIGH, LOW)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORReceivingMode(LoraHandle_t * p_handle) {
    // (M0, M1) = (LOW, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToConfigurationMode(LoraHandle_t * p_handle) {
    // (M0, M1) = (HIGH, HIGH)
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}
