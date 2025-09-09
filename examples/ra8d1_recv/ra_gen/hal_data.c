/* generated HAL source file - do not edit */
#include "hal_data.h"
sci_b_uart_instance_ctrl_t g_uart2_ctrl;

sci_b_baud_setting_t g_uart2_baud_setting =
        {
        /* Baud rate calculated with 0.160% error. */.baudrate_bits_b.abcse = 0,
          .baudrate_bits_b.abcs = 0, .baudrate_bits_b.bgdm = 1, .baudrate_bits_b.cks = 1, .baudrate_bits_b.brr = 194, .baudrate_bits_b.mddr =
                  (uint8_t) 256,
          .baudrate_bits_b.brme = false };

/** UART extended configuration for UARTonSCI HAL driver */
const sci_b_uart_extended_cfg_t g_uart2_cfg_extend =
{ .clock = SCI_B_UART_CLOCK_INT, .rx_edge_start = SCI_B_UART_START_BIT_FALLING_EDGE, .noise_cancel =
          SCI_B_UART_NOISE_CANCELLATION_DISABLE,
  .rx_fifo_trigger = SCI_B_UART_RX_FIFO_TRIGGER_MAX, .p_baud_setting = &g_uart2_baud_setting, .flow_control =
          SCI_B_UART_FLOW_CONTROL_RTS,
#if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
  .flow_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
  .rs485_setting =
  { .enable = SCI_B_UART_RS485_DISABLE,
    .polarity = SCI_B_UART_RS485_DE_POLARITY_HIGH,
    .assertion_time = 1,
    .negation_time = 1, } };

/** UART interface configuration */
const uart_cfg_t g_uart2_cfg =
{ .channel = 2, .data_bits = UART_DATA_BITS_8, .parity = UART_PARITY_OFF, .stop_bits = UART_STOP_BITS_1, .p_callback =
          g_uart2_callback,
  .p_context = NULL, .p_extend = &g_uart2_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
  .rxi_ipl = (12),
  .txi_ipl = (12), .tei_ipl = (12), .eri_ipl = (12),
#if defined(VECTOR_NUMBER_SCI2_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI2_RXI,
#else
  .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI2_TXI,
#else
  .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI2_TEI,
#else
  .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI2_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI2_ERI,
#else
  .eri_irq = FSP_INVALID_VECTOR,
#endif
        };

/* Instance structure to use this module. */
const uart_instance_t g_uart2 =
{ .p_ctrl = &g_uart2_ctrl, .p_cfg = &g_uart2_cfg, .p_api = &g_uart_on_sci_b };
iic_master_instance_ctrl_t g_i2c_master0_ctrl;
const iic_master_extended_cfg_t g_i2c_master0_extend =
{ .timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT,
  .timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED,
  .smbus_operation = 0,
  /* Actual calculated bitrate: 98945. Actual calculated duty cycle: 51%. */.clock_settings.brl_value = 15,
  .clock_settings.brh_value = 16,
  .clock_settings.cks_value = 4,
  .clock_settings.sddl_value = 0,
  .clock_settings.dlcs_value = 0, };
const i2c_master_cfg_t g_i2c_master0_cfg =
{ .channel = 1, .rate = I2C_MASTER_RATE_STANDARD, .slave = 0x00, .addr_mode = I2C_MASTER_ADDR_MODE_7BIT,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
  .p_callback = NULL,
  .p_context = NULL,
#if defined(VECTOR_NUMBER_IIC1_RXI)
    .rxi_irq             = VECTOR_NUMBER_IIC1_RXI,
#else
  .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TXI)
    .txi_irq             = VECTOR_NUMBER_IIC1_TXI,
#else
  .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TEI)
    .tei_irq             = VECTOR_NUMBER_IIC1_TEI,
#else
  .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_ERI)
    .eri_irq             = VECTOR_NUMBER_IIC1_ERI,
#else
  .eri_irq = FSP_INVALID_VECTOR,
#endif
  .ipl = (12),
  .p_extend = &g_i2c_master0_extend, };
