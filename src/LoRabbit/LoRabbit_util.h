/**
 * @file LoRabbit_util.h
 * @brief LoRabbitライブラリの内部で使われるユーティリティ関数
 * @details データレートのenum値から物理パラメータ（SF, BW）への変換などを行います。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

#include "LoRabbit.h"

/**
 * @defgroup LoRabbitUtil Utility Functions
 * @brief 内部で利用されるヘルパー関数群
 * @internal
 * @{
 */

/**
 * @brief 空中データレートのenum値からSpreading Factor (SF) を取得する
 * @param[in] air_data_rate 空中データレートのenum値
 * @return Spreading Factor (5-11)
 */
int get_spreading_factor_from_air_data_rate(LoraAirDateRate_t air_data_rate);

/**
 * @brief 空中データレートのenum値から帯域幅 (Bandwidth) を取得する
 * @param[in] air_data_rate 空中データレートのenum値
 * @return 帯域幅 (kHz)
 */
double get_bandwidth_khz_from_air_data_rate(LoraAirDateRate_t air_data_rate);

/**
 * @brief UARTボーレートのenum値をFSPが要求するuint32_t型のボーレート値に変換する
 * @param[in] rate_enum UARTボーレートのenum値
 * @return ボーレート値 (e.g., 9600)
 */
uint32_t lora_enum_to_fsp_baud(LoraUartBaudRate_t rate_enum);

/** @} */ // end of LoRabbitUtil group
