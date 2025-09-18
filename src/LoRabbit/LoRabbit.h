#pragma once

#include "hal_data.h"
#include <stdint.h>
#include <tk/tkernel.h>

#define LORA_PIN_UNDEFINED (BSP_IO_PORT_FF_PIN_FF)

typedef enum {
    LORA_FLAG_DISABLED = 0b0,
    LORA_FLAG_ENABLED  = 0b1,
} LoraFlag_t;

typedef enum {
    LORA_UART_BAUD_RATE_1200_BPS   = 0b000, //   1,200bps
    LORA_UART_BAUD_RATE_2400_BPS   = 0b001, //   2,400bps
    LORA_UART_BAUD_RATE_4800_BPS   = 0b010, //   4,800bps
    LORA_UART_BAUD_RATE_9600_BPS   = 0b011, //   9,600bps (default)
    LORA_UART_BAUD_RATE_19200_BPS  = 0b100, //  19,200bps
    LORA_UART_BAUD_RATE_38400_BPS  = 0b101, //  38,400bps
    LORA_UART_BAUD_RATE_57600_BPS  = 0b110, //  57,600bps
    LORA_UART_BAUD_RATE_115200_BPS = 0b111, // 115,200bps
} LoraUartBaudRate_t;

typedef enum {
    LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125  = 0b00000, // 15,625bps, SF=5,  BW=125kHz
    LORA_AIR_DATA_RATE_9375_BPS_SF_6_BW_125   = 0b00100, //  9,375bps, SF=6,  BW=125kHz
    LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125   = 0b01000, //  5,469bps, SF=7,  BW=125kHz
    LORA_AIR_DATA_RATE_3125_BPS_SF_8_BW_125   = 0b01100, //  3,125bps, SF=8,  BW=125kHz
    LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125   = 0b10000, //  1,758bps, SF=9,  BW=125kHz (default)
    LORA_AIR_DATA_RATE_31250_BPS_SF_5_BW_250  = 0b00001, // 31,250bps, SF=5,  BW=250kHz
    LORA_AIR_DATA_RATE_18750_BPS_SF_6_BW_250  = 0b00101, // 18,750bps, SF=6,  BW=250kHz
    LORA_AIR_DATA_RATE_10938_BPS_SF_7_BW_250  = 0b01001, // 10,938bps, SF=7,  BW=250kHz
    LORA_AIR_DATA_RATE_6250_BPS_SF_8_BW_250   = 0b01101, //  6,250bps, SF=8,  BW=250kHz
    LORA_AIR_DATA_RATE_3516_BPS_SF_9_BW_250   = 0b10001, //  3,516bps, SF=9,  BW=250kHz
    LORA_AIR_DATA_RATE_1953_BPS_SF_10_BW_250  = 0b10101, //  1,953bps, SF=10, BW=250kHz
    LORA_AIR_DATA_RATE_62500_BPS_SF_5_BW_500  = 0b00010, // 62,500bps, SF=5,  BW=500kHz
    LORA_AIR_DATA_RATE_37500_BPS_SF_6_BW_500  = 0b00110, // 37,500bps, SF=6,  BW=500kHz
    LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500  = 0b01010, // 21,875bps, SF=7,  BW=500kHz
    LORA_AIR_DATA_RATE_12500_BPS_SF_8_BW_500  = 0b01110, // 12,500bps, SF=8,  BW=500kHz
    LORA_AIR_DATA_RATE_7031_BPS_SF_9_BW_500   = 0b10010, //  7,031bps, SF=9,  BW=500kHz
    LORA_AIR_DATA_RATE_3906_BPS_SF_10_BW_500  = 0b10110, //  3,906bps, SF=10, BW=500kHz
    LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500  = 0b11010, //  2,148bps, SF=11, BW=500kHz
} LoraAirDateRate_t;

typedef enum {
    LORA_PAYLOAD_SIZE_200_BYTE = 0b00, // 200byte (default)
    LORA_PAYLOAD_SIZE_128_BYTE = 0b01, // 128byte
    LORA_PAYLOAD_SIZE_64_BYTE  = 0b10, //  64byte
    LORA_PAYLOAD_SIZE_32_BYTE  = 0b11, //  32byte
} LoraPayloadSize_t;

typedef enum {
    LORA_TRANSMITTING_POWER_13_DBM = 0b01, // 13dBm (default)
    LORA_TRANSMITTING_POWER_7_DBM  = 0b10, //  7dBm
    LORA_TRANSMITTING_POWER_0_DBM  = 0b11, //  0dBm
} LoraTransmittingPower_t;

typedef enum {
    LORA_TRANSMISSION_METHOD_TYPE_TRANSPARENT = 0b0, // トランスペアレント送信モード (default)
    LORA_TRANSMISSION_METHOD_TYPE_FIXED       = 0b1, // 固定送信モード
} LoraTransmissionMethodType_t;

typedef enum {
    LORA_WOR_CYCLE_500_MS  = 0b000, //   500ms
    LORA_WOR_CYCLE_1000_MS = 0b001, // 1,000ms
    LORA_WOR_CYCLE_1500_MS = 0b010, // 1,500ms
    LORA_WOR_CYCLE_2000_MS = 0b011, // 2,000ms (default)
    LORA_WOR_CYCLE_3000_MS = 0b101, // 3,000ms
} LoraWorCycle_t;

typedef enum {
    LORA_STATE_IDLE,        // アイドル状態
    LORA_STATE_WAITING_TX,  // 送信完了(AUX High)を待っている状態
    LORA_STATE_WAITING_RX,  // 受信開始(AUX Low)を待っている状態
} LoraState_t;

struct s_LoraHandle; // LoraHandle_t の前方宣言

// baudrate 設定ヘルパー関数の型を定義
typedef int (*lora_baud_set_helper_t)(struct s_LoraHandle *p_handle, uint32_t baudrate);

// ハードウェア構成を定義する構造体
typedef struct {
    uart_instance_t const * p_uart; // UART
    bsp_io_port_pin_t       m0;     // M0
    bsp_io_port_pin_t       m1;     // M1
    bsp_io_port_pin_t       aux;    // AUX

    // baudrate を変更するヘルパー関数のポインタ
    lora_baud_set_helper_t pf_baud_set_helper;
} LoraHwConfig_t;

// E220-900T22S(JP)の設定項目
typedef struct {
  uint16_t own_address;
  LoraUartBaudRate_t baud_rate;
  LoraAirDateRate_t air_data_rate;
  LoraPayloadSize_t payload_size;
  LoraFlag_t rssi_ambient_noise_flag;
  LoraTransmittingPower_t transmitting_power;
  uint8_t own_channel;
  LoraFlag_t rssi_byte_flag;
  LoraTransmissionMethodType_t transmission_method_type;
  LoraWorCycle_t wor_cycle;
  uint16_t encryption_key;
} LoraConfigItem_t;

// LoRaハンドルの本体（すべての状態を保持）
#define LORA_RX_BUFFER_SIZE 256
typedef struct s_LoraHandle {
    LoraHwConfig_t hw_config; // ハードウェア構成

    LoraConfigItem_t current_config; // 現在の設定

    // インスタンスごとの受信リングバッファ
    volatile uint8_t rx_buffer[LORA_RX_BUFFER_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

#ifdef LORABBIT_USE_AUX_IRQ
    // 同期用セマフォID
    ID tx_done_sem_id;
    ID rx_start_sem_id;

    // 内部状態
    volatile LoraState_t state;
#endif
} LoraHandle_t;

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