/* Instance structure to use this module. */
const i2c_master_instance_t g_i2c_master0 =
{ .p_ctrl = &g_i2c_master0_ctrl, .p_cfg = &g_i2c_master0_cfg, .p_api = &g_i2c_master_on_iic };
/* Control structure for storing the driver's internal state. */
i3c_instance_ctrl_t g_i3c0_ctrl;

/* Extended configuration for this instance of I3C. */
const i3c_extended_cfg_t g_i3c0_cfg_extend =
        { .bitrate_settings =
        {
        /* Standard Open Drain; Actual Frequency (Hz): 1000000, Actual High Period (ns): 162.5. */
        .stdbr = ((26U << R_I3C0_STDBR_SBRHO_Pos) | (134U << R_I3C0_STDBR_SBRLO_Pos))
        /* Standard Push-Pull; Actual Frequency (Hz): 3333333.3, Actual High Period (ns): 162.5. */
        | ((26U << R_I3C0_STDBR_SBRHP_Pos) | (22U << R_I3C0_STDBR_SBRLP_Pos)) | (0U << R_I3C0_STDBR_DSBRPO_Pos),
          /* Extended Open Drain; Actual Frequency (Hz): 1000000, Actual High Period (ns): 162.5. */
          .extbr = ((26U << R_I3C0_EXTBR_EBRHO_Pos) | (134U << R_I3C0_EXTBR_EBRLO_Pos))
          /* Extended Push-Pull; Actual Frequency (Hz): 3333333.3, Actual High Period (ns): 162.5.  */
          | ((26U << R_I3C0_EXTBR_EBRHP_Pos) | (22U << R_I3C0_EXTBR_EBRLP_Pos)),

          .clock_stalling =
          { .assigned_address_phase_enable = 0,
            .transition_phase_enable = 0,
            .parity_phase_enable = 0,
            .ack_phase_enable = 0,
            .clock_stalling_time = 0, }, },
          .ibi_control.hot_join_acknowledge = 0, .ibi_control.notify_rejected_hot_join_requests = 0, .ibi_control.notify_rejected_mastership_requests =
                  0,
          .ibi_control.notify_rejected_interrupt_requests = 0, .bus_free_detection_time = 7, .bus_available_detection_time =
                  160,
          .bus_idle_detection_time = 160000, .timeout_detection_enable = false, .slave_command_response_info =
          { .inband_interrupt_enable = false,
            .mastership_request_enable = 0,
            .hotjoin_request_enable = false,
            .activity_state = I3C_ACTIVITY_STATE_ENTAS0,
            .write_length = 65535,
            .read_length = 65535,
            .ibi_payload_length = 0,
            .write_data_rate = I3C_DATA_RATE_SETTING_2MHZ,
            .read_data_rate = I3C_DATA_RATE_SETTING_2MHZ,
            .clock_data_turnaround = I3C_CLOCK_DATA_TURNAROUND_8NS,
            .read_turnaround_time_enable = false,
            .read_turnaround_time = 0,
            .oscillator_frequency = 0,
            .oscillator_inaccuracy = 0,
            .hdr_ddr_support = false,
            .hdr_tsp_support = false,
            .hdr_tsl_support = false, },
          .resp_irq = VECTOR_NUMBER_I3C0_RESPONSE, .rcv_irq = VECTOR_NUMBER_I3C0_RCV_STATUS, .rx_irq =
                  VECTOR_NUMBER_I3C0_RX,
          .tx_irq = VECTOR_NUMBER_I3C0_TX, .ibi_irq = VECTOR_NUMBER_I3C0_IBI, .eei_irq = VECTOR_NUMBER_I3C0_EEI,

          .ipl = (12),
          .eei_ipl = (12), };

/* Configuration for this instance. */
const i3c_cfg_t g_i3c0_cfg =
{ .channel = 0, .device_type = I3C_DEVICE_TYPE_MAIN_MASTER, .p_callback = g_i3c0_callback,
#if defined(NULL)
    .p_context = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend = &g_i3c0_cfg_extend, };

