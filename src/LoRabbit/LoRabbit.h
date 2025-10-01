/**
 * @file LoRabbit.h
 * @brief LoRabbitライブラリのコアヘッダファイル
 * @details LoRaモジュールを操作するための主要な型定義、エラーコード、
 * および状態を保持するメインハンドル(LoraHandle_t)を定義します。
 * @author men100
 * @date 2025/09/30
 */
#pragma once

#include <stdint.h>
#include <tk/tkernel.h>
#include "LoRabbit_config.h"
#include "hal_data.h"

/**
 * @defgroup LoRabbitCore LoRabbit Core API
 * @brief LoRabbitライブラリの基本的な型定義と構造体
 * @{
 */

#define LORA_PIN_UNDEFINED (BSP_IO_PORT_FF_PIN_FF) /**< FSPにおいて未定義のピンを示す定数 */

/**
 * @brief 機能の有効/無効を示すフラグ
 */
typedef enum {
    LORA_FLAG_DISABLED = 0b0, /**< 無効 */
    LORA_FLAG_ENABLED  = 0b1, /**< 有効 */
} LoraFlag_t;

/**
 * @brief UARTのボーレート設定
 */
typedef enum {
    LORA_UART_BAUD_RATE_1200_BPS   = 0b000, /**< 1,200bps */
    LORA_UART_BAUD_RATE_2400_BPS   = 0b001, /**< 2,400bps */
    LORA_UART_BAUD_RATE_4800_BPS   = 0b010, /**< 4,800bps */
    LORA_UART_BAUD_RATE_9600_BPS   = 0b011, /**< 9,600bps (デフォルト) */
    LORA_UART_BAUD_RATE_19200_BPS  = 0b100, /**< 19,200bps */
    LORA_UART_BAUD_RATE_38400_BPS  = 0b101, /**< 38,400bps */
    LORA_UART_BAUD_RATE_57600_BPS  = 0b110, /**< 57,600bps */
    LORA_UART_BAUD_RATE_115200_BPS = 0b111, /**< 115,200bps */
} LoraUartBaudRate_t;

/**
 * @brief LoRaの空中データレート設定 (速度、SF、帯域幅)
 */
typedef enum {
    LORA_AIR_DATA_RATE_15625_BPS_SF_5_BW_125  = 0b00000, /**< 15,625bps, SF=5,  BW=125kHz */
    LORA_AIR_DATA_RATE_9375_BPS_SF_6_BW_125   = 0b00100, /**<  9,375bps, SF=6,  BW=125kHz */
    LORA_AIR_DATA_RATE_5469_BPS_SF_7_BW_125   = 0b01000, /**<  5,469bps, SF=7,  BW=125kHz */
    LORA_AIR_DATA_RATE_3125_BPS_SF_8_BW_125   = 0b01100, /**<  3,125bps, SF=8,  BW=125kHz */
    LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125   = 0b10000, /**<  1,758bps, SF=9,  BW=125kHz (デフォルト) */
    LORA_AIR_DATA_RATE_31250_BPS_SF_5_BW_250  = 0b00001, /**< 31,250bps, SF=5,  BW=250kHz */
    LORA_AIR_DATA_RATE_18750_BPS_SF_6_BW_250  = 0b00101, /**< 18,750bps, SF=6,  BW=250kHz */
    LORA_AIR_DATA_RATE_10938_BPS_SF_7_BW_250  = 0b01001, /**< 10,938bps, SF=7,  BW=250kHz */
    LORA_AIR_DATA_RATE_6250_BPS_SF_8_BW_250   = 0b01101, /**<  6,250bps, SF=8,  BW=250kHz */
    LORA_AIR_DATA_RATE_3516_BPS_SF_9_BW_250   = 0b10001, /**<  3,516bps, SF=9,  BW=250kHz */
    LORA_AIR_DATA_RATE_1953_BPS_SF_10_BW_250  = 0b10101, /**<  1,953bps, SF=10, BW=250kHz */
    LORA_AIR_DATA_RATE_62500_BPS_SF_5_BW_500  = 0b00010, /**< 62,500bps, SF=5,  BW=500kHz */
    LORA_AIR_DATA_RATE_37500_BPS_SF_6_BW_500  = 0b00110, /**< 37,500bps, SF=6,  BW=500kHz */
    LORA_AIR_DATA_RATE_21875_BPS_SF_7_BW_500  = 0b01010, /**< 21,875bps, SF=7,  BW=500kHz */
    LORA_AIR_DATA_RATE_12500_BPS_SF_8_BW_500  = 0b01110, /**< 12,500bps, SF=8,  BW=500kHz */
    LORA_AIR_DATA_RATE_7031_BPS_SF_9_BW_500   = 0b10010, /**<  7,031bps, SF=9,  BW=500kHz */
    LORA_AIR_DATA_RATE_3906_BPS_SF_10_BW_500  = 0b10110, /**<  3,906bps, SF=10, BW=500kHz */
    LORA_AIR_DATA_RATE_2148_BPS_SF_11_BW_500  = 0b11010, /**<  2,148bps, SF=11, BW=500kHz */
} LoraAirDateRate_t;

