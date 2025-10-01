/**
 * @file LoRabbit_ai_adr.c
 * @brief AI-based Adaptive Data Rate (AI-ADR) の実装
 * @details LoRabbit_ai_adr.hで宣言された、e-AIを用いた最適パラメータ予測機能を実装します。
 */
#include "LoRabbit_ai_adr.h"

#ifdef LORABBIT_USE_AI_ADR

#include "LoRabbit_internal.h"
#include "LoRabbit_hal.h"
#include "LoRabbit_util.h"

#include <stdbool.h>

// e-AIトランスレータが生成したヘッダ
#include "Typedef.h"
#include "layer_shapes_lora_adr.h"
#include "weights_lora_adr.h"

// ビルドエラー回避のため、グローバル変数の実体をこちらで定義する (layer_shapes_lora_adr.h)
TsInt8 dnn_buffer1[16];
TsInt8 dnn_buffer2[8];

struct shapes_lora_adr layer_shapes_lora_adr ={
    {1,2,2,16},
    {1,16,16,8},
    {1,8,8,4},
    4
};

// ビルドエラー回避のため、グローバル変数の実体をこちらで定義する (weights_lora_adr.h)
// Input, Output scale and zero point values of the model
const TPrecision ip_op_scale[] = {0.364705890417099,0.00390625};
const TsInt8 ip_op_zero_point[] = {124,-128};
// Weights and bias tensor values including zero point and multiplier values for required layers
const TsInt32 sequential_1_dense_1_MatMul_multiplier[] = {1433845854};
const TsInt8 sequential_1_dense_1_MatMul_shift[] = {8};
const TsInt8 sequential_1_dense_1_MatMul_offset[] = {124,-128};
const TsInt8 sequential_1_dense_1_MatMul_weights[] = {-127,111,-127,-127,127,1,38,-81,-127,127,127,-127,-79,127,79,-64,43,-127,-23,7,100,-127,-127,127,-111,0,119,-108,-127,100,-127,-127};
const TsInt32 sequential_1_dense_1_MatMul_biases[] = {-248,0,104,-36,0,-74,0,-152,-108,0,0,63,197,0,0,-71};
const TsInt32 sequential_1_dense_1_2_MatMul_multiplier[] = {1229603229};
const TsInt8 sequential_1_dense_1_2_MatMul_shift[] = {4};
const TsInt8 sequential_1_dense_1_2_MatMul_offset[] = {-128,-128};
const TsInt8 sequential_1_dense_1_2_MatMul_weights[] = {109,25,-101,67,-90,-19,109,-50,-8,58,127,-78,-47,-17,-36,86,0,-13,63,-28,88,100,-32,-102,-109,-85,-61,93,-21,-72,-59,-92,91,-90,-64,-85,-52,-7,-82,76,-71,127,-57,39,-67,-35,64,-127,21,1,-66,41,52,16,-109,-119,126,102,-93,66,93,-67,39,-109,-82,-17,37,-14,52,86,45,-7,127,17,126,-16,68,53,-32,-91,23,83,59,99,-20,104,123,-9,-6,-20,-49,-127,-127,-80,2,118,36,-74,-95,-44,97,-42,-75,84,68,36,-2,-99,-125,127,47,-32,-119,-43,118,-10,-25,10,127,66,-97,-6,-9,89,-103,-116,87,7};
const TsInt32 sequential_1_dense_1_2_MatMul_biases[] = {0,0,0,-140,0,0,-238,0};
const TsInt32 sequential_1_dense_2_1_MatMul_multiplier[] = {2128051528};
const TsInt8 sequential_1_dense_2_1_MatMul_shift[] = {7};
const TsInt8 sequential_1_dense_2_1_MatMul_offset[] = {-128,30};
const TsInt8 sequential_1_dense_2_1_MatMul_weights[] = {48,-4,-96,116,-18,-102,-127,-14,4,-52,-17,-40,11,-67,-28,84,89,80,54,112,-54,-47,1,127,54,-127,-81,39,127,-120,-44,-9};
const TsInt32 sequential_1_dense_2_1_MatMul_biases[] = {-1212,2307,1340,-761};
const TPrecision StatefulPartitionedCall_1_0_multiplier[] = {0.013001230545341969,0.00390625};
const TsInt8 StatefulPartitionedCall_1_0_offset[] = {30,-128};

