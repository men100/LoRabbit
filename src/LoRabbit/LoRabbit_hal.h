#pragma once

/**
 * @brief LoRaハンドルを初期化する
 * @param p_handle 初期化対象のハンドル
 * @param p_hw_config 使用するハードウェアの構成
 * @return 0:成功, -1:失敗
 */
int LoRabbit_Init(LoraHandle_t *p_handle, LoraHwConfig_t const *p_hw_config);

/**
 * @brief LoRaモジュールの設定を行う
 * @param p_handle 操作対象のハンドル
 * @param p_config LoRa設定値
 * @return 0:成功, 1:失敗, -1:未初期化
 */
int LoRabbit_InitModule(LoraHandle_t *p_handle, LoraConfigItem_t *p_config);

/**
 * @brief LoRa受信を行う
 * @param p_handle 操作対象のハンドル
 * @param recv_frame LoRa受信データの格納先
 * @param timeout [割り込み利用時のみ] 受信開始を待つタイムアウト値(ms)。ポーリング時は無視される。
 * @return 0より大きい:受信データ長, 0:タイムアウト(受信なし), 負:エラー
 */
int LoRabbit_ReceiveFrame(LoraHandle_t *p_handle, RecvFrameE220900T22SJP_t *recv_frame, TMO timeout);

/**
 * @brief LoRa送信を行う
 * @param p_handle 操作対象のハンドル
 * @param target_address 送信先アドレス
 * @param target_channel 送信先チャンネル
 * @param p_send_data 送信データ
 * @param size 送信データサイズ
 * @return 0:成功, 1:失敗
 */
int LoRabbit_SendFrame(LoraHandle_t *p_handle, uint16_t target_address, uint8_t target_channel, uint8_t *p_send_data, int size);

/**
 * @brief LoRaパケットの空中占有時間(Time on Air)を計算する
 * @param air_data_rate 使用する空中データレート
 * @param payload_size_bytes ペイロードのバイト数
 * @return 計算された時間 (ミリ秒)
 */
int LoRabbit_GetTimeOnAirMsec(LoraAirDateRate_t air_data_rate, uint8_t payload_size_bytes);

/**
 * @brief 各種動作モードへ移行する関数群
 */
void LoRabbit_SwitchToNormalMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToWORSendingMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToWORReceivingMode(LoraHandle_t *p_handle);
void LoRabbit_SwitchToConfigurationMode(LoraHandle_t *p_handle);

/**
 * @brief UART割り込み時に呼び出すべきハンドラ関数
 * @note ユーザーはFSPのコールバック関数の中からこの関数を呼び出す
 * @param p_handle 該当するLoRaハンドル
 * @param p_args FSPコールバックから渡される引数
 */
void LoRabbit_UartCallbackHandler(LoraHandle_t *p_handle, uart_callback_args_t *p_args);

#ifdef LORABBIT_USE_AUX_IRQ
/**
 * @brief LoRa AUX割り込み時に呼び出すべきハンドラ関数
 * @note ユーザーはFSPのコールバック関数の中からこの関数を呼び出す
 * @param p_handle 該当するLoRaハンドル
 * @param p_args FSPコールバックから渡される引数
 */
void LoRabbit_AuxCallbackHandler(LoraHandle_t *p_handle, external_irq_callback_args_t *p_args);
#endif
