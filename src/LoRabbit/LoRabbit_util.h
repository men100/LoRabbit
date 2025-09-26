#pragma once

#include "LoRabbit.h"

int get_spreading_factor_from_air_data_rate(LoraAirDateRate_t air_data_rate);
double get_bandwidth_khz_from_air_data_rate(LoraAirDateRate_t air_data_rate);
uint32_t lora_enum_to_fsp_baud(LoraUartBaudRate_t rate_enum);