/**
 * @brief 1パケットあたりの最大ペイロードサイズ
 */
typedef enum {
    LORA_PAYLOAD_SIZE_200_BYTE = 0b00, /**< 200バイト (デフォルト) */
    LORA_PAYLOAD_SIZE_128_BYTE = 0b01, /**< 128バイト */
    LORA_PAYLOAD_SIZE_64_BYTE  = 0b10, /**< 64バイト */
    LORA_PAYLOAD_SIZE_32_BYTE  = 0b11, /**< 32バイト */
} LoraPayloadSize_t;

/**
 * @brief 送信電力
 */
typedef enum {
    LORA_TRANSMITTING_POWER_13_DBM = 0b01, /**< 13dBm (デフォルト) */
    LORA_TRANSMITTING_POWER_7_DBM  = 0b10, /**< 7dBm */
    LORA_TRANSMITTING_POWER_0_DBM  = 0b11, /**< 0dBm */
} LoraTransmittingPower_t;

/**
 * @brief 送信方式
 */
typedef enum {
    LORA_TRANSMISSION_METHOD_TYPE_TRANSPARENT = 0b0, /**< トランスペアレント送信モード (デフォルト) */
    LORA_TRANSMISSION_METHOD_TYPE_FIXED       = 0b1, /**< 固定送信モード */
} LoraTransmissionMethodType_t;

/**
 * @brief WOR (Wake-on-Radio) の受信周期
 */
typedef enum {
    LORA_WOR_CYCLE_500_MS  = 0b000, /**< 500ms */
    LORA_WOR_CYCLE_1000_MS = 0b001, /**< 1,000ms */
    LORA_WOR_CYCLE_1500_MS = 0b010, /**< 1,500ms */
    LORA_WOR_CYCLE_2000_MS = 0b011, /**< 2,000ms (デフォルト) */
    LORA_WOR_CYCLE_3000_MS = 0b101, /**< 3,000ms */
} LoraWorCycle_t;

/**
 * @brief AUXピン割り込み利用時の内部状態
 * @internal
 */
typedef enum {
    LORA_STATE_IDLE,        /**< アイドル状態 */
    LORA_STATE_WAITING_TX,  /**< 送信完了(AUX High)を待っている状態 */
    LORA_STATE_WAITING_RX,  /**< 受信開始(AUX Low)を待っている状態 */
} LoraState_t;

struct s_LoraHandle; // LoraHandle_t の前方宣言

/**
 * @brief MCUのボーレート設定ヘルパー関数のポインタ型
 * @internal
 */
typedef int (*lora_baud_set_helper_t)(struct s_LoraHandle *p_handle, uint32_t baudrate);

