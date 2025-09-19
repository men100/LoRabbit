#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <string.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// 静的に encoder, decoder を確保 (stack で確保すると溢れる)
heatshrink_encoder hse;
heatshrink_decoder hsd;

uint8_t compressed_buf[256];
uint8_t decompressed_buf[256];

int heatshrink_encode_and_decode(void) {
    // 圧縮したい入力データ
    const char *original_data_str = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbcccccccccccccccdddddddddddddd";
    const uint8_t *original_data = (const uint8_t *)original_data_str;
    const size_t original_data_size = strlen(original_data_str);

    LOG("Original size: %u bytes\n", original_data_size);
    LOG("Original data: \"%s\"\n", original_data_str);

    // 圧縮・伸張用のバッファをスタックに準備
    memset(compressed_buf, 0, sizeof(compressed_buf));
    memset(decompressed_buf, 0, sizeof(decompressed_buf));

    // エンコード (圧縮)
    LOG("=== Encoding ===\n");

    // reset encoder
    heatshrink_encoder_reset(&hse);

    size_t sunk_bytes = 0;
    size_t total_out_bytes = 0;

    // sink処理
    size_t count_in = 0;
    heatshrink_encoder_sink(&hse, (uint8_t *)original_data, original_data_size, &count_in);
    sunk_bytes += count_in;

    // finish & poll処理
    HSE_finish_res fres;
    do {
        fres = heatshrink_encoder_finish(&hse);
        if (fres < 0) {
            LOG("ERROR: heatshrink_encoder_finish failed(%d)\n", fres);
            return -1;
        }

        HSE_poll_res pres;
        do {
            size_t count_out = 0;
            pres = heatshrink_encoder_poll(&hse, &compressed_buf[total_out_bytes], sizeof(compressed_buf) - total_out_bytes, &count_out);
            if (pres < 0) {
                LOG("ERROR: heatshrink_encoder_poll failed(%d)\n", pres);
                return -1;
            }
            total_out_bytes += count_out;
        } while (pres == HSER_POLL_MORE);

    } while (fres == HSER_FINISH_MORE);

    size_t compressed_size = total_out_bytes;
    LOG("Compressed size: %u bytes\n", compressed_size);
    LOG("Compressed data: ");
    for (int i = 0; i < compressed_size; i++) {
        LOG("0x%02X ", compressed_buf[i]);
    }
    LOG("\n");

    //  デコード (伸張)
    LOG("=== Decoding ===\n");

    // reset decoder
    heatshrink_decoder_reset(&hsd);

    sunk_bytes = 0;
    total_out_bytes = 0;

    // sink処理
    heatshrink_decoder_sink(&hsd, compressed_buf, compressed_size, &count_in);
    sunk_bytes += count_in;

    // finish & poll処理
    HSD_finish_res dfres;
    do {
        dfres = heatshrink_decoder_finish(&hsd);
        if (dfres < 0) {
            LOG("ERROR: heatshrink_decoder_finish failed(%d)\n", fres);
            return -1;
        }

        HSD_poll_res pres;
        do {
            size_t count_out = 0;
            pres = heatshrink_decoder_poll(&hsd, &decompressed_buf[total_out_bytes], sizeof(decompressed_buf) - total_out_bytes, &count_out);
            if (pres < 0) {
                LOG("ERROR: heatshrink_decoder_poll failed(%d)\n", pres);
                return -1;
            }
            total_out_bytes += count_out;
        } while (pres == HSDR_POLL_MORE);

    } while (dfres == HSDR_FINISH_MORE);

    size_t decompressed_size = total_out_bytes;
    LOG("Decompressed size: %u bytes\n", decompressed_size);
    LOG("Decompressed data: \"%s\"\n", (char*)decompressed_buf);

    // 検証
    LOG("=== Verifying ===\n");
    if (decompressed_size == original_data_size && memcmp(original_data, decompressed_buf, original_data_size) == 0) {
        LOG("Verification successful\n");
    } else {
        LOG("ERROR: Verification failed\n");
        return -1;
    }

    return 0;
}

// usermain関数
EXPORT INT usermain(void)
{
    tm_putstring((UB*)"Start User-main program.\n");

    tm_putstring((UB*)"heatshrink encode and decode test\n");

    heatshrink_encode_and_decode();

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
