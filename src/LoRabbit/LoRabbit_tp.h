/**
 * @file LoRabbit_tp.h
 * @brief LoRabbit Transport Protocol
 * @details LoRaの1パケットサイズを超える大容量データを、分割・再構築して送受信するための高レベルAPIを定義します。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

#include "LoRabbit.h" // LoraHandle_t などの型定義をインクルード

/**
 * @defgroup LoRabbitTP Transport Protocol
 * @brief 大容量データを扱うための高レベルAPI群
 * @{
 */

/**
 * @brief パケットサイズを超えるようなデータを分割して送信する。処理が完了するまでブロックする。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[in] target_address 送信先アドレス
 * @param[in] target_channel 送信先チャンネル
 * @param[in] p_data 送信するデータが格納されたバッファ
 * @param[in] size 送信するデータのサイズ (最大 約47KB)
 * @param[in] request_ack ACKを要求するかどうか (true: 信頼性通信, false: 高速通信)
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT データサイズが大きすぎる
 * @retval LORABBIT_ERROR_ACK_FAILED ACKが返ってこない
 * @retval その他 負値のエラーコード
 */
int LoRabbit_SendData(LoraHandle_t *p_handle,
                           uint16_t target_address,
                           uint8_t target_channel,
                           uint8_t *p_data,
                           uint32_t size,
                           bool request_ack);

/**
 * @brief 分割されたデータを受信し、一つのデータに復元する。処理が完了するまでブロックする。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[out] p_buffer 受信データを書き出すバッファ
 * @param[in] buffer_size p_bufferの最大サイズ
 * @param[out] p_received_size 実際に受信したデータのサイズを格納するポインタ
 * @param[in] timeout 最初のパケットを待つ最大時間(ms)。TMO_FEVRで無限待ち。
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_BUFFER_OVERFLOW バッファサイズ不足
 * @retval LORABBIT_ERROR_TIMEOUT タイムアウト
 * @retval その他 負値のエラーコード
 */
int LoRabbit_ReceiveData(LoraHandle_t *p_handle,
                              uint8_t *p_buffer,
                              uint32_t buffer_size,
                              uint32_t *p_received_size,
                              TMO timeout);

/**
 * @brief データを圧縮し、分割して送信する。処理が完了するまでブロックする。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[in] target_address 送信先アドレス
 * @param[in] target_channel 送信先チャンネル
 * @param[in] p_data 送信するデータが格納されたバッファ
 * @param[in] size 送信するデータのサイズ
 * @param[in] request_ack ACKを要求するかどうか
 * @param[in] p_work_buffer 圧縮処理に使用するワークバッファ
 * @param[in] work_buffer_size ワークバッファのサイズ。'size'以上である必要がある。
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_BUFFER_OVERFLOW ワークバッファサイズ不足
 * @retval その他 LoRabbit_SendData()が返すエラーコード
 */
int LoRabbit_SendCompressedData(LoraHandle_t *p_handle,
                                uint16_t target_address,
                                uint8_t target_channel,
                                uint8_t *p_data,
                                uint32_t size,
                                bool request_ack,
                                uint8_t *p_work_buffer,
                                uint32_t work_buffer_size);

/**
 * @brief 分割されたデータを受信し、伸長して復元する。処理が完了するまでブロックする。
 * @param[in,out] p_handle 操作対象のハンドル
 * @param[out] p_buffer 伸長後のデータを書き出すバッファ
 * @param[in] buffer_size p_bufferの最大サイズ
 * @param[out] p_received_size 実際に受信・伸長したデータのサイズを格納するポインタ
 * @param[in] timeout 最初のパケットを待つ最大時間(ms)
 * @param[in] p_work_buffer 受信と伸長処理に使用するワークバッファ
 * @param[in] work_buffer_size ワークバッファのサイズ。受信する可能性のある最大圧縮データサイズ以上である必要がある。
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_BUFFER_OVERFLOW バッファサイズ不足
 * @retval LORABBIT_ERROR_DECOMPRESS_FAILED 伸長処理失敗
 * @retval その他 LoRabbit_ReceiveData()が返すエラーコード
 */
int LoRabbit_ReceiveCompressedData(LoraHandle_t *p_handle,
                                   uint8_t *p_buffer,
                                   uint32_t buffer_size,
                                   uint32_t *p_received_size,
                                   TMO timeout,
                                   uint8_t *p_work_buffer,
                                   uint32_t work_buffer_size);

/**
 * @brief 現在の大容量データ転送の進捗状況を取得する
 * @param[in] p_handle 操作対象のハンドル
 * @param[out] p_status 取得した状態を格納する構造体へのポインタ
 * @retval LORABBIT_OK 成功
 * @retval LORABBIT_ERROR_INVALID_ARGUMENT 引数がNULL
 */
int LoRabbit_GetTransferStatus(LoraHandle_t *p_handle, LoRabbit_TransferStatus_t *p_status);

/**
 * @name Communication History Management
 * @brief 通信履歴の管理・操作を行う関数群
 * @{
 */

/**
 * @brief 現在ハンドルに蓄積されている通信履歴をコンソールに整形して出力する
 * @param[in] p_handle 操作対象のハンドル
 */
void LoRabbit_DumpHistory(LoraHandle_t *p_handle);

/**
 * @brief 現在ハンドルに蓄積されている通信履歴をCSV形式でコンソールに出力する
 * @param[in] p_handle 操作対象のハンドル
 * @retval LORABBIT_OK 成功
 */
int LoRabbit_ExportHistoryCSV(LoraHandle_t *p_handle);

/**
 * @brief ハンドル内の通信履歴リングバッファをクリアする
 * @param[in] p_handle 操作対象のハンドル
 * @retval LORABBIT_OK 成功
 */
int LoRabbit_ClearHistory(LoraHandle_t *p_handle);

/** @} */
/** @} */ // end of LoRabbitTP group
