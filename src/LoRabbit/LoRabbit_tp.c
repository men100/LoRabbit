/**
 * @file LoRabbit_tp.c
 * @brief LoRabbit Transport Protocol の実装
 * @details LoRabbit_tp.hで宣言された、大容量データの分割送受信や
 * 圧縮・伸長、通信履歴管理などの高レベルAPIを実装します。
 */
#include "LoRabbit.h"
#include "LoRabbit_tp.h"
#include "LoRabbit_config.h"
#include "LoRabbit_internal.h"
#include "LoRabbit_util.h"
#include <stdio.h>
#include <string.h>
#include <tm/tmonitor.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>

// heatshrink encoder & decoder
static heatshrink_encoder s_hse;
static heatshrink_decoder s_hsd;

// 大容量伝送用の定義
#define LORABBIT_TP_HEADER_SIZE      8
#define LORABBIT_TP_MAX_PAYLOAD      (197 - LORABBIT_TP_HEADER_SIZE) // 189バイト
#define LORABBIT_TP_MAX_TOTAL_SIZE   (255 * LORABBIT_TP_MAX_PAYLOAD) // 48,195バイト

// コントロールバイトのフラグ定義
#define LORABBIT_TP_FLAG_ACK_REQUEST (1 << 7)
#define LORABBIT_TP_FLAG_IS_ACK      (1 << 6)
#define LORABBIT_TP_FLAG_EOT         (1 << 5)

// パケットヘッダ構造体（内部利用）
typedef struct {
    uint16_t source_address;
    uint8_t  source_channel;
    uint8_t  control_byte;
    uint8_t  transaction_id;
    uint8_t  total_packets;
    uint8_t  packet_index;
    uint8_t  payload_length;
} LoRabbitTP_Header_t;

// ヘッダを解析するヘルパー関数（内部利用）
static void lora_parse_header(uint8_t *raw_packet, LoRabbitTP_Header_t *p_header) {
    p_header->source_address = (raw_packet[0] << 8) | raw_packet[1];
    p_header->source_channel = raw_packet[2];
    p_header->control_byte   = raw_packet[3];
    p_header->transaction_id = raw_packet[4];
    p_header->total_packets  = raw_packet[5];
    p_header->packet_index   = raw_packet[6];
    p_header->payload_length = raw_packet[7];
}

// ACKパケットを送信するヘルパー関数（内部利用）
static int lora_send_ack(LoraHandle_t *p_handle, LoRabbitTP_Header_t *p_data_header) {
    uint8_t ack_payload[LORABBIT_TP_HEADER_SIZE];

    // ACKのヘッダを組み立てる
    ack_payload[0] = p_handle->current_config.own_address >> 8;
    ack_payload[1] = p_handle->current_config.own_address & 0xFF;
    ack_payload[2] = p_handle->current_config.own_channel;
    ack_payload[3] = LORABBIT_TP_FLAG_IS_ACK; // コントロールバイト
    ack_payload[4] = p_data_header->transaction_id;
    ack_payload[5] = p_data_header->total_packets;
    ack_payload[6] = p_data_header->packet_index;
    ack_payload[7] = 0; // ペイロード長

    // ACK を送信 (待機なし)
    return lora_send_frame_fire_and_forget_internal(p_handle,
                                                    p_data_header->source_address,
                                                    p_data_header->source_channel,
                                                    ack_payload,
                                                    sizeof(ack_payload));
}

/**
 * @brief 転送状態を「実行中」に設定するヘルパー
 */
static void lora_status_set_active(LoraHandle_t *p_handle, uint8_t total_packets, uint8_t current_index) {
    tk_wai_sem(p_handle->status_mutex_id, 1, TMO_FEVR);
    p_handle->transfer_status.is_active = true;
    p_handle->transfer_status.total_packets = total_packets;
    p_handle->transfer_status.current_packet_index = current_index;
    tk_sig_sem(p_handle->status_mutex_id, 1);
}

