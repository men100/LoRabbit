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
