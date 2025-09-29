/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (20)
#endif
/* ISR prototypes */
void glcdc_line_detect_isr(void);
void mipi_dsi_seq0_isr(void);
void mipi_dsi_seq1_isr(void);
void mipi_dsi_vin1_isr(void);
void mipi_dsi_rcv_isr(void);
void mipi_dsi_ferr_isr(void);
void mipi_dsi_ppi_isr(void);
void r_icu_isr(void);
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void gpt_counter_overflow_isr(void);
void sci_b_uart_rxi_isr(void);
void sci_b_uart_txi_isr(void);
void sci_b_uart_tei_isr(void);
void sci_b_uart_eri_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
#define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
#define VECTOR_NUMBER_MIPIDSI_SEQ0 ((IRQn_Type) 1) /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
#define MIPIDSI_SEQ0_IRQn          ((IRQn_Type) 1) /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
#define VECTOR_NUMBER_MIPIDSI_SEQ1 ((IRQn_Type) 2) /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
#define MIPIDSI_SEQ1_IRQn          ((IRQn_Type) 2) /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
#define VECTOR_NUMBER_MIPIDSI_VIN1 ((IRQn_Type) 3) /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
#define MIPIDSI_VIN1_IRQn          ((IRQn_Type) 3) /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
#define VECTOR_NUMBER_MIPIDSI_RCV ((IRQn_Type) 4) /* MIPIDSI RCV (DSI packet receive interrupt) */
#define MIPIDSI_RCV_IRQn          ((IRQn_Type) 4) /* MIPIDSI RCV (DSI packet receive interrupt) */
#define VECTOR_NUMBER_MIPIDSI_FERR ((IRQn_Type) 5) /* MIPIDSI FERR (DSI fatal error interrupt) */
#define MIPIDSI_FERR_IRQn          ((IRQn_Type) 5) /* MIPIDSI FERR (DSI fatal error interrupt) */
#define VECTOR_NUMBER_MIPIDSI_PPI ((IRQn_Type) 6) /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
#define MIPIDSI_PPI_IRQn          ((IRQn_Type) 6) /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
#define VECTOR_NUMBER_ICU_IRQ3 ((IRQn_Type) 7) /* ICU IRQ3 (External pin interrupt 3) */
#define ICU_IRQ3_IRQn          ((IRQn_Type) 7) /* ICU IRQ3 (External pin interrupt 3) */
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 8) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 8) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 9) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 9) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 10) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 10) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 11) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 11) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW ((IRQn_Type) 12) /* GPT0 COUNTER OVERFLOW (Overflow) */
#define GPT0_COUNTER_OVERFLOW_IRQn          ((IRQn_Type) 12) /* GPT0 COUNTER OVERFLOW (Overflow) */
#define VECTOR_NUMBER_SCI2_RXI ((IRQn_Type) 13) /* SCI2 RXI (Receive data full) */
#define SCI2_RXI_IRQn          ((IRQn_Type) 13) /* SCI2 RXI (Receive data full) */
#define VECTOR_NUMBER_SCI2_TXI ((IRQn_Type) 14) /* SCI2 TXI (Transmit data empty) */
#define SCI2_TXI_IRQn          ((IRQn_Type) 14) /* SCI2 TXI (Transmit data empty) */
#define VECTOR_NUMBER_SCI2_TEI ((IRQn_Type) 15) /* SCI2 TEI (Transmit end) */
#define SCI2_TEI_IRQn          ((IRQn_Type) 15) /* SCI2 TEI (Transmit end) */
#define VECTOR_NUMBER_SCI2_ERI ((IRQn_Type) 16) /* SCI2 ERI (Receive error) */
#define SCI2_ERI_IRQn          ((IRQn_Type) 16) /* SCI2 ERI (Receive error) */
#define VECTOR_NUMBER_ICU_IRQ1 ((IRQn_Type) 17) /* ICU IRQ1 (External pin interrupt 1) */
#define ICU_IRQ1_IRQn          ((IRQn_Type) 17) /* ICU IRQ1 (External pin interrupt 1) */
#define VECTOR_NUMBER_ICU_IRQ12 ((IRQn_Type) 18) /* ICU IRQ12 (External pin interrupt 12) */
#define ICU_IRQ12_IRQn          ((IRQn_Type) 18) /* ICU IRQ12 (External pin interrupt 12) */
#define VECTOR_NUMBER_ICU_IRQ13 ((IRQn_Type) 19) /* ICU IRQ13 (External pin interrupt 13) */
#define ICU_IRQ13_IRQn          ((IRQn_Type) 19) /* ICU IRQ13 (External pin interrupt 13) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (20)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