/**
 * @brief 転送状態の進捗を更新するヘルパー
 */
static void lora_status_set_progress(LoraHandle_t *p_handle, uint8_t current_index) {
    tk_wai_sem(p_handle->status_mutex_id, 1, TMO_FEVR);
    p_handle->transfer_status.current_packet_index = current_index;
    tk_sig_sem(p_handle->status_mutex_id, 1);
}

/**
 * @brief 転送状態を「アイドル」にリセットするヘルパー
 */
static void lora_status_set_idle(LoraHandle_t *p_handle) {
    tk_wai_sem(p_handle->status_mutex_id, 1, TMO_FEVR);
    p_handle->transfer_status.is_active = false;
    p_handle->transfer_status.total_packets = 0;
    p_handle->transfer_status.current_packet_index = 0;
    tk_sig_sem(p_handle->status_mutex_id, 1);
}

/**
 * @brief パケットを1つ受信し、期待通りのものか検証する
 * @param[in] p_handle ハンドル
 * @param[in,out] p_remaining_timeout 残りタイムアウト時間(ms)へのポインタ。関数内で消費時間を減算する。
 * @param[in] expected_index 期待するパケットインデックス
 * @param[in] transaction_id_to_match 期待するトランザクションID (最初のパケットの場合は無視される)
 * @param[out] p_out_frame 受信したフレームの格納先
 * @param[out] p_out_header パースしたヘッダの格納先
 * @retval LORABBIT_OK 期待通りのパケットを受信
 * @retval LORABBIT_ERROR_TIMEOUT タイムアウト
 * @retval LORABBIT_ERROR_RETRY 期待しないパケットを受信（リトライが必要）
 */
static ER lora_receive_and_validate_packet(
    LoraHandle_t *p_handle,
    TMO *p_remaining_timeout,
    uint8_t expected_index,
    uint8_t transaction_id_to_match,
    RecvFrameE220900T22SJP_t *p_out_frame,
    LoRabbitTP_Header_t *p_out_header)
{
    SYSTIM start_time, end_time;
    tk_get_tim(&start_time);

    int recv_len = LoRabbit_ReceiveFrame(p_handle, p_out_frame, *p_remaining_timeout);

    // タイムアウトを更新
    if (*p_remaining_timeout != TMO_FEVR) {
        tk_get_tim(&end_time);
        uint64_t start_ms = ((uint64_t)start_time.hi << 32) | start_time.lo;
        uint64_t end_ms   = ((uint64_t)end_time.hi << 32) | end_time.lo;
        if (end_ms > start_ms) {
            *p_remaining_timeout -= (TMO)(end_ms - start_ms);
        }
    }

    if (recv_len <= 0) {
        return LORABBIT_ERROR_TIMEOUT;
    }

    lora_parse_header(p_out_frame->recv_data, p_out_header);

    // パケットを検証
    bool is_valid = false;
    if (expected_index == 0) { // 最初のパケットの検証
        if (!(p_out_header->control_byte & LORABBIT_TP_FLAG_IS_ACK) &&
             p_out_header->packet_index == 0) {
            is_valid = true;
        }
    } else { // 後続パケットの検証
        if (!(p_out_header->control_byte & LORABBIT_TP_FLAG_IS_ACK) &&
            (p_out_header->transaction_id == transaction_id_to_match) &&
            (p_out_header->packet_index == expected_index)) {
            is_valid = true;
        }
    }

    return is_valid ? LORABBIT_OK : LORABBIT_ERROR_RETRY;
}

/**
 * @brief ハンドル内のリングバッファに新しい通信ログを追加する
 */
static void lora_add_log_to_history(LoraHandle_t *p_handle, const LoraCommLog_t *p_log) {
    memcpy(&p_handle->history[p_handle->history_index], p_log, sizeof(LoraCommLog_t));
    p_handle->history_index++;
    if (p_handle->history_index >= LORABBIT_HISTORY_SIZE) {
        p_handle->history_index = 0;
        p_handle->history_wrapped = true;
    }
}

