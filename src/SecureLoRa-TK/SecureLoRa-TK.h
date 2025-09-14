#pragma once

#include "hal_data.h"
#include <stdint.h>
#include <tk/tkernel.h>

#define LORA_PIN_UNDEFINED (BSP_IO_PORT_FF_PIN_FF)

// ハードウェア構成を定義する構造体
typedef struct {
    uart_instance_t const * p_uart; // UART
    bsp_io_port_pin_t       m0;     // M0
    bsp_io_port_pin_t       m1;     // M1
    bsp_io_port_pin_t       aux;    // AUX
} LoraHwConfig_t;

// LoRaハンドルの本体（すべての状態を保持）
#define LORA_RX_BUFFER_SIZE 256
typedef struct {
    LoraHwConfig_t hw_config; // ハードウェア構成

    // インスタンスごとの受信リングバッファ
    volatile uint8_t rx_buffer[LORA_RX_BUFFER_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

#ifdef SECURELORA_TK_USE_AUX_IRQ
    // 同期用セマフォID
    ID tx_done_sem_id;
#endif
} LoraHandle_t;

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
 * @brief LoRaハンドルを初期化する
 * @param p_handle 初期化対象のハンドル
 * @param p_hw_config 使用するハードウェアの構成
 * @return 0:成功, -1:失敗
 */
int LoRa_Init(LoraHandle_t *p_handle, LoraHwConfig_t const *p_hw_config);

/**
 * @brief LoRaモジュールの設定を行う
 * @param p_handle 操作対象のハンドル
 * @param p_config LoRa設定値
 * @return 0:成功, 1:失敗, -1:未初期化
 */
int LoRa_InitModule(LoraHandle_t *p_handle, LoRaConfigItem_t *p_config);

/**
 * @brief LoRa受信を行う
 * @param recv_frame LoRa受信データの格納先
 * @return 0:成功, 1:失敗
 */
int LoRa_ReceiveFrame(LoraHandle_t *p_handle, RecvFrameE220900T22SJP_t *recv_frame);

/**
 * @brief LoRa送信を行う
 * @param p_handle 操作対象のハンドル
 * @param config LoRa設定値
 * @param send_data 送信データ
 * @param size 送信データサイズ
 * @return 0:成功, 1:失敗
 */
int LoRa_SendFrame(LoraHandle_t *p_handle, LoRaConfigItem_t *p_config, uint8_t *p_send_data, int size);

/**
 * @brief 各種動作モードへ移行する関数群
 */
void LoRa_SwitchToNormalMode(LoraHandle_t *p_handle);
void LoRa_SwitchToWORSendingMode(LoraHandle_t *p_handle);
void LoRa_SwitchToWORReceivingMode(LoraHandle_t *p_handle);
void LoRa_SwitchToConfigurationMode(LoraHandle_t *p_handle);

/**
 * @brief UART割り込み時に呼び出すべきハンドラ関数
 * @note ユーザーはFSPのコールバック関数の中からこの関数を呼び出す
 * @param p_handle 該当するLoRaハンドル
 * @param p_args FSPコールバックから渡される引数
 */
void LoRa_UartCallbackHandler(LoraHandle_t *p_handle, uart_callback_args_t *p_args);

#ifdef SECURELORA_TK_USE_AUX_IRQ
/**
 * @brief LoRa AUX割り込み時に呼び出すべきハンドラ関数
 * @note ユーザーはFSPのコールバック関数の中からこの関数を呼び出す
 * @param p_handle 該当するLoRaハンドル
 * @param p_args FSPコールバックから渡される引数
 */
void LoRa_AuxCallbackHandler(LoraHandle_t *p_handle, external_irq_callback_args_t *p_args);
#endif
