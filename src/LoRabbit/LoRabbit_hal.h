/**
 * @file LoRabbit_hal.h
 * @brief LoRabbit Hardware Abstraction Layer (HAL)
 * @details LoRaモジュールを直接操作するための基本的なAPI（初期化、送受信、モード切替など）を定義します。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

#include "LoRabbit.h" // LoraHandle_t などの型定義をインクルード

/**
 * @defgroup LoRabbitHAL Hardware Abstraction Layer
 * @brief LoRaモジュールを直接制御する低レベルAPI群
 * @{
 */

/**
 * @brief LoRaハンドルを初期化する
 * @details LoRaライブラリを使用する前に必ず呼び出す必要があります。
 * ハンドル内の各種変数を初期化し、同期用のセマフォを生成します。
 * @param[in,out] p_handle 初期化対象のハンドル
 * @param[in] p_hw_config 使用するハードウェアの構成
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT 引数がNULL
 * @retval 負値 μT-Kernelのセマフォ生成エラーコード
 */
int LoRabbit_Init(LoraHandle_t *p_handle, LoraHwConfig_t const *p_hw_config);

/**
 * @brief LoRaモジュールの設定を書き込む
 * @details モジュールをコンフィグレーションモードに移行させ、指定された設定を書き込みます。
 * 成功した場合、設定はハンドル内にも保存されます。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[in] p_config LoRaモジュールに書き込む設定値
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT 引数がNULL
 * @retval LORABBIT_ERROR_WRITE_CONFIG 書き込み失敗
 */
int LoRabbit_InitModule(LoraHandle_t *p_handle, LoraConfigItem_t *p_config);

/**
 * @brief LoRaフレームを1つ受信する
 * @details UARTからデータを受信し、1つのLoRaフレームとして返します。
 * この関数はデータを受信するまで、あるいはタイムアウトするまでブロックします。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[out] recv_frame LoRa受信データの格納先
 * @param[in] timeout [割り込み利用時のみ] 受信開始を待つタイムアウト値(ms)。ポーリング時は無視されます。
 * @return 0より大きい値: 受信したペイロード長, 0:タイムアウト(受信なし), 負値:エラー
 */
int LoRabbit_ReceiveFrame(LoraHandle_t *p_handle, RecvFrameE220900T22SJP_t *recv_frame, TMO timeout);

/**
 * @brief LoRaフレームを1つ送信する
 * @details 指定されたデータからLoRaフレームを組み立てて送信します。
 * 送信が完了するまでブロックします。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[in] target_address 送信先アドレス
 * @param[in] target_channel 送信先チャンネル
 * @param[in] p_send_data 送信データが格納されたバッファ
 * @param[in] size 送信データサイズ
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT 引数が不正（サイズ超過など）
 * @retval LORABBIT_ERROR_TIMEOUT タイムアウト（割り込み利用時）
 */
int LoRabbit_SendFrame(LoraHandle_t *p_handle, uint16_t target_address, uint8_t target_channel, uint8_t *p_send_data, int size);

/**
 * @brief LoRaパケットの空中占有時間(Time on Air)を計算する
 * @param[in] air_data_rate 使用する空中データレート
 * @param[in] payload_size_bytes ペイロードのバイト数
 * @return 計算された時間 (ミリ秒)
 */
int LoRabbit_GetTimeOnAirMsec(LoraAirDateRate_t air_data_rate, uint8_t payload_size_bytes);

/**
 * @name LoRa Module Operation Modes
 * @brief LoRaモジュールの動作モードを切り替える関数群
 * @{
 */
void LoRabbit_SwitchToNormalMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToWORSendingMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToWORReceivingMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToConfigurationMode(LoraHandle_t *p_handle);
/** @} */

/**
 * @name FSP Callback Handlers
 * @brief FSPの割り込みコールバック内からユーザーが呼び出すべきハンドラ関数
 * @{
 */

/**
 * @brief UART受信割り込み時に呼び出すべきハンドラ関数
 * @details ユーザーはFSPのUARTコールバック関数の中からこの関数を呼び出してください。
 * 受信したデータをライブラリ内部のリングバッファに格納します。
 * @param[in,out] p_handle 該当するLoRaハンドル
 * @param[in] p_args FSPコールバックから渡される引数
 */
void LoRabbit_UartCallbackHandler(LoraHandle_t *p_handle, uart_callback_args_t *p_args);

#ifdef LORABBIT_USE_AUX_IRQ
/**
 * @brief LoRa AUXピンの外部割り込み時に呼び出すべきハンドラ関数
 * @details ユーザーはFSPの外部割り込み(ICU)コールバックの中からこの関数を呼び出してください。
 * 送受信の完了を検知し、同期用セマフォを解放します。
 * @param[in,out] p_handle 該当するLoRaハンドル
 * @param[in] p_args FSPコールバックから渡される引数
 */
void LoRabbit_AuxCallbackHandler(LoraHandle_t *p_handle, external_irq_callback_args_t *p_args);
#endif

/** @} */

/** @} */ // end of LoRabbitHAL group
