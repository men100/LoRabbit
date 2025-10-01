/**
 * @file LoRabbit_config.h
 * @brief LoRabbitライブラリの動作をカスタマイズするための設定ファイル
 * @details ユーザーはこのファイル内のマクロを有効/無効にすることで、
 * ライブラリの機能をコンパイル時に選択できます。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

/**
 * @defgroup LoRabbitConfig Library Configuration
 * @brief ライブラリのコンパイル時設定
 * @{
 */

/**
 * @name Communication History Settings
 * @{
 */
#define LORABBIT_HISTORY_SIZE        32 /**< 保存する通信履歴の最大数 */
/** @} */

/**
 * @name Transport Protocol Settings
 * @{
 */
#define LORABBIT_TP_RETRY_COUNT      3    /**< 大容量データ送信時の、各パケットの最大リトライ回数 */
#define LORABBIT_TP_ACK_TIMEOUT_MS   2000 /**< ACK応答を待つタイムアウト時間 (ミリ秒) */
/** @} */

/**
 * @name Optional Feature Toggles
 * @{
 */

/**
 * @brief AUXピンによる外部割り込み機能の有効/無効
 * @details このマクロを有効にすると、AUXピンの割り込みを利用したイベント駆動の送受信が可能になります。
 * 無効時は、遅延(delay)ベースのポーリングで送受信の完了を待ちます。
 */
// #define LORABBIT_USE_AUX_IRQ

/**
 * @brief デバッグ用コンソール出力の有効/無効
 * @details このマクロを有効にすると、LORA_PRINTF() マクロがtm_printf()を呼び出し、
 * ライブラリの内部動作に関するログが出力されます。
 */
// #define LORABBIT_DEBUG_MODE

/**
 * @brief AIモデル推論によるADR (Adaptive Data Rate) 機能の有効/無効
 * @details このマクロを有効にすると、AIを用いて最適な通信パラメータを推奨する
 * LoRabbit_Get_AI_Recommendation() APIが利用可能になります。
 */
// #define LORABBIT_USE_AI_ADR

/** @} */
/** @} */ // end of LoRabbitConfig group