/**
 * @brief ハードウェア構成を定義する構造体
 */
typedef struct {
    uart_instance_t const * p_uart; /**< 使用するUARTのFSPインスタンス */
    bsp_io_port_pin_t       m0;     /**< M0ピン */
    bsp_io_port_pin_t       m1;     /**< M1ピン */
    bsp_io_port_pin_t       aux;    /**< AUXピン (割り込み未使用時は LORA_PIN_UNDEFINED) */
    lora_baud_set_helper_t  pf_baud_set_helper; /**< MCUのUARTボーレートを変更するためのコールバック関数 */
} LoraHwConfig_t;

/**
 * @brief LoRaモジュールの設定項目を保持する構造体
 */
typedef struct {
  uint16_t own_address;                 /**< モジュールの個別アドレス (0-65535) */
  LoraUartBaudRate_t baud_rate;         /**< UARTボーレート */
  LoraAirDateRate_t air_data_rate;      /**< 空中データレート */
  LoraPayloadSize_t payload_size;       /**< 1パケットの最大ペイロード長 */
  LoraFlag_t rssi_ambient_noise_flag;   /**< 周囲のRSSIノイズをUARTから出力するか */
  LoraTransmittingPower_t transmitting_power; /**< 送信電力 */
  uint8_t own_channel;                  /**< 通信チャンネル (0-80) */
  LoraFlag_t rssi_byte_flag;            /**< 受信パケットにRSSI値を追加するか */
  LoraTransmissionMethodType_t transmission_method_type; /**< 送信方式 */
  LoraWorCycle_t wor_cycle;             /**< WOR受信周期 */
  uint16_t encryption_key;              /**< 暗号化キー (0-65535) */
} LoraConfigItem_t;

/**
 * @brief 大容量データ転送の進捗状況を示す構造体
 */
typedef struct {
    bool     is_active;            /**< 現在、大容量送受信中か */
    uint8_t  total_packets;        /**< 今回の転送における全パケット数 */
    uint8_t  current_packet_index; /**< 現在処理中のパケット番号 (0から) */
} LoRabbit_TransferStatus_t;

/**
 * @brief 1回の通信結果を記録するログ構造体
 */
typedef struct {
    SYSTIM   timestamp;            /**< 通信開始時刻 */
    uint32_t data_size;            /**< 送信したオリジナルデータのサイズ */
    LoraAirDateRate_t air_data_rate;/**< 使用した空中データレート */
    LoraTransmittingPower_t transmitting_power; /**< 使用した送信電力 */
    bool     ack_requested;        /**< ACKを要求したか */
    bool     ack_success;          /**< 最終的な成功/失敗 */
    int8_t   last_ack_rssi;        /**< 最後に成功したACKのRSSI値 */
    uint8_t  total_retries;        /**< 全パケットの合計リトライ回数 */
} LoraCommLog_t;

/**
 * @brief LoRaモジュールの全状態を保持するメインハンドル構造体
 */
#define LORA_RX_BUFFER_SIZE 256
typedef struct s_LoraHandle {
    LoraHwConfig_t hw_config; /**< ハードウェア構成 */
    LoraConfigItem_t current_config; /**< 現在のLoRaモジュール設定 */

    volatile uint8_t rx_buffer[LORA_RX_BUFFER_SIZE]; /**< UART受信用リングバッファ */
    volatile uint16_t rx_head; /**< リングバッファの書き込み位置 */
    volatile uint16_t rx_tail; /**< リングバッファの読み出し位置 */

#ifdef LORABBIT_USE_AUX_IRQ
    ID tx_done_sem_id;  /**< 送信完了同期用セマフォID */
    ID rx_start_sem_id; /**< 受信開始同期用セマフォID */
    volatile LoraState_t state; /**< AUX割り込み利用時の内部状態 */
#endif

    volatile LoRabbit_TransferStatus_t transfer_status; /**< 大容量データ転送の進捗状況 */
    ID status_mutex_id; /**< 転送状態を保護するミューテックスID */

    ID encoder_mutex_id; /**< 圧縮処理(エンコーダ)を保護するミューテックスID */
    ID decoder_mutex_id; /**< 伸長処理(デコーダ)を保護するミューテックスID */

    ID api_mutex_id; /**< ライブラリ全体を保護するミューテックスID (現在は未使用) */

    LoraCommLog_t history[LORABBIT_HISTORY_SIZE]; /**< 通信履歴を保存するリングバッファ */
    uint8_t       history_index;   /**< 履歴バッファの現在の書き込み位置 */
    bool          history_wrapped; /**< 履歴バッファが一周したかを示すフラグ */
} LoraHandle_t;

