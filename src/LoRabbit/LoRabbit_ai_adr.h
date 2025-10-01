/**
 * @file LoRabbit_ai_adr.h
 * @brief AI-based Adaptive Data Rate (AI-ADR)
 * @details 通信履歴を元にe-AIが最適な通信パラメータを予測するためのAPIを定義します。
 * @author men100
 * @date 2025/09/30
 */
#pragma once
#include "LoRabbit.h"

#ifdef LORABBIT_USE_AI_ADR

/**
 * @defgroup LoRabbitAI AI-based ADR
 * @brief AIを用いて最適な通信パラメータを予測するAPI
 * @{
 */

/**
 * @brief 通信履歴を元にAIが最適なパラメータを予測して返す
 * @details ハンドル内の最新の通信履歴を入力としてAIモデルを実行し、
 * 最適と予測されたデータレートと送信電力をポインタ引数に格納します。
 * この関数は、LoRaモジュールの設定を自動的には変更しません。
 * @param[in] p_handle 操作対象のハンドル
 * @param[out] p_recommendation AIが推奨する設定が格納される構造体へのポインタ
 * @retval LORABBIT_OK 予測に成功
 * @retval LORABBIT_ERROR_NOT_READY_DATA_FOR_AI 推論に必要な履歴データがない
 * @retval LORABBIT_ERROR_AI_INFERENCE_FAILED 推論に失敗
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT 引数がNULL
 */
int LoRabbit_Get_AI_Recommendation(LoraHandle_t *p_handle, LoRaRecommendedConfig_t *p_recommendation);

/** @} */ // end of LoRabbitAI group
#endif
