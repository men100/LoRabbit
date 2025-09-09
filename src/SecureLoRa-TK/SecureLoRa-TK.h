#pragma once

#include "hal_data.h"
#include <stdint.h>

// E220-900T22S(JP)の設定項目
typedef struct {
  uint16_t own_address;
  uint8_t baud_rate;
  uint8_t air_data_rate;
  uint8_t subpacket_size;
  uint8_t rssi_ambient_noise_flag;
  uint8_t transmission_pause_flag;
  uint8_t transmitting_power;
  uint8_t own_channel;
  uint8_t rssi_byte_flag;
  uint8_t transmission_method_type;
  uint8_t lbt_flag;
  uint16_t wor_cycle;
  uint16_t encryption_key;
  uint16_t target_address;
  uint8_t target_channel;
} LoRaConfigItem_t;

// 受信フレーム構造体
typedef struct {
  uint8_t recv_data[201];
  uint8_t recv_data_len;
  int rssi;
} RecvFrameE220900T22SJP_t;

/**
 * @brief LoRaライブラリを初期化する
 * @param p_uart_instance 使用するUARTのインスタンス
 */
void LoRa_Init(uart_instance_t const * p_uart_instance);

/**
 * @brief E220-900T22S(JP)へLoRa初期設定を行う
 * @param config LoRa設定値
 * @return 0:成功, 1:失敗, -1:未初期化
 */
int LoRa_InitModule(LoRaConfigItem_t *config);

/**
 * @brief LoRa受信を行う
 * @param recv_frame LoRa受信データの格納先
 * @return 0:成功, 1:失敗
 */
int LoRa_ReceiveFrame(RecvFrameE220900T22SJP_t *recv_frame);

/**
 * @brief LoRa送信を行う
 * @param config LoRa設定値
 * @param send_data 送信データ
 * @param size 送信データサイズ
 * @return 0:成功, 1:失敗
 */
int LoRa_SendFrame(LoRaConfigItem_t *config, uint8_t *send_data, int size);

/**
 * @brief 各種動作モードへ移行する関数群
 */
void LoRa_SwitchToNormalMode(void);
void LoRa_SwitchToWORSendingMode(void);
void LoRa_SwitchToWORReceivingMode(void);
void LoRa_SwitchToConfigurationMode(void);

/**
 * @brief UARTの割り込みコールバック関数
 * @note FSPコンフィギュレータで設定した名前と一致させる必要があります
 */
void lora_uart_callback(uart_callback_args_t *p_args);