/**
 * @brief AIモデルで最適なパラメータ（データレートと送信電力）を予測する（内部関数）
 */
static int predict_best_parameters(int8_t last_ack_rssi,
                                  bool last_ack_success,
                                  LoRaRecommendedConfig_t *p_recommendation)
{
    // AIへの入力データを準備し、「量子化」する
    TsIN ai_input[2];

    // 量子化の公式: quantized_value = real_value / scale + zero_point
    // weights.h から入力層のスケールとゼロ点を取得
    const TPrecision input_scale = ip_op_scale[0];
    const TsInt8 input_zero_point = ip_op_zero_point[0];

    ai_input[0] = (TsIN)(roundf((float)last_ack_rssi / input_scale) + input_zero_point);
    ai_input[1] = (TsIN)(roundf((last_ack_success ? 1.0f : 0.0f) / input_scale) + input_zero_point);

    // e-AIの推論関数を呼び出す
    TsInt errorcode = 0;
    TsOUT *ai_output_quantized = dnn_compute_lora_adr(ai_input, &errorcode);
    if (errorcode != 0) {
        return LORABBIT_ERROR_AI_INFERENCE_FAILED;
    }

    // 出力結果（量子化済み）を「逆量子化」して確率に戻す
    float probabilities[4];
    const TPrecision output_scale = ip_op_scale[1];
    const TsInt8 output_zero_point = ip_op_zero_point[1];

    for (int i = 0; i < 4; i++) {
        // 逆量子化の公式: real_value = (quantized_value - zero_point) * scale
        probabilities[i] = (float)(ai_output_quantized[i] - output_zero_point) * output_scale;
    }

    // 最も確率の高いクラスのインデックスを探す
    int best_class_index = -1;
    float best_probability = -1.0f;
    for (int i = 0; i < 4; i++) {
        if (probabilities[i] > best_probability) {
            best_probability = probabilities[i];
            best_class_index = i;
        }
    }

    LORA_PRINTF("AI-ADR: Probabilities: [%.2f, %.2f, %.2f, %.2f], Best index: %d\n",
                probabilities[0], probabilities[1], probabilities[2], probabilities[3], best_class_index);

    // インデックスを、データレートと送信電力のペアに変換する
    if (best_class_index != -1) {
        // このマッピング配列は、Pythonの訓練時のクラス順序と合わせる
        static const LoRaRecommendedConfig_t class_to_config_map[] = {
            { LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500, LORA_TRANSMITTING_POWER_13_DBM }, // 高速・近距離向け
            { LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125,  LORA_TRANSMITTING_POWER_13_DBM }, // 中速・中距離 （LoRaWANで一般的）
            { LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125,  LORA_TRANSMITTING_POWER_13_DBM }, // デフォルト設定
            { LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500, LORA_TRANSMITTING_POWER_13_DBM }  // 低速・長距離向け
        };

        if (best_class_index < (sizeof(class_to_config_map) / sizeof(class_to_config_map[0]))) {
            memcpy(p_recommendation, &class_to_config_map[best_class_index], sizeof(LoRaRecommendedConfig_t));
            return LORABBIT_OK;
        }
    }

    return LORABBIT_ERROR_AI_INFERENCE_FAILED;
}

/**
 * @brief AI-ADRのメイン関数（新しいAPI）
 */
int LoRabbit_Get_AI_Recommendation(LoraHandle_t *p_handle, LoRaRecommendedConfig_t *p_recommendation) {
    if (NULL == p_handle || NULL == p_recommendation) {
        return LORABBIT_ERROR_INVALID_ARGUMENT;
    }

    // 最新の通信履歴を取得
    if (p_handle->history_index == 0 && !p_handle->history_wrapped) {
        return LORABBIT_ERROR_NOT_READY_DATA_FOR_AI;
    }
    uint8_t last_log_index = (p_handle->history_index - 1 + LORABBIT_HISTORY_SIZE) % LORABBIT_HISTORY_SIZE;
    const LoraCommLog_t *p_last_log = &p_handle->history[last_log_index];

    // 特徴量を抽出
    int8_t last_rssi = p_last_log->last_ack_rssi;
    bool last_success = p_last_log->ack_success;

    // AIモデルで予測を実行し、結果をポインタ引数に格納
    int ret = predict_best_parameters(last_rssi, last_success, p_recommendation);

    return ret;
}
#endif
