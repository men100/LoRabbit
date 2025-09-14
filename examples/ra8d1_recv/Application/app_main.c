#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <stdio.h>
#include <SecureLoRa-TK.h>

// FSPで生成されたUARTインスタンス
extern const uart_instance_t g_uart2;

// アプリケーションでLoRaハンドルの実体を定義
static LoraHandle_t s_lora_handle;

void g_uart2_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRa_UartCallbackHandler(&s_lora_handle, p_args);
}

void g_irq1_callback(external_irq_callback_args_t *p_args) {
    LoRa_AuxCallbackHandler(&s_lora_handle, p_args);
}

// LoRa設定値
LoRaConfigItem_t s_config = {
    .own_address              = 0x0000,  // own_address 0
    .baud_rate                = 0b011,   // baud_rate 9600 bps
    .air_data_rate            = 0b10000, // air_data_rate SF:9 BW:125
    .subpacket_size           = 0b00,    // subpacket_size 200
    .rssi_ambient_noise_flag  = 0b1,     // rssi_ambient_noise_flag 有効
    .transmission_pause_flag  = 0b0,     // transmission_pause_flag 有効
    .transmitting_power       = 0b01,    // transmitting_power 13 dBm
    .own_channel              = 0x00,    // own_channel 0
    .rssi_byte_flag           = 0b1,     // rssi_byte_flag 有効
    .transmission_method_type = 0b1,     // transmission_method_type 固定送信モード
    .lbt_flag                 = 0b0,     // lbt_flag 有効
    .wor_cycle                = 0b011,   // wor_cycle 2000 ms
    .encryption_key           = 0x0000,  // encryption_key 0
    .target_address           = 0x0000,  // target_address 0
    .target_channel           = 0x00     // target_channel 0
};

LOCAL void task_1(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_1;			// Task ID number
LOCAL T_CTSK ctsk_1 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_1,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_2(INT stacd, void *exinf);	// task execution function
LOCAL ID	tskid_2;			// Task ID number
LOCAL T_CTSK ctsk_2 = {				// Task creation information
	.itskpri	= 10,
	.stksz		= 1024,
	.task		= task_2,
	.tskatr		= TA_HLNG | TA_RNG3,
};

LOCAL void task_1(INT stacd, void *exinf)
{
	while(1) {
		tm_printf((UB*)"task 1\n");

		/* Inverts the LED on the board. */
		out_h(PORT_PODR(6), in_h(PORT_PODR(6))^(1<<0));     // BLUE
		out_h(PORT_PODR(4), in_h(PORT_PODR(4))^(1<<14));    // GREEN
		out_h(PORT_PODR(1), in_h(PORT_PODR(1))^(1<<7));     // RED

		tk_dly_tsk(1000);
	}
}

LOCAL void task_2(INT stacd, void *exinf)
{
    RecvFrameE220900T22SJP_t recv_frame;

	while(1) {
		tm_printf((UB*)"task 2\n");

	    // 通常モードに移行して受信開始
	    tm_putstring((UB*)"Switching to Normal Mode.\n");
	    LoRa_SwitchToNormalMode(&s_lora_handle);

	    if (LoRa_ReceiveFrame(&s_lora_handle, &recv_frame) == 0) {
	        tm_printf((UB*)"Received! Length: %d, RSSI: %d dBm\n", recv_frame.recv_data_len, recv_frame.rssi);
	        tm_printf((UB*)"Data: %.*s\n", recv_frame.recv_data_len, recv_frame.recv_data);
	    }
	}
}

/* usermain関数 */
EXPORT INT usermain(void)
{
	tm_putstring((UB*)"Start User-main program.\n");

	/* Turn off the LED on the board. */
	out_h(PORT_PODR(6), in_h(PORT_PODR(6))&~(1<<0));    // BLUE
	out_h(PORT_PODR(4), in_h(PORT_PODR(4))&~(1<<14));   // GREEN
	out_h(PORT_PODR(1), in_h(PORT_PODR(1))&~(1<<7));    // RED

	tm_putstring((UB*)"LoRa recv test222\n");

    // ハードウェア構成を定義
    LoraHwConfig_t lora_hw_config = {
        .p_uart = &g_uart2,      // FSPで生成されたUARTインスタンス
        .m0     = PMOD2_9_GPIO,  // FSPで定義したM0ピン
        .m1     = PMOD2_10_GPIO, // FSPで定義したM1ピン
        .aux    = PMOD2_7_INT    // FSPで定義したAUXピン
    };

    // LoRaライブラリを初期化
    LoRa_Init(&s_lora_handle, &lora_hw_config);

    // LoRaモジュールを初期化
    if (LoRa_InitModule(&s_lora_handle, &s_config) != 0) {
        tm_putstring((UB*)"LoRa Init Failed!\n");
        R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED3_RED, BSP_IO_LEVEL_HIGH);
        while(1);
    }
    tm_putstring((UB*)"LoRa Init Success!\n");

	/* Create & Start Tasks */
	tskid_1 = tk_cre_tsk(&ctsk_1);
	tk_sta_tsk(tskid_1, 0);

	tskid_2 = tk_cre_tsk(&ctsk_2);
	tk_sta_tsk(tskid_2, 0);

	tk_slp_tsk(TMO_FEVR);

	return 0;
}
