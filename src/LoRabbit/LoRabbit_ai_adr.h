#pragma once
#include "LoRabbit.h"

#ifdef LORABBIT_USE_AI_ADR
/**
 * @brief 通信履歴を元にAIが最適なパラメータを予測して返す
 * @details ハンドル内の最新の通信履歴を入力としてAIモデルを実行し、
 * 最適と予測されたデータレートと送信電力をポインタ引数に格納します。
 * この関数は、LoRaモジュールの設定を自動的には変更しません。
 * @param[in] p_handle 操作対象のハンドル
 * @param[out] p_recommendation AIが推奨する設定が格納される構造体へのポインタ
 * @retval LORABBIT_OK 予測に成功
 * @retval LORABBIT_ERROR_... 予測に失敗
 */
int LoRabbit_Get_AI_Recommendation(LoraHandle_t *p_handle, LoRaRecommendedConfig_t *p_recommendation);
#endif
