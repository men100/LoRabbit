/*
 *	EK-RA8D1 LCD Example for μT-Kernel 3.0 BSP2
 *
 *    Copyright (C) 2025 by Ken Sakamura.
 *    This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2025/06.
 */
/*
 * tglib.h  Tiny graphics library header file
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

/* 
 * Tiny graphics Library API
 */
ER tglib_init(void);
UW tglib_get_width(void);
UW tglib_get_height(void);

ER tglib_onoff_lcd(UINT status);
#define LCD_ON    1
#define LCD_OFF   0

void tglib_clear_scr(UW color);
void tglib_draw_rect(UW color, UW posX, UW posY, UW width, UW height);
#define TLIBLCD_COLOR_BLUE	(0x000000FF)
#define TLIBLCD_COLOR_RED	(0x00FF0000)
#define TLIBLCD_COLOR_GREEN	(0x0000FF00)
#define TLIBLCD_COLOR_BLACK	(0x00000000)
#define TLIBLCD_COLOR_WHITE	(0xFFFFFFFF)

#endif /* MTK3BSP2_TGLIG_H */