static const char* lora_tx_power_to_string(LoraTransmittingPower_t power) {
    switch (power) {
        case LORA_TRANSMITTING_POWER_13_DBM: return "13dBm";
        case LORA_TRANSMITTING_POWER_7_DBM:  return "7dBm";
        case LORA_TRANSMITTING_POWER_0_DBM:  return "0dBm";
        default: return "Unknown";
    }
}

// =====================================

int LoRabbit_SendData(LoraHandle_t *p_handle,
                           uint16_t target_address,
                           uint8_t target_channel,
                           uint8_t *p_data,
                           uint32_t size,
                           bool request_ack)
{
    // ログ構造体を準備し、送信前パラメータを記録
    LoraCommLog_t new_log;
    memset(&new_log, 0, sizeof(new_log));
    tk_get_tim(&new_log.timestamp);
    new_log.data_size = size;
    new_log.air_data_rate = p_handle->current_config.air_data_rate;
    new_log.transmitting_power = p_handle->current_config.transmitting_power;
    new_log.ack_requested = request_ack;

    if (size > LORABBIT_TP_MAX_TOTAL_SIZE) {
        return LORABBIT_ERROR_INVALID_ARGUMENT; // サイズ超過
    }

    static uint8_t transaction_id_counter = 0;
    const uint8_t transaction_id = transaction_id_counter++;
    const uint8_t total_packets = (size + LORABBIT_TP_MAX_PAYLOAD - 1) / LORABBIT_TP_MAX_PAYLOAD;

    uint8_t packet_buffer[197];
    uint32_t sent_size = 0;

    // 転送開始を記録
    lora_status_set_active(p_handle, total_packets, 0);

    for (uint8_t i = 0; i < total_packets; i++) {
        // 現在のパケット番号を更新 (iが0の時も呼ばれるが、動作に支障はない)
        lora_status_set_progress(p_handle, i);

        bool ack_received = false;
        for (uint8_t retry = 0; retry < LORABBIT_TP_RETRY_COUNT; retry++) {
            if (retry > 0) {
                new_log.total_retries++; // リトライ回数をカウント
            }

            // ヘッダを組み立てる
            packet_buffer[0] = p_handle->current_config.own_address >> 8;
            packet_buffer[1] = p_handle->current_config.own_address & 0xFF;
            packet_buffer[2] = p_handle->current_config.own_channel;

            uint8_t control_byte = 0;
            if (request_ack) control_byte |= LORABBIT_TP_FLAG_ACK_REQUEST;
            if (i == total_packets - 1) control_byte |= LORABBIT_TP_FLAG_EOT;
            packet_buffer[3] = control_byte;

            packet_buffer[4] = transaction_id;
            packet_buffer[5] = total_packets;
            packet_buffer[6] = i;

            uint32_t remaining_size = size - sent_size;
            uint8_t payload_len = (remaining_size > LORABBIT_TP_MAX_PAYLOAD) ? LORABBIT_TP_MAX_PAYLOAD : remaining_size;
            packet_buffer[7] = payload_len;

            // ペイロードをコピー
            memcpy(&packet_buffer[LORABBIT_TP_HEADER_SIZE], &p_data[sent_size], payload_len);

            // 送信
            LoRabbit_SendFrame(p_handle, target_address, target_channel, packet_buffer, LORABBIT_TP_HEADER_SIZE + payload_len);

            if (!request_ack) {
                ack_received = true;
                break; // ACK不要ならリトライしない
            }

            // ACK待機
            RecvFrameE220900T22SJP_t ack_frame;
            int recv_len = LoRabbit_ReceiveFrame(p_handle, &ack_frame, LORABBIT_TP_ACK_TIMEOUT_MS);
            if (recv_len > 0) {
                LoRabbitTP_Header_t ack_header;
                lora_parse_header(ack_frame.recv_data, &ack_header);
                if ((ack_header.control_byte & LORABBIT_TP_FLAG_IS_ACK) &&
                    (ack_header.transaction_id == transaction_id) &&
                    (ack_header.packet_index == i))
                {
                    new_log.last_ack_rssi = ack_frame.rssi; // ACKのRSSIを記録
                    ack_received = true;
                    break; // 正しいACKを受信
                }
            }
        } // retry loop

        if (!ack_received) {
            // 最終結果を記録
            new_log.ack_success = false;
            lora_add_log_to_history(p_handle, &new_log);

            // 転送終了（失敗）を記録
            lora_status_set_idle(p_handle);
            return LORABBIT_ERROR_ACK_FAILED; // ACKタイムアウト
        }
        sent_size += packet_buffer[7]; // payload_len
    } // main loop

    // 最終結果を記録
    new_log.ack_success = true;
    lora_add_log_to_history(p_handle, &new_log);

    // 転送終了（成功）を記録
    lora_status_set_idle(p_handle);

    return LORABBIT_OK; // 成功
}

