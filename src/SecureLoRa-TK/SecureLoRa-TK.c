#include "SecureLoRa-TK.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>

// デバッグ出力用マクロ
#define LORA_PRINTF(...) tm_printf(__VA_ARGS__)

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

static int lora_available(LoraHandle_t * p_handle) {
    return (p_handle->rx_head - p_handle->rx_tail + LORA_RX_BUFFER_SIZE) % LORA_RX_BUFFER_SIZE;
}

static int lora_read(LoraHandle_t * p_handle) {
    if (p_handle->rx_head == p_handle->rx_tail) {
        return -1;
    }

    uint8_t data = p_handle->rx_buffer[p_handle->rx_tail];
    p_handle->rx_tail = (p_handle->rx_tail + 1) % LORA_RX_BUFFER_SIZE;
    return data;
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

    return 0;
}

int LoRa_InitModule(LoraHandle_t * p_handle, LoRaConfigItem_t * p_config) {
    const uart_instance_t *p_uart = p_handle->hw_config.p_uart;
    if (NULL == p_uart) {
        return -1; // Not initialized
    }

    int ret = 0;

    LORA_PRINTF((UB*)"switch to configuration mode\n");
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

    LORA_PRINTF((UB*)"# Command Request\n");
    for (size_t i = 0; i < sizeof(command); i++) {
        LORA_PRINTF((UB*)"0x%02x ", command[i]);
    }
    LORA_PRINTF((UB*)"\n");

    R_SCI_B_UART_Write(p_uart->p_ctrl, command, sizeof(command));
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    while (lora_available(p_handle) && response_len < sizeof(response)) {
        response[response_len++] = lora_read(p_handle);
    }

    LORA_PRINTF((UB*)"# Command Response\n");
    for (size_t i = 0; i < response_len; i++) {
        LORA_PRINTF((UB*)"0x%02x ", response[i]);
    }
    LORA_PRINTF((UB*)"\n");

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
        LORA_PRINTF((UB*)"send data length too long\n");
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
          LORA_PRINTF((UB*)"%02x", frame[i]);
        } else {
            LORA_PRINTF((UB*)"%c", frame[i]);
        }
      }
      LORA_PRINTF((UB*)"\n");
#endif

    R_SCI_B_UART_Write(p_uart->p_ctrl, frame, frame_size);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    
    // 送信後にモジュールから応答データが返る場合があるため、バッファをクリア
    while (lora_available(p_handle)) {
        lora_read(p_handle);
    }

    return 0;
}

void LoRa_SwitchToNormalMode(LoraHandle_t * p_handle) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORSendingMode(LoraHandle_t * p_handle) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORReceivingMode(LoraHandle_t * p_handle) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToConfigurationMode(LoraHandle_t * p_handle) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m0, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, p_handle->hw_config.m1, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}
