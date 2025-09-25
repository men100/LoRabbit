#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <LoRabbit.h>
#include "tglib.h"
#include "r_sci_b_uart.h"

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart2;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

void g_uart2_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_UartCallbackHandler(&s_lora_handle, p_args);
}

void g_irq1_callback(external_irq_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_AuxCallbackHandler(&s_lora_handle, p_args);
}

int my_sci_b_uart_baud_set_helper(LoraHandle_t *p_handle, uint32_t baudrate) {
    fsp_err_t err = FSP_SUCCESS;
    uart_instance_t const *p_uart = p_handle->hw_config.p_uart;

    // ドライバ固有のBaudCalculate関数を呼び出す
    sci_b_baud_setting_t baud_setting;
    memset(&baud_setting, 0x0, sizeof(sci_b_baud_setting_t));
    err = R_SCI_B_UART_BaudCalculate(baudrate,
                                     false, // Bitrate Modulation 無効
                                     5000,  // 許容エラー率 (5%)
                                     &baud_setting);
    if (FSP_SUCCESS != err) {
        tm_printf((UB*)"R_SCI_B_UART_BaudCalculate failed\n");
        // 計算失敗
        return -1;
    }

    // mddr (上位8bit) が 0x80 になっているが、Bitrate Modulation (brme) が 0 になっているので無視される
    tm_printf((UB*)"baudrate=%dbps, baud_setting.baudrate_bits: 0x%08x\n",
            baudrate, baud_setting.baudrate_bits);

    // 計算結果を void* にキャストして、抽象APIである baudSet に渡す
    err = p_uart->p_api->baudSet(p_uart->p_ctrl, (void*)&baud_setting);

    return (FSP_SUCCESS == err) ? 0 : -1;
}

// LoRa設定値
LoraConfigItem_t s_config = {
    .own_address              = 0x0000,
    .baud_rate                = LORA_UART_BAUD_RATE_115200_BPS,
    .air_data_rate            = LORA_AIR_DATA_RATE_1758_BPS_SF_9_BW_125,
    .payload_size             = LORA_PAYLOAD_SIZE_200_BYTE,
    .rssi_ambient_noise_flag  = LORA_FLAG_ENABLED,
    .transmitting_power       = LORA_TRANSMITTING_POWER_13_DBM,
    .own_channel              = 0,
    .rssi_byte_flag           = LORA_FLAG_ENABLED,
    .transmission_method_type = LORA_TRANSMISSION_METHOD_TYPE_FIXED,
    .wor_cycle                = LORA_WOR_CYCLE_2000_MS,
    .encryption_key           = 0x0000,
};

LOCAL void task_1(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_1;			// Task ID number
LOCAL T_CTSK ctsk_1 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_1,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_1(INT stacd, void *exinf)
{
	UINT	x, y, w, h, dw, dh;

	tm_printf((UB*)"Start task-1\n");

	tglib_init();				/* Init LCD */
	tglib_onoff_lcd(LCD_ON);		/* LCD ON */

	/* Get display size */
	w = tglib_get_width();
	h = tglib_get_height();
	tm_printf((UB*)"Width:%d  Height:%d\n", w, h);

	while(1) {
		/* Cleare Screen */
		tglib_clear_scr(TLIBLCD_COLOR_RED);
		tk_dly_tsk(500);
		tglib_clear_scr(TLIBLCD_COLOR_GREEN);
		tk_dly_tsk(500);
		tglib_clear_scr(TLIBLCD_COLOR_BLUE);
		tk_dly_tsk(500);
		tglib_clear_scr(TLIBLCD_COLOR_WHITE);
		tk_dly_tsk(500);

		/* Draw a color bar */
		x = y = 0; dw = w/10; dh = h/10;
		while(y<h) {
			tglib_draw_rect(TLIBLCD_COLOR_RED, x, y, w, dh);
			y += dh;
			tglib_draw_rect(TLIBLCD_COLOR_GREEN, x, y, w, dh);
			y += dh;
			tglib_draw_rect(TLIBLCD_COLOR_BLUE, x, y, w, dh);
			y += dh;
			tglib_draw_rect(TLIBLCD_COLOR_WHITE, x, y, w, dh);
			y += dh;
			tglib_draw_rect(TLIBLCD_COLOR_BLACK, x, y, w, dh);
			y += dh;
		}

		/* Draw a rectangle */
		x = w/2 - dw/2; y = h/2 - dh/2; 
		UINT ww = dw; UINT hh = dh;
		for(UINT i = 0; i<2; i++) {
			tglib_draw_rect(TLIBLCD_COLOR_RED, x, y, ww, hh);
			x -= dw/2; y -= dh/2; ww += dw; hh += dh;
			tk_dly_tsk(200);

			tglib_draw_rect(TLIBLCD_COLOR_GREEN, x, y, ww, hh);
			x -= dw/2; y -= dh/2; ww += dw; hh += dh;
			tk_dly_tsk(200);

			tglib_draw_rect(TLIBLCD_COLOR_BLUE, x, y, ww, hh);
			x -= dw/2; y -= dh/2; ww += dw; hh += dh;
			tk_dly_tsk(200);

			tglib_draw_rect(TLIBLCD_COLOR_WHITE, x, y, ww, hh);
			x -= dw/2; y -= dh/2; ww += dw; hh += dh;
			tk_dly_tsk(200);
		}

		tk_dly_tsk(300);
	}
}

/* usermain関数 */
EXPORT INT usermain(void)
{
	tm_putstring((UB*)"Start User-main program.\n");

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart2,      // FSPで生成されたUARTインスタンス
        .m0     = PMOD2_9_GPIO,  // FSPで定義したM0ピン
        .m1     = PMOD2_10_GPIO, // FSPで定義したM1ピン
        .aux    = PMOD2_7_INT,   // FSPで定義したAUXピン

        // baudrate 変更の際のヘルパ関数を登録
        .pf_baud_set_helper = my_sci_b_uart_baud_set_helper,
    };

    // LoRaライブラリを初期化
    LoRabbit_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRabbit_InitModule(&s_lora_handle, &s_config) != 0) {
        tm_putstring((UB*)"LoRa Init Failed!\n");
        R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED3_RED, BSP_IO_LEVEL_HIGH);
        while(1);
    }
    tm_putstring((UB*)"LoRa Init Success!\n");

	/* Create & Start Tasks */
	tskid_1 = tk_cre_tsk(&ctsk_1);
	tk_sta_tsk(tskid_1, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