int LoRabbit_ReceiveData(LoraHandle_t *p_handle,
                              uint8_t *p_buffer,
                              uint32_t buffer_size,
                              uint32_t *p_received_size,
                              TMO timeout)
{
    int ret = LORABBIT_OK;
    RecvFrameE220900T22SJP_t frame;
    LoRabbitTP_Header_t header;
    uint32_t written_size = 0;

    if(p_received_size) {
        *p_received_size = 0;
    }

    // 転送開始を記録
    lora_status_set_active(p_handle, 0, 0); // この時点では総パケット数不明

    // 最初のパケットを受信するループ
    TMO remaining_timeout = timeout;
    while (remaining_timeout > 0 || timeout == TMO_FEVR) {
        ret = lora_receive_and_validate_packet(p_handle, &remaining_timeout, 0, 0, &frame, &header);
        if (ret == LORABBIT_OK) {
            break; // 成功！
        }
        if (ret != LORABBIT_ERROR_RETRY) {
            goto cleanup_and_exit; // タイムアウトか致命的エラー
        }
        // LORABBIT_ERROR_RETRY の場合はループを継続
    }
    if (ret != LORABBIT_OK) {
        goto cleanup_and_exit;
    }

    // バッファサイズをチェック
    if ((uint32_t)header.total_packets * LORABBIT_TP_MAX_PAYLOAD > buffer_size) {
        ret = LORABBIT_ERROR_BUFFER_OVERFLOW; // バッファサイズ不足
        goto cleanup_and_exit;
    }

    // 状態を更新（総パケット数）
    lora_status_set_active(p_handle, header.total_packets, 0);

    // 最初のパケットを処理
    // データをバッファにコピー
    memcpy(p_buffer, &frame.recv_data[LORABBIT_TP_HEADER_SIZE], header.payload_length);

    // 合計サイズを更新
    written_size += header.payload_length;

    // ACK要求があれば返信する
    if (header.control_byte & LORABBIT_TP_FLAG_ACK_REQUEST) {
        lora_send_ack(p_handle, &header);
    }

    // 伝送完了チェック
    if (header.control_byte & LORABBIT_TP_FLAG_EOT) {
        if(p_received_size) {
            *p_received_size = written_size;
        }
        ret = LORABBIT_OK; // 1パケットで完了
        goto cleanup_and_exit;
    }

    // 後続パケットを受信するループ
    for (uint8_t expected_index = 1; expected_index < header.total_packets; expected_index++) {
        lora_status_set_progress(p_handle, expected_index);
        remaining_timeout = LORABBIT_TP_ACK_TIMEOUT_MS;

        while (remaining_timeout > 0) {
            ret = lora_receive_and_validate_packet(p_handle,
                                                   &remaining_timeout,
                                                   expected_index,
                                                   header.transaction_id,
                                                   &frame,
                                                   &header);
            if (ret == LORABBIT_OK) {
                break; // 成功！
            }
            if (ret != LORABBIT_ERROR_RETRY) {
                goto cleanup_and_exit; // タイムアウトか致命的エラー
            }
            // LORABBIT_ERROR_RETRY の場合はループを継続
        }
        if (ret != LORABBIT_OK) {
            goto cleanup_and_exit;
        }

        // データをバッファにコピー
        memcpy(&p_buffer[written_size],
               &frame.recv_data[LORABBIT_TP_HEADER_SIZE],
               header.payload_length);

        // 合計サイズを更新
        written_size += header.payload_length;

        // ACK要求があれば返信する
        if (header.control_byte & LORABBIT_TP_FLAG_ACK_REQUEST) {
            lora_send_ack(p_handle, &header);
        }

        // EOTフラグの検証
        bool is_last_packet = (expected_index == header.total_packets - 1);
        bool has_eot_flag = (header.control_byte & LORABBIT_TP_FLAG_EOT);

        if (is_last_packet && !has_eot_flag) {
            // 最後のパケットのはずなのにEOTフラグがない -> プロトコルエラー
            ret = LORABBIT_ERROR_INVALID_PACKET;
            goto cleanup_and_exit;
        }

        // MEMO: ここのチェックは厳密すぎるかも
#if 0
        if (!is_last_packet && has_eot_flag) {
            // 最後ではないのにEOTフラグがある -> プロトコルエラー
            ret = LORABBIT_ERROR_INVALID_PACKET;
            goto cleanup_and_exit;
        }
#endif
    }

    if (p_received_size) {
        *p_received_size = written_size;
    }

cleanup_and_exit:
    lora_status_set_idle(p_handle);
    return ret;
}

