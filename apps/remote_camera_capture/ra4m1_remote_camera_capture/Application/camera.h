#pragma once

// 画像サイズの定義
#define SRC_WIDTH   96
#define SRC_HEIGHT  95
#define DST_WIDTH   32
#define DST_HEIGHT  32

// 96x95 から 32x32 に縮小されたカメラデータ (RGB565)
extern uint16_t resized_image[DST_WIDTH * DST_HEIGHT];

void camera_init(void);
void camera_take_picture(void);
