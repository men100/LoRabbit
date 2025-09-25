#include "LoRabbit.h"
#include "LoRabbit_tp.h"
#include "LoRabbit_config.h"
#include "LoRabbit_internal.h"
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

    // ACKを送信
    return LoRabbit_SendFrame(p_handle, p_data_header->source_address, p_data_header->source_channel, ack_payload, sizeof(ack_payload));
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

int LoRabbit_SendData(LoraHandle_t *p_handle,
                           uint16_t target_address,
                           uint8_t target_channel,
                           uint8_t *p_data,
                           uint32_t size,
                           bool request_ack)
{
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
                    ack_received = true;
                    break; // 正しいACKを受信
                }
            }
        } // retry loop

        if (!ack_received) {
            // 転送終了（失敗）を記録
            lora_status_set_idle(p_handle);
            return LORABBIT_ERROR_ACK_FAILED; // ACKタイムアウト
        }
        sent_size += packet_buffer[7]; // payload_len
    } // main loop

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

    // 最初のパケットを待つ
    int recv_len = LoRabbit_ReceiveFrame(p_handle, &frame, timeout);
    if (recv_len <= 0) {
        ret = (recv_len == 0) ? LORABBIT_ERROR_TIMEOUT : LORABBIT_ERROR_INVALID_PACKET; // タイムアウト or エラー
        goto cleanup_and_exit;
    }
    lora_parse_header(frame.recv_data, &header);

    // 最初のパケットが正当かチェック
    if ((header.control_byte & LORABBIT_TP_FLAG_IS_ACK) || header.packet_index != 0) {
        ret = LORABBIT_ERROR_INVALID_PACKET; // 不正なパケット
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
    memcpy(p_buffer, &frame.recv_data[LORABBIT_TP_HEADER_SIZE], header.payload_length);
    written_size += header.payload_length;
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

    // 残りのパケットを受信
    for (uint8_t expected_index = 1; expected_index < header.total_packets; expected_index++) {
        // 状態を更新（現在パケット番号）
        lora_status_set_progress(p_handle, expected_index);

        // パケット間のタイムアウトは短く設定
        recv_len = LoRabbit_ReceiveFrame(p_handle, &frame, LORABBIT_TP_ACK_TIMEOUT_MS);
        if (recv_len <= 0) {
            ret = LORABBIT_ERROR_TIMEOUT;
            goto cleanup_and_exit;
        }

        LoRabbitTP_Header_t subsequent_header;
        lora_parse_header(frame.recv_data, &subsequent_header);

        // パケットが現在のトランザクションに属し、かつ期待したインデックスか検証
        if ((subsequent_header.control_byte & LORABBIT_TP_FLAG_IS_ACK) ||
            (subsequent_header.transaction_id != header.transaction_id) ||
            (subsequent_header.packet_index != expected_index))
        {
            // 不正なパケットは無視してタイムアウトまで待機を続けることもできるが、
            // ここではシンプルにエラーとして終了
            ret = LORABBIT_ERROR_INVALID_PACKET;
            goto cleanup_and_exit;
        }

        // データをバッファにコピー
        memcpy(&p_buffer[written_size], &frame.recv_data[LORABBIT_TP_HEADER_SIZE], subsequent_header.payload_length);
        written_size += subsequent_header.payload_length;

        // ACKを返す
        if (subsequent_header.control_byte & LORABBIT_TP_FLAG_ACK_REQUEST) {
            lora_send_ack(p_handle, &subsequent_header);
        }

        // 伝送完了チェック
        if (subsequent_header.control_byte & LORABBIT_TP_FLAG_EOT) {
            if(p_received_size) {
                *p_received_size = written_size;
            }
            ret = LORABBIT_OK; // 完了
            goto cleanup_and_exit;
        }
    }

    if (p_received_size) {
        *p_received_size = written_size;
    }

cleanup_and_exit:
    // 転送終了を記録
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
