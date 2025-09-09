/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (15)
#endif
/* ISR prototypes */
void adc_scan_end_isr(void);
void i3c_rcv_isr(void);
void i3c_resp_isr(void);
void i3c_rx_isr(void);
void i3c_tx_isr(void);
void i3c_ibi_isr(void);
void i3c_eei_isr(void);
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void sci_b_uart_rxi_isr(void);
void sci_b_uart_txi_isr(void);
void sci_b_uart_tei_isr(void);
void sci_b_uart_eri_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_ADC0_SCAN_END ((IRQn_Type) 0) /* ADC0 SCAN END (End of A/D scanning operation) */
#define ADC0_SCAN_END_IRQn          ((IRQn_Type) 0) /* ADC0 SCAN END (End of A/D scanning operation) */
#define VECTOR_NUMBER_I3C0_RCV_STATUS ((IRQn_Type) 1) /* I3C0 RCV STATUS (Receive status buffer full) */
#define I3C0_RCV_STATUS_IRQn          ((IRQn_Type) 1) /* I3C0 RCV STATUS (Receive status buffer full) */
#define VECTOR_NUMBER_I3C0_RESPONSE ((IRQn_Type) 2) /* I3C0 RESPONSE (Response status buffer full) */
#define I3C0_RESPONSE_IRQn          ((IRQn_Type) 2) /* I3C0 RESPONSE (Response status buffer full) */
#define VECTOR_NUMBER_I3C0_RX ((IRQn_Type) 3) /* I3C0 RX (Receive) */
#define I3C0_RX_IRQn          ((IRQn_Type) 3) /* I3C0 RX (Receive) */
#define VECTOR_NUMBER_I3C0_TX ((IRQn_Type) 4) /* I3C0 TX (Transmit) */
#define I3C0_TX_IRQn          ((IRQn_Type) 4) /* I3C0 TX (Transmit) */
#define VECTOR_NUMBER_I3C0_IBI ((IRQn_Type) 5) /* I3C0 IBI (IBI status buffer full) */
#define I3C0_IBI_IRQn          ((IRQn_Type) 5) /* I3C0 IBI (IBI status buffer full) */
#define VECTOR_NUMBER_I3C0_EEI ((IRQn_Type) 6) /* I3C0 EEI (Error) */
#define I3C0_EEI_IRQn          ((IRQn_Type) 6) /* I3C0 EEI (Error) */
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 7) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 7) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 8) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 8) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 9) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 9) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 10) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 10) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_SCI2_RXI ((IRQn_Type) 11) /* SCI2 RXI (Receive data full) */
#define SCI2_RXI_IRQn          ((IRQn_Type) 11) /* SCI2 RXI (Receive data full) */
#define VECTOR_NUMBER_SCI2_TXI ((IRQn_Type) 12) /* SCI2 TXI (Transmit data empty) */
#define SCI2_TXI_IRQn          ((IRQn_Type) 12) /* SCI2 TXI (Transmit data empty) */
#define VECTOR_NUMBER_SCI2_TEI ((IRQn_Type) 13) /* SCI2 TEI (Transmit end) */
#define SCI2_TEI_IRQn          ((IRQn_Type) 13) /* SCI2 TEI (Transmit end) */
#define VECTOR_NUMBER_SCI2_ERI ((IRQn_Type) 14) /* SCI2 ERI (Receive error) */
#define SCI2_ERI_IRQn          ((IRQn_Type) 14) /* SCI2 ERI (Receive error) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (15)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
