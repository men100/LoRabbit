/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = adc_scan_end_isr, /* ADC0 SCAN END (End of A/D scanning operation) */
            [1] = i3c_rcv_isr, /* I3C0 RCV STATUS (Receive status buffer full) */
            [2] = i3c_resp_isr, /* I3C0 RESPONSE (Response status buffer full) */
            [3] = i3c_rx_isr, /* I3C0 RX (Receive) */
            [4] = i3c_tx_isr, /* I3C0 TX (Transmit) */
            [5] = i3c_ibi_isr, /* I3C0 IBI (IBI status buffer full) */
            [6] = i3c_eei_isr, /* I3C0 EEI (Error) */
            [7] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [8] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [9] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [10] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [11] = sci_b_uart_rxi_isr, /* SCI2 RXI (Receive data full) */
            [12] = sci_b_uart_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [13] = sci_b_uart_tei_isr, /* SCI2 TEI (Transmit end) */
            [14] = sci_b_uart_eri_isr, /* SCI2 ERI (Receive error) */
            [15] = r_icu_isr, /* ICU IRQ1 (External pin interrupt 1) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_ADC0_SCAN_END,GROUP0), /* ADC0 SCAN END (End of A/D scanning operation) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_I3C0_RCV_STATUS,GROUP1), /* I3C0 RCV STATUS (Receive status buffer full) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_I3C0_RESPONSE,GROUP2), /* I3C0 RESPONSE (Response status buffer full) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_I3C0_RX,GROUP3), /* I3C0 RX (Receive) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_I3C0_TX,GROUP4), /* I3C0 TX (Transmit) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_I3C0_IBI,GROUP5), /* I3C0 IBI (IBI status buffer full) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_I3C0_EEI,GROUP6), /* I3C0 EEI (Error) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP7), /* IIC1 RXI (Receive data full) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP0), /* IIC1 TXI (Transmit data empty) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP1), /* IIC1 TEI (Transmit end) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP2), /* IIC1 ERI (Transfer error) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SCI2_RXI,GROUP3), /* SCI2 RXI (Receive data full) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP4), /* SCI2 TXI (Transmit data empty) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP5), /* SCI2 TEI (Transmit end) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_SCI2_ERI,GROUP6), /* SCI2 ERI (Receive error) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ1,GROUP7), /* ICU IRQ1 (External pin interrupt 1) */
        };
        #endif
        #endif