// TODO: heatshrink のエラーコードチェック
int LoRabbit_SendCompressedData(LoraHandle_t *p_handle,
                                uint16_t target_address,
                                uint8_t target_channel,
                                uint8_t *p_data,
                                uint32_t size,
                                bool request_ack,
                                uint8_t *p_work_buffer,
                                uint32_t work_buffer_size)
{
    // エンコーダ用ミューテックスをロック
    ER err = tk_wai_sem(p_handle->encoder_mutex_id, 1, TMO_FEVR);
    if (err != LORABBIT_OK) {
        LORA_PRINTF("LoRabbit_SendCompressedData: tk_wai_sem failed(%d)\n", err);
        return err;
    }

    // ワークバッファのサイズが十分かチェック
    // (heatshrinkは稀にデータサイズが増えるため、少しマージンを見るのが安全)
    if (work_buffer_size < size) {
        return LORABBIT_ERROR_BUFFER_OVERFLOW;
    }

    uint32_t compressed_size = 0;

    // 圧縮処理
    heatshrink_encoder_reset(&s_hse);

    size_t total_sunk = 0;
    size_t total_polled = 0;

    while (total_sunk < size) {
        size_t sunk_count = 0;

        // 入力データをエンコーダに渡す
        heatshrink_encoder_sink(&s_hse, &p_data[total_sunk], size - total_sunk, &sunk_count);
        total_sunk += sunk_count;

        // 圧縮されたデータを出力バッファに取り出す
        HSE_poll_res pres;
        do {
            size_t polled_count = 0;

            // p_work_buffer に書き出す
            pres = heatshrink_encoder_poll(&s_hse, &p_work_buffer[total_polled], work_buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        } while (pres == HSER_POLL_MORE);
    }

    // 最後のデータを強制的に出力
    HSE_finish_res fres;
    do {
        size_t polled_count = 0;
        fres = heatshrink_encoder_finish(&s_hse);
        if (fres == HSER_FINISH_MORE) {
            heatshrink_encoder_poll(&s_hse, &p_work_buffer[total_polled], work_buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        }
    } while (fres == HSER_FINISH_MORE);

    // 最終的な圧縮サイズを更新
    compressed_size = total_polled;

    LORA_PRINTF("Original size: %lu, Compressed size: %lu\n", size, compressed_size);

    // エンコーダ用ミューテックスをアンロック
    tk_sig_sem(p_handle->encoder_mutex_id, 1);

    // 圧縮したデータを、既存の大容量伝送関数で送信
    return LoRabbit_SendData(p_handle,
                             target_address,
                             target_channel,
                             p_work_buffer,   // 圧縮済みデータを渡す
                             compressed_size, // 圧縮後のサイズを渡す
                             request_ack);
}

// TODO: heatshrink のエラーコードチェック
int LoRabbit_ReceiveCompressedData(LoraHandle_t *p_handle,
                                   uint8_t *p_buffer,
                                   uint32_t buffer_size,
                                   uint32_t *p_received_size,
                                   TMO timeout,
                                   uint8_t *p_work_buffer,
                                   uint32_t work_buffer_size)
{
    // デコーダ用ミューテックスをロック
    ER err = tk_wai_sem(p_handle->decoder_mutex_id, 1, TMO_FEVR);
    if (err != LORABBIT_OK) {
        return err;
    }

    uint32_t compressed_size = 0;

    // 既存の大容量受信関数で、圧縮されたデータを受信する
    int result = LoRabbit_ReceiveData(p_handle,
                                      p_work_buffer,
                                      work_buffer_size,
                                      &compressed_size,
                                      timeout);
    if (result != 0) {
        return result; // エラーまたはタイムアウト
    }

    if (p_received_size) {
        *p_received_size = 0;
    }

    // 伸長処理
    heatshrink_decoder_reset(&s_hsd);

    size_t total_sunk = 0;
    size_t total_polled = 0;

    while (total_sunk < compressed_size) {
        size_t sunk_count = 0;

        // 受信した圧縮データをデコーダに渡す
        heatshrink_decoder_sink(&s_hsd, &p_work_buffer[total_sunk], compressed_size - total_sunk, &sunk_count);
        total_sunk += sunk_count;

        // 伸長されたデータを出力バッファ(p_buffer)に取り出す
        HSD_poll_res pres;
        do {
            size_t polled_count = 0;
            // バッファに空きがある場合のみpollを呼ぶ
            if (total_polled < buffer_size) {
                pres = heatshrink_decoder_poll(&s_hsd, &p_buffer[total_polled], buffer_size - total_polled, &polled_count);
                total_polled += polled_count;
            } else {
                // バッファが満杯になったら、pollせずにループを抜ける
                pres = HSDR_POLL_EMPTY;
            }
        } while (pres == HSDR_POLL_MORE);
    }

    HSD_finish_res fres;
    do {
        fres = heatshrink_decoder_finish(&s_hsd);
        if (fres == HSDR_FINISH_MORE) {
            // バッファが満杯なのに、まだ出力データがある場合はオーバーフロー
            if (total_polled >= buffer_size) {
                return LORABBIT_ERROR_BUFFER_OVERFLOW;
            }
            size_t polled_count = 0;
            heatshrink_decoder_poll(&s_hsd, &p_buffer[total_polled], buffer_size - total_polled, &polled_count);
            total_polled += polled_count;
        }
    } while (fres == HSDR_FINISH_MORE);

    if (fres == HSDR_FINISH_DONE) {
        LORA_PRINTF("Compressed size: %lu, Original size: %lu\n", compressed_size, total_polled);
        if (p_received_size) {
            *p_received_size = total_polled; // 最終的な伸長サイズ
        }
        result = LORABBIT_OK; // 成功
    } else {
        result = LORABBIT_ERROR_DECOMPRESS_FAILED; // 伸長エラー
    }

    // デコーダ用ミューテックスをアンロック
    tk_sig_sem(p_handle->decoder_mutex_id, 1);

    return result;
}

int LoRabbit_GetTransferStatus(LoraHandle_t *p_handle, LoRabbit_TransferStatus_t *p_status) {
    if (NULL == p_handle || NULL == p_status) {
        return LORABBIT_ERROR_INVALID_ARGUMENT;
    }

    tk_wai_sem(p_handle->status_mutex_id, 1, TMO_FEVR);
    memcpy(p_status, (void*)&p_handle->transfer_status, sizeof(LoRabbit_TransferStatus_t));
    tk_sig_sem(p_handle->status_mutex_id, 1);

    return LORABBIT_OK;
}

void LoRabbit_DumpHistory(LoraHandle_t *p_handle) {
    if (NULL == p_handle) {
        LORA_PRINTF("p_handle is NULL.\n");
        return;
    }

    LORA_PRINTF("\n--- LoRabbit Communication History ---\n");

    // バッファが一周したかどうかに基づいて、表示するエントリ数と開始点を決定
    uint8_t num_entries = p_handle->history_wrapped ? LORABBIT_HISTORY_SIZE : p_handle->history_index;
    uint8_t start_index = p_handle->history_wrapped ? p_handle->history_index : 0;

    if (num_entries == 0) {
        LORA_PRINTF("No history yet.\n");
        return;
    }

    LORA_PRINTF("Idx  | Timestamp  | Size | Rate         | Power | ACK Req/OK | RSSI | Retries\n");
    LORA_PRINTF("-----+------------+------+--------------+-------+------------+------+--------\n");

    for (uint8_t i = 0; i < num_entries; i++) {
        uint8_t current_idx = (start_index + i) % LORABBIT_HISTORY_SIZE;
        const LoraCommLog_t *p_log = &p_handle->history[current_idx];

        int sf = get_spreading_factor_from_air_data_rate(p_log->air_data_rate);
        int bw = (int)get_bandwidth_khz_from_air_data_rate(p_log->air_data_rate);

        // 表示する際に、LORA_PRINTF側で文字列をフォーマットする
        char rate_str[16];
        snprintf(rate_str, sizeof(rate_str), "SF%d/%dkHz", sf, bw);

        LORA_PRINTF("[%2u] | %10lu | %4lu | %-12s | %5s | %d / %-6d | %4d | %d\n",
                    current_idx,
                    p_log->timestamp.lo,
                    p_log->data_size,
                    rate_str, // フォーマットした文字列を使用
                    lora_tx_power_to_string(p_log->transmitting_power),
                    p_log->ack_requested,
                    p_log->ack_success,
                    p_log->last_ack_rssi,
                    p_log->total_retries);
    }
    LORA_PRINTF("-------------------------------------\n");
}

int LoRabbit_ExportHistoryCSV(LoraHandle_t *p_handle) {
    // CSVヘッダを出力
    LORA_PRINTF("timestamp_lo,data_size,air_data_rate,transmitting_power,ack_requested,ack_success,last_ack_rssi,total_retries\n");

    // ログデータをカンマ区切りで出力
    uint8_t num_entries = p_handle->history_wrapped ? LORABBIT_HISTORY_SIZE : p_handle->history_index;
    uint8_t start_index = p_handle->history_wrapped ? p_handle->history_index : 0;

    for (uint8_t i = 0; i < num_entries; i++) {
        uint8_t current_idx = (start_index + i) % LORABBIT_HISTORY_SIZE;
        const LoraCommLog_t *p_log = &p_handle->history[current_idx];

        // enumの値は、Pythonなどで扱いやすいように整数値として出力
        LORA_PRINTF("%lu,%lu,%d,%d,%d,%d,%d,%d\n",
                    p_log->timestamp.lo,
                    p_log->data_size,
                    (int)p_log->air_data_rate,
                    (int)p_log->transmitting_power,
                    (int)p_log->ack_requested,
                    (int)p_log->ack_success,
                    (int)p_log->last_ack_rssi,
                    (int)p_log->total_retries);
    }

    return LORABBIT_OK;
}

int LoRabbit_ClearHistory(LoraHandle_t *p_handle) {
    memset(p_handle->history, 0, sizeof(p_handle->history));
    p_handle->history_index = 0;
    p_handle->history_wrapped = false;

    LORA_PRINTF("Communication history cleared.\n");

    return LORABBIT_OK;
}
