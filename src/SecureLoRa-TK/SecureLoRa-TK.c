#include "SecureLoRa-TK.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>

// デバッグ出力用マクロ
#define LORA_PRINTF(...) tm_printf(__VA_ARGS__)

// モジュール内部で保持するUARTインスタンス
static uart_instance_t const * sp_uart_instance = NULL;

// ===== UART受信のためのリングバッファ =====
#define RX_BUFFER_SIZE 256
static volatile uint8_t g_rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t g_rx_head = 0;
static volatile uint16_t g_rx_tail = 0;

//void lora_uart_callback(uart_callback_args_t *p_args) {
void g_uart2_callback(uart_callback_args_t *p_args) {
    if (UART_EVENT_RX_CHAR == p_args->event) {
        uint16_t next_head = (g_rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head != g_rx_tail) {
            g_rx_buffer[g_rx_head] = (uint8_t)p_args->data;
            g_rx_head = next_head;
        }
    }
}

static int lora_available() {
    return (g_rx_head - g_rx_tail + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
}

static int lora_read() {
    if (g_rx_head == g_rx_tail) {
        return -1;
    }
    uint8_t data = g_rx_buffer[g_rx_tail];
    g_rx_tail = (g_rx_tail + 1) % RX_BUFFER_SIZE;
    return data;
}
// =====================================

void LoRa_Init(uart_instance_t const * p_uart_instance) {
    sp_uart_instance = p_uart_instance;
    g_rx_head = 0;
    g_rx_tail = 0;
}

int LoRa_InitModule(LoRaConfigItem_t *config) {
    if (NULL == sp_uart_instance) {
        return -1; // Not initialized
    }
    int ret = 0;

    LORA_PRINTF((UB*)"switch to configuration mode\n");
    LoRa_SwitchToConfigurationMode();
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    uint8_t command[11] = {0xC0, 0x00, 0x08}; // ヘッダ(3) + パラメータ(8)
    uint8_t response[11] = {0};
    uint8_t response_len = 0;

    command[3] = config->own_address >> 8;
    command[4] = config->own_address & 0xff;
    command[5] = (config->baud_rate << 5) | (config->air_data_rate);
    command[6] = (config->subpacket_size << 6) | (config->rssi_ambient_noise_flag << 5) |
                 (config->transmission_pause_flag << 4) | (config->transmitting_power);
    command[7] = config->own_channel;
    command[8] = (config->rssi_byte_flag << 7) | (config->transmission_method_type << 6) |
                 (config->lbt_flag << 4) | (config->wor_cycle);
    command[9] = config->encryption_key >> 8;
    command[10] = config->encryption_key & 0xff;

    LORA_PRINTF((UB*)"# Command Request\n");
    for (size_t i = 0; i < sizeof(command); i++) LORA_PRINTF((UB*)"0x%02x ", command[i]);
    LORA_PRINTF((UB*)"\n");

    R_SCI_B_UART_Write(sp_uart_instance->p_ctrl, command, sizeof(command));
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    while (lora_available() && response_len < sizeof(response)) {
        response[response_len++] = lora_read();
    }

    LORA_PRINTF((UB*)"# Command Response\n");
    for (size_t i = 0; i < response_len; i++) LORA_PRINTF((UB*)"0x%02x ", response[i]);
    LORA_PRINTF((UB*)"\n");

    if (response_len != sizeof(command)) {
        ret = 1;
    }
    return ret;
}

int LoRa_ReceiveFrame(RecvFrameE220900T22SJP_t *recv_frame) {
    int len = 0;
    memset(recv_frame->recv_data, 0x00, sizeof(recv_frame->recv_data));

    while (1) {
        while (lora_available()) {
            recv_frame->recv_data[len] = lora_read();
            len++;
            if (len >= (int)sizeof(recv_frame->recv_data) -1) return 1;
        }

        if ((lora_available() == 0) && (len > 0)) {
            R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            if (lora_available() == 0) {
                recv_frame->recv_data_len = len - 1;
                recv_frame->rssi = recv_frame->recv_data[len - 1] - 256;
                break;
            }
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return 0;
}

int LoRa_SendFrame(LoRaConfigItem_t *config, uint8_t *send_data, int size) {
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

    R_SCI_B_UART_Write(sp_uart_instance->p_ctrl, frame, frame_size);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    
    // 送信後にモジュールから応答データが返る場合があるため、バッファをクリア
    while (lora_available()) {
        lora_read();
    }

    return 0;
}

void LoRa_SwitchToNormalMode(void) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_9_GPIO, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_10_GPIO, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORSendingMode(void) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_9_GPIO, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_10_GPIO, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToWORReceivingMode(void) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_9_GPIO, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_10_GPIO, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

void LoRa_SwitchToConfigurationMode(void) {
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_9_GPIO, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PMOD2_10_GPIO, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}
