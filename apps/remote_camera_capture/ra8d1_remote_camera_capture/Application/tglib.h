/*
 *  EK-RA8D1 LCD Example for μT-Kernel 3.0 BSP2
 *
 * Copyright (C) 2025 by Ken Sakamura.
 * This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 * Released by TRON Forum(http://www.tron.org) at 2025/06.
 */
/*
 * tglib.h  Tiny graphics library header file (BGR565 without byte swap)
 */
#ifndef MTK3BSP2_TGLIG_H
#define MTK3BSP2_TGLIG_H

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "gt911.h"
#include "mipi_dsi_ep.h"
#include "r_mipi_dsi.h"
#include "hal_data.h"
#include "common_utils.h"

/* * Tiny graphics Library API
 */
ER tglib_init(void);
UW tglib_get_width(void);
UW tglib_get_height(void);

ER tglib_onoff_lcd(UINT status);
#define LCD_ON    1
#define LCD_OFF   0

void tglib_clear_scr(UH color);
void tglib_draw_rect(UH color, UW posX, UW posY, UW width, UW height);

#define FONT_WIDTH      8
#define FONT_HEIGHT     16
void tglib_draw_char(char character, UW posX, UW posY, UH color);
void tglib_draw_string(const char *str, UW posX, UW posY, UH color);
void tglib_draw_char_scaled(char character, UW posX, UW posY, UH color, UW scale);
void tglib_draw_string_scaled(const char *str, UW posX, UW posY, UH color, UW scale);

#define TLIBLCD_COLOR_BLUE  (0x001F)      // R:0, G:0, B:31
#define TLIBLCD_COLOR_RED   (0xF800)      // R:31, G:0, B:0
#define TLIBLCD_COLOR_GREEN (0x07E0)      // R:0, G:63, B:0
#define TLIBLCD_COLOR_BLACK (0x0000)
#define TLIBLCD_COLOR_WHITE (0xFFFF)

#endif /* MTK3BSP2_TGLIG_H */
