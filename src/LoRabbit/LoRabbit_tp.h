#pragma once

/**
 * @brief パケットを超えるようなデータを分割して送信する。処理が完了するまでブロックする。
 * @param p_handle 操作対象のハンドル
 * @param target_address 送信先アドレス
 * @param target_channel 送信先チャンネル
 * @param p_data 送信するデータが格納されたバッファ
 * @param size 送信するデータのサイズ (最大 約47KB)
 * @param request_ack ACKを要求するかどうか (true: 信頼性通信, false: 高速通信)
 * @return 0:成功, E_SZOVER:サイズ超過, E_TMOUT:ACKタイムアウト, その他負値:エラー
 */
int LoRabbit_SendData(LoraHandle_t *p_handle,
                           uint16_t target_address,
                           uint8_t target_channel,
                           uint8_t *p_data,
                           uint32_t size,
                           bool request_ack);

/**
 * @brief 分割されたデータを受信し、一つのデータに復元する。処理が完了するまでブロックする。
 * @param p_handle 操作対象のハンドル
 * @param p_buffer 受信データを書き出すバッファ
 * @param buffer_size p_bufferの最大サイズ
 * @param p_received_size 実際に受信したデータのサイズを格納するポインタ
 * @param timeout 最初のパケットを待つ最大時間(ms)。TMO_FEVRで無限待ち。
 * @return 0:成功, E_SZOVER:バッファサイズ不足, E_TMOUT:タイムアウト, その他負値:エラー
 */
int LoRabbit_ReceiveData(LoraHandle_t *p_handle,
                              uint8_t *p_buffer,
                              uint32_t buffer_size,
                              uint32_t *p_received_size,
                              TMO timeout);

/**
 * @brief データを圧縮し、分割して送信する。処理が完了するまでブロックする。
 * @param p_handle 操作対象のハンドル
 * @param target_address 送信先アドレス
 * @param target_channel 送信先チャンネル
 * @param p_data 送信するデータが格納されたバッファ
 * @param size 送信するデータのサイズ
 * @param request_ack ACKを要求するかどうか
 * @param p_work_buffer 圧縮処理に使用するワークバッファ
 * @param work_buffer_size ワークバッファのサイズ。'size'以上である必要がある。
 * @return 0:成功, E_SZOVER:サイズ超過, E_TMOUT:ACKタイムアウト, その他負値:エラー
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
 * @param p_handle 操作対象のハンドル
 * @param p_buffer [out] 伸長後のデータを書き出すバッファ
 * @param buffer_size [in] p_bufferの最大サイズ
 * @param p_received_size [out] 実際に受信・伸長したデータのサイズを格納するポインタ
 * @param timeout [in] 最初のパケットを待つ最大時間(ms)
 * @param p_work_buffer [in] 受信と伸長処理に使用するワークバッファ
 * @param work_buffer_size [in] ワークバッファのサイズ。受信する可能性のある最大圧縮データサイズ以上である必要がある。
 * @return 0:成功, E_SZOVER:バッファサイズ不足, E_TMOUT:タイムアウト, その他負値:エラー
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
 */
int LoRabbit_GetTransferStatus(LoraHandle_t *p_handle, LoRabbit_TransferStatus_t *p_status);

/**
 * @brief 現在ハンドルに蓄積されている通信履歴をコンソールに出力する
 * @param[in] p_handle 操作対象のハンドル
 */
void LoRabbit_DumpHistory(LoraHandle_t *p_handle);
