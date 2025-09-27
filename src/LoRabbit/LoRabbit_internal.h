#pragma once

#include "LoRabbit_config.h"

// LORABBIT_DEBUG_MODE が定義されている場合のみデバッグ出力を有効にする
#ifdef LORABBIT_DEBUG_MODE
    #include <tm/tmonitor.h>
    // デバッグ出力用マクロ
    #define LORA_PRINTF(...) tm_printf((UB*)__VA_ARGS__)
#else
    #define LORA_PRINTF(...) ((void)0)
#endif

/** @internal HAL層：LoRaフレームを送信するが、完了を待たない */
ER lora_send_frame_fire_and_forget_internal(LoraHandle_t *p_handle,
                                            uint16_t target_address,
                                            uint8_t target_channel,
                                            uint8_t *p_send_data,
                                            int size);