/* Instance structure to use this module. */
const i3c_instance_t g_i3c0 =
{ .p_ctrl = &g_i3c0_ctrl, .p_cfg = &g_i3c0_cfg, .p_api = &g_i3c_on_i3c };
adc_instance_ctrl_t g_adc0_ctrl;
const adc_extended_cfg_t g_adc0_cfg_extend =
{ .add_average_count = ADC_ADD_OFF,
  .clearing = ADC_CLEAR_AFTER_READ_ON,
  .trigger = ADC_START_SOURCE_DISABLED,
  .trigger_group_b = ADC_START_SOURCE_DISABLED,
  .double_trigger_mode = ADC_DOUBLE_TRIGGER_DISABLED,
  .adc_vref_control = ADC_VREF_CONTROL_VREFH,
  .enable_adbuf = 0,
#if defined(VECTOR_NUMBER_ADC0_WINDOW_A)
    .window_a_irq        = VECTOR_NUMBER_ADC0_WINDOW_A,
#else
  .window_a_irq = FSP_INVALID_VECTOR,
#endif
  .window_a_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_ADC0_WINDOW_B)
    .window_b_irq      = VECTOR_NUMBER_ADC0_WINDOW_B,
#else
  .window_b_irq = FSP_INVALID_VECTOR,
#endif
  .window_b_ipl = (BSP_IRQ_DISABLED), };
const adc_cfg_t g_adc0_cfg =
{ .unit = 0, .mode = ADC_MODE_SINGLE_SCAN, .resolution = ADC_RESOLUTION_12_BIT, .alignment =
          (adc_alignment_t) ADC_ALIGNMENT_RIGHT,
  .trigger = (adc_trigger_t) 0xF, // Not used
  .p_callback = NULL,
  /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend = &g_adc0_cfg_extend,
#if defined(VECTOR_NUMBER_ADC0_SCAN_END)
    .scan_end_irq        = VECTOR_NUMBER_ADC0_SCAN_END,
#else
  .scan_end_irq = FSP_INVALID_VECTOR,
#endif
  .scan_end_ipl = (10),
#if defined(VECTOR_NUMBER_ADC0_SCAN_END_B)
    .scan_end_b_irq      = VECTOR_NUMBER_ADC0_SCAN_END_B,
#else
  .scan_end_b_irq = FSP_INVALID_VECTOR,
#endif
  .scan_end_b_ipl = (BSP_IRQ_DISABLED), };
#if ((0) | (0))
const adc_window_cfg_t g_adc0_window_cfg =
{
    .compare_mask        =  0,
    .compare_mode_mask   =  0,
    .compare_cfg         = (adc_compare_cfg_t) ((0) | (0) | (0) | (ADC_COMPARE_CFG_EVENT_OUTPUT_OR)),
    .compare_ref_low     = 0,
    .compare_ref_high    = 0,
    .compare_b_channel   = (ADC_WINDOW_B_CHANNEL_0),
    .compare_b_mode      = (ADC_WINDOW_B_MODE_LESS_THAN_OR_OUTSIDE),
    .compare_b_ref_low   = 0,
    .compare_b_ref_high  = 0,
};
#endif
const adc_channel_cfg_t g_adc0_channel_cfg =
{ .scan_mask = ADC_MASK_CHANNEL_0 | ADC_MASK_CHANNEL_4 | ADC_MASK_CHANNEL_7 | 0,
  .scan_mask_group_b = 0,
  .priority_group_a = ADC_GROUP_A_PRIORITY_OFF,
  .add_mask = 0,
  .sample_hold_mask = 0,
  .sample_hold_states = 24,
#if ((0) | (0))
    .p_window_cfg        = (adc_window_cfg_t *) &g_adc0_window_cfg,
#else
  .p_window_cfg = NULL,
#endif
        };
/* Instance structure to use this module. */
const adc_instance_t g_adc0 =
{ .p_ctrl = &g_adc0_ctrl, .p_cfg = &g_adc0_cfg, .p_channel_cfg = &g_adc0_channel_cfg, .p_api = &g_adc_on_adc };
void g_hal_init(void)
{
    g_common_init ();
}
