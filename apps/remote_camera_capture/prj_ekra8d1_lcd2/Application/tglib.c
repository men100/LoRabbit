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
 * tglib.c  Tiny graphics library
 */

#include "tglib.h"

/*--------------------------------------------------------------------------------------------*/
/*
 * Semaphore for exclusive control of library API
 */
LOCAL ID	tlib_semid;
LOCAL T_CSEM	csem = {
	.isemcnt = 1,
	.maxsem  = 1,
	.sematr  = TA_TFIFO | TA_FIRST,
};
#define TLIBLCD_LOCK     tk_wai_sem(tlib_semid, 1, TMO_FEVR);
#define TLIBLCD_UNLOCK   tk_sig_sem(tlib_semid, 1);

/*
 * local definition
 */
LOCAL UH	max_x, max_y;
LOCAL UW	buffer_size, hstride;
LOCAL UW	*p_frame_buffer;

LOCAL void draw_rect (UW color, UW posX, UW posY, UW width, UW height);

/*--------------------------------------------------------------------------------------------*/
/*
 * LCD Library API
 */

 /*
 *  Initialize library
 */
ER tglib_init(void)
{
	fsp_err_t	err	= FSP_SUCCESS;

	/* Initialize GLCDC module */
	err = R_GLCDC_Open(&g_display_ctrl, &g_display_cfg);
	if(FSP_SUCCESS != err) return E_IO;

	/* Initialize LCD Controller (mipi_dsi_ep.c) */
	mipi_dsi_push_table(g_lcd_init_focuslcd);

	/* Get LCDC configuration */
	max_x = (g_display_cfg.input[0].hsize);
	max_y = (g_display_cfg.input[0].vsize);
	hstride = (g_display_cfg.input[0].hstride);

	/* Initialize buffer pointers */
	buffer_size = (UW) (max_x * max_y * BYTES_PER_PIXEL);
	p_frame_buffer = (UW*) g_display_cfg.input[0].p_base;

	err = R_GLCDC_BufferChange (&g_display_ctrl, (uint8_t*) p_frame_buffer, DISPLAY_FRAME_LAYER_1);
	if(FSP_SUCCESS != err) return E_IO;

	tlib_semid = tk_cre_sem(&csem);
	return(tlib_semid > E_OK? E_OK: (ER)tlib_semid);
}

/* 
 * Get display size - Height
 */
UW tglib_get_height(void)
{
	return max_y;
}

/* 
 * Get display size - Width
 */
UW tglib_get_width(void)
{
	return max_x;
}

/*
 * LCD ON/OFF
*/
ER tglib_onoff_lcd(UINT status)
{
	TLIBLCD_LOCK

	fsp_err_t	err = FSP_SUCCESS;

	if(status == LCD_ON) {
		err = R_GLCDC_Start(&g_display_ctrl);
		if(FSP_SUCCESS != err) goto L_RETURN;
		draw_rect (WHITE, 0, 0, max_x, max_y);
	} else {
		draw_rect (BLACK, 0, 0, max_x, max_y);
		err = R_GLCDC_Stop(&g_display_ctrl);
	}

L_RETURN:
	TLIBLCD_UNLOCK
	return (FSP_SUCCESS != err)? E_OK: E_IO;
}

/*
 * Clear Screen
*/
void tglib_clear_scr(UW color)
{
	TLIBLCD_LOCK
	draw_rect (color, 0, 0, max_x, max_y);
	TLIBLCD_UNLOCK
}

/*
 * Draw a rectangle
 */
void tglib_draw_rect(UW color, UW posX, UW posY, UW width, UW height)
{
	if((posX >= max_x)||(posY >= max_y)) return;

	if(max_x <= (posX + width)) width -= (posX + width)-max_x;
	if(max_y <= (posY + height)) height -= (posY + height)-max_y;

	TLIBLCD_LOCK
	draw_rect (color, posX, posY, width, height);
	TLIBLCD_UNLOCK
}

/*--------------------------------------------------------------------------------------------*/
/* 
 * Library local functions
 */
/* 
 * Draw a rectangle
 */
LOCAL void draw_rect (UW color, UW posX, UW posY, UW width, UW height)
{
	UW xx = ( max_x <= (posX + width) ) ? max_x : (posX + width);
	UW yy = ( max_y <= (posY + height) ) ? max_y : (posY + height);
	
	UW *p_fb = p_frame_buffer + ( hstride * posY );
	
	for (UW y = posY; y < yy; y++)
	{
		for (UW x = posX; x < xx; x ++)
		{
			p_fb[x] = color;
		}
		p_fb += hstride;
	}
}

