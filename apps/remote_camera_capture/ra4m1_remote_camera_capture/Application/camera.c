#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <ArducamCamera.h>
#include "camera.h"
#include "hal_data.h"

#define LOG(...) tm_printf((UB*)__VA_ARGS__)

// 画像サイズの定義
#define SRC_WIDTH   96
#define SRC_HEIGHT  95
#define DST_WIDTH   32
#define DST_HEIGHT  32

// Camera インスタンス
ArducamCamera myCamera;

// SPI Chip Select Pin
const int spi_cs_pin = P413_SPI0_CS;

// 縮小後の画像を格納するバッファ (32x32ピクセル x 2バイト/ピクセル = 2048バイト)
uint16_t resized_image[DST_WIDTH * DST_HEIGHT];

// カメラから読み出すための一時バッファ
uint8_t read_buffer[255];

void camera_init(void) {
    // カメラライブラリを初期化
    arducamCameraInit(&myCamera, spi_cs_pin);

    if (myCamera.arducamCameraOp->begin(&myCamera) != CAM_ERR_SUCCESS) {
        LOG("ERROR: Camera initialization failed.\n");
        while(1); // 初期化に失敗したら停止
    }
    LOG("Camera initialization successful.\n");

    // 動作確認として Sensor ID を表示
    LOG("Sensor ID: 0x%02X (Expected: 0x81 for 5MP, 0x82 for 3MP (legacy models)\n", myCamera.cameraId);
}


void camera_take_picture(void) {
    // 実際には 96x95 のキャプチャが行われる模様
    myCamera.arducamCameraOp->takePicture(&myCamera, CAM_IMAGE_MODE_96X96, CAM_IMAGE_PIX_FMT_RGB565);

    // 撮影した画像の合計サイズを取得
    uint32_t total_len = myCamera.totalLength;
    if (total_len == 0) {
        LOG("ERROR: Capture failed or image size is zero.\n");
        while(1); // 撮影に失敗したら停止
    }
    LOG("Image capture successful. Total size: %lu bytes\n", total_len);

    // 縮小比率の計算
    const float x_ratio = (float)SRC_WIDTH / DST_WIDTH;
    const float y_ratio = (float)SRC_HEIGHT / DST_HEIGHT;

    // 現在読み込み済みのソース画像のY座標
    int current_src_y = -1;
    // ソース画像の一行分を保持するバッファ (96ピクセル x 2バイト/ピクセル = 192バイト)
    uint16_t source_line_buffer[SRC_WIDTH];

    // 縮小後の画像の各行を生成するループ
    for (int dst_y = 0; dst_y < DST_HEIGHT; dst_y++) {
        // この行の生成に必要なソース画像の行番号を計算
        int target_src_y = (int)floorf(dst_y * y_ratio);

        // 必要なソース行までデータを読み進める (既に読み込んでいる場合はスキップ)
        while (current_src_y < target_src_y) {
            // 不要な行データを読み飛ばす
            uint32_t bytes_to_skip = SRC_WIDTH * 2;
            while (bytes_to_skip > 0) {
                uint32_t len = (bytes_to_skip > sizeof(read_buffer)) ? sizeof(read_buffer) : bytes_to_skip;
                readBuff(&myCamera, read_buffer, len);
                bytes_to_skip -= len;
            }
            current_src_y++;
        }

        // 目的のソース行(192バイト)を source_line_buffer に読み込む
        uint32_t bytes_to_read = SRC_WIDTH * 2;
        uint32_t line_buffer_idx = 0;
        while (bytes_to_read > 0) {
            uint32_t len = (bytes_to_read > sizeof(read_buffer)) ? sizeof(read_buffer) : bytes_to_read;
            readBuff(&myCamera, read_buffer, len);
            // 読み込んだデータを16bit(uint16_t)単位でコピー
            for (uint32_t i = 0; i < len / 2; i++) {
                source_line_buffer[line_buffer_idx++] = (read_buffer[i*2+1] << 8) | read_buffer[i*2];
            }
            bytes_to_read -= len;
        }
        current_src_y++;

        // 読み込んだソース行からピクセルを抽出し、縮小後の行を生成
        for (int dst_x = 0; dst_x < DST_WIDTH; dst_x++) {
            int src_x = (int)floorf(dst_x * x_ratio);
            resized_image[dst_y * DST_WIDTH + dst_x] = source_line_buffer[src_x];
        }
        LOG("Resized row %d/%d\n", dst_y + 1, DST_HEIGHT);
    }

    LOG("Resize process finished!\n");
    LOG("The 'resized_image' buffer now contains the 32x32 image.\n");
}
