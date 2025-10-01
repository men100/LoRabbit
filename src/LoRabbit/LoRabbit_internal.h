/**
 * @file LoRabbit_internal.h
 * @brief LoRabbitライブラリの内部でのみ使用される定義と関数
 * @details デバッグマクロや、内部モジュール間で共有される関数プロトタイプを定義します。
 * ユーザーがこのヘッダを直接インクルードすることはありません。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

#include "LoRabbit_config.h"
#include "LoRabbit.h" // LoraHandle_t のために必要

/**
 * @defgroup LoRabbitInternal Internal Functions
 * @brief ライブラリ内部でのみ使用される関数とマクロ
 * @internal
 * @{
 */

// LORABBIT_DEBUG_MODE が定義されている場合のみデバッグ出力を有効にする
#ifdef LORABBIT_DEBUG_MODE
    #include <tm/tmonitor.h>
    /**
     * @brief デバッグ出力用マクロ。LORABBIT_DEBUG_MODEが有効な場合のみtm_printfを呼び出す。
     */
    #define LORA_PRINTF(...) tm_printf((UB*)__VA_ARGS__)
#else
    #define LORA_PRINTF(...) ((void)0)
#endif

/**
 * @brief LoRaフレームを送信するが、完了を待たない (Fire and Forget)
 * @details ACKパケットの送信など、応答を待つ必要がない場合に使用される内部関数。
 * @param p_handle 操作対象のハンドル
 * @param target_address 送信先アドレス
 * @param target_channel 送信先チャンネル
 * @param p_send_data 送信データ
 * @param size 送信データサイズ
 * @retval LORABBIT_OK 成功
 */
ER lora_send_frame_fire_and_forget_internal(LoraHandle_t *p_handle,
                                            uint16_t target_address,
                                            uint8_t target_channel,
                                            uint8_t *p_send_data,
                                            int size);
/** @} */ // end of LoRabbitInternal group
