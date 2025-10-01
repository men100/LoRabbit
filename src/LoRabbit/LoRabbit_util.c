/**
 * @file LoRabbit_util.c
 * @brief LoRabbitライブラリのユーティリティ関数の実装
 * @details LoRabbit_util.hで宣言されたヘルパー関数を実装します。
 */
#include "LoRabbit_util.h"

// air_data_rate から Spreading Factor を返す
inline int get_spreading_factor_from_air_data_rate(LoraAirDateRate_t air_data_rate) {
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
inline double get_bandwidth_khz_from_air_data_rate(LoraAirDateRate_t air_data_rate) {
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