/**
 * @brief 受信したLoRaフレームの情報を格納する構造体
 */
typedef struct {
  uint8_t recv_data[201]; /**< 受信データ本体 (最大200バイト + RSSI 1バイト) */
  uint8_t recv_data_len;  /**< 受信したペイロードの長さ */
  int rssi;               /**< 受信時のRSSI値 */
} RecvFrameE220900T22SJP_t;

/**
 * @brief LoRabbitライブラリが返すステータスコード
 */
#define E_LR_BASE (-100)
typedef enum {
    LORABBIT_OK = E_OK, /**< (0) 正常終了 */
    LORABBIT_ERROR_TIMEOUT               = E_TMOUT,         /**< (-50) タイムアウト (μT-Kernel標準) */
    LORABBIT_ERROR_INVALID_ARGUMENT      = E_LR_BASE -   0, /**< (-100) 不正な引数 */
    LORABBIT_ERROR_UNSUPPORTED           = E_LR_BASE -   1, /**< (-101) 未サポートの操作 */
    LORABBIT_ERROR_WRITE_CONFIG          = E_LR_BASE -   2, /**< (-102) モジュールへの設定書き込み失敗 */
    LORABBIT_ERROR_UART_READ_FAILED      = E_LR_BASE -   3, /**< (-103) モジュールからのUART読み出し失敗 */
    LORABBIT_ERROR_BUFFER_OVERFLOW       = E_LR_BASE -   4, /**< (-104) 提供されたバッファサイズ不足 */
    LORABBIT_ERROR_INVALID_PACKET        = E_LR_BASE -   5, /**< (-105) 不正なパケットを受信 */
    LORABBIT_ERROR_ACK_FAILED            = E_LR_BASE -   6, /**< (-106) ACK受信失敗 */
    LORABBIT_ERROR_COMPRESS_FAILED       = E_LR_BASE -   7, /**< (-107) データ圧縮失敗 */
    LORABBIT_ERROR_DECOMPRESS_FAILED     = E_LR_BASE -   8, /**< (-108) データ伸長失敗 */
    LORABBIT_ERROR_RETRY                 = E_LR_BASE -   9, /**< (-109) 内部リトライ要求 */
    LORABBIT_ERROR_AI_INFERENCE_FAILED   = E_LR_BASE - 100, /**< (-200) AIモデルの推論失敗 */
    LORABBIT_ERROR_NOT_READY_DATA_FOR_AI = E_LR_BASE - 101, /**< (-201) AI推論に必要なデータがない */
} LoRabbit_Status_t;

/**
 * @brief AIが推奨する通信パラメータを格納する構造体
 */
typedef struct {
    LoraAirDateRate_t       air_data_rate;      /**< 推奨される空中データレート */
    LoraTransmittingPower_t transmitting_power; /**< 推奨される送信電力 */
} LoRaRecommendedConfig_t;

#include "LoRabbit_hal.h"
#include "LoRabbit_tp.h"
#include "LoRabbit_util.h"
#ifdef LORABBIT_USE_AI_ADR
#include "LoRabbit_ai_adr.h"
#endif

/** @} */ // end of LoRabbitCore group
