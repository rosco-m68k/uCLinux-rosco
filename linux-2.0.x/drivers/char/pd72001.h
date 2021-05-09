/*****************************************************************************/
/* 
 *  pd72001.h, v1.0 <2003-07-28 10:48:10 gc>
 * 
 *  linux/drivers/char/pd72001.h
 *
 *  uClinux version 2.0.x NEC uPD72001 MPSC serial driver
 *
 *  Author:     Guido Classen (classeng@clagi.de)
 *
 *  This program is free software;  you can redistribute it and/or modify it
 *  under the  terms of the GNU  General Public License as  published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  This program  is distributed  in the  hope that it  will be  useful, but
 *  WITHOUT   ANY   WARRANTY;  without   even   the   implied  warranty   of
 *  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR  PURPOSE.   See the  GNU
 *  General Public License for more details.
 * *
 *  Change history:
 *       2003-07-28 gc: initial version
 *
 */
 /****************************************************************************/

#ifndef __PD72001_MPSC_H
#define __PD72001_MPSC_H

typedef struct _Mpsc {
        volatile u8 *control;
        volatile u8 *data;
} Mpsc;



#define MPSC_WRITE_REG(scc, reg, data) do {  if ((reg) != 0)           \
                                              *(scc).control = (reg);  \
                                              *(scc).control = (data); \
                                          } while((0))
#define MPSC_WRITE_DATA(scc, dat) ( *(scc).data = (dat) )
#define MPSC_WRITE_CONTROL(scc, dat) ( *(scc).control = (dat) )
#define MPSC_WRITE_COMMAND MPSC_WRITE_CONTROL

#define MPSC_READ_REG(scc, reg) ( (*(scc).control = (reg)), *(scc).control) 
#define MPSC_READ_DATA(scc) ( *(scc).data) 
#define MPSC_READ_CONTROL(scc) ( *(scc).control) 



#define MPSC_SR0 0                      /* status register 0 */
#define MPSC_SR0_RX_CHAR_AVAIL          0x01
#define MPSC_SR0_SENDING_ABORT          0x02
#define MPSC_SR0_TX_BUF_EMPTY           0x04
#define MPSC_SR0_SHORT_FRAME            0x08
#define MPSC_SR0_PARITY_ERROR           0x10
#define MPSC_SR0_RX_OVERRUN             0x20
#define MPSC_SR0_CRC_FRAMING_ERROR      0x40
#define MPSC_SR0_END_OF_FRAME           0x80

#define MPSC_SR1 1                      /* status register 1 */
#define MPSC_SR1_BRG_ZERO_COUNT         0x01
#define MPSC_SR1_IDLE_DETECT            0x02
#define MPSC_SR1_ALL_SENT               0x04
#define MPSC_SR1_DCD                    0x08
#define MPSC_SR1_SYNC_HUNT              0x10
#define MPSC_SR1_CTS                    0x20
#define MPSC_SR1_TX_UNDERRUN_EOM        0x40
#define MPSC_SR1_BREAK_ABORT_GA_DETECT  0x80

#define MPSC_SR2 2                      /* status register 2 */
/* in RR_2 of channel B (only there!!!)
 * is the cause of the interrupt encoded in
 * folowing bits. (assuming Output Vector Type B is selected in CR2A)
 */
#define MPSC_SR2B_INT_CHANNEL_A         0x04

/* Bits used for interrupt event */
#define MPSC_SR2B_EVENT_MASK            0x03    
#define MPSC_SR2B_INT_TX_EMPTY          0x00
#define MPSC_SR2B_INT_EXT_STATUS        0x01
#define MPSC_SR2B_INT_RX_AVAIL          0x02
#define MPSC_SR2B_INT_SPECIAL_RX        0x03 


#define MPSC_SR3 3                      /* status register 3 */
#define MPSC_SR4 4                      /* status register 4 */
#define MPSC_SR5 5                      /* status register 5 */
#define MPSC_SR6 6                      /* status register 6 */
#define MPSC_SR7 7                      /* status register 7 */
#define MPSC_SR8 8                      /* status register 8 */
#define MPSC_SR9 9                      /* status register 9 */
#define MPSC_SR10 10                    /* status register 10 */
#define MPSC_SR11 11                    /* status register 11 */
#define MPSC_SR12 12                    /* status register 12 */
#define MPSC_SR13 13                    /* status register 13 */
#define MPSC_SR14 14                    /* status register 14 */
#define MPSC_SR15 15                    /* status register 15 */


#define MPSC_CR0 0                      /* write register 0 */

/* scc commands */
#define MPSC_CR0_CMD_MASK               0x38
#define MPSC_CR0_CMD_NULL               0x00
#define MPSC_CR0_CMD_POINT_HIGH         0x08
#define MPSC_CR0_CMD_RESET_EXT_STAT_INT 0x10
#define MPSC_CR0_CMD_CHANNEL_RESET      0x18
#define MPSC_CR0_CMD_ENABLE_RX_INT      0x20
#define MPSC_CR0_CMD_RESET_TX_INT       0x28
#define MPSC_CR0_CMD_RESET_ERROR        0x30
#define MPSC_CR0_CMD_END_OF_INT         0x38

#define MPSC_CR0_CMD_RESET_RX_CRC       0x40
#define MPSC_CR0_CMD_RESET_TX_CRC       0x80
#define MPSC_CR0_CMD_RESET_TX_UNDERRUN_EOM  0xc0





#define MPSC_CR1 1                      /* control register 1 */


#define MPSC_CR1_EXT_INT_ENABLE         0x01    /* enable extern interrupt */
#define MPSC_CR1_TX_INT_ENABLE          0x02    /* enable TX interrrupt */
#define MPSC_CR1_FIRST_TRANSMIT_INT     0x04

#define MPSC_CR1_RX_INT_MASK            0x18
#define MPSC_CR1_RX_INT_DISABLE         0x00    
#define MPSC_CR1_RX_INT_FIRST_CHAR      0x08    /* RX int. for first char in */
                                                /* fifo */
#define MPSC_CR1_RX_INT_ALL_1           0x10 
#define MPSC_CR1_RX_INT_ALL_2           0x18 

#define MPSC_CR1_FIRST_RX_INT_MASK      0x20
#define MPSC_CR1_OVERRUN_ERROR_SPECIAL  0x40
#define MPSC_CR1_SHORT_FRAME_DETECT     0x80


#define MPSC_CR2 2                      /* control register 2 */
#define MPSC_CR2A_VECTOR_MODE_VECTORED  0x80
#define MPSC_CR2A_STATUS_AFFECTS_VECTOR 0x40

#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_MASK 0x38
#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_A1 0x00
#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_A2 0x08
#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_A3 0x10
#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_B1 0x18
#define MPSC_CR2A_OUTPUT_VECTOR_TYPE_B2 0x20

#define MPSC_CR2A_PRIORITY_RxB          0x04
#define MPSC_CR2A_INT_DMA_MODE_MASK     0x03
#define MPSC_CR2A_INT_DMA_MODE_BOTH_INT 0x00
#define MPSC_CR2A_INT_DMA_MODE_A_DMA    0x01
#define MPSC_CR2A_INT_DMA_MODE_BOTH_DMA 0x02



#define MPSC_CR3 3                      /* control register 3 */
#define MPSC_CR3_RX_ENABLE              0x01
#define MPSC_CR3_SYNC_CHAR_LOAD_INHIBIT 0x02
#define MPSC_CR3_ADDR_SEARCH_MODE       0x04
#define MPSC_CR3_RX_CRC_ENABLE          0x08
#define MPSC_CR3_ENTER_HUNT_MODE        0x10
#define MPSC_CR3_AUTO_ENABLE            0x20

#define MPSC_CR3_RX_BITS_MASK           0xc0
#define MPSC_CR3_RX_BITS_5              0x00
#define MPSC_CR3_RX_BITS_7              0x40
#define MPSC_CR3_RX_BITS_6              0x80
#define MPSC_CR3_RX_BITS_8              0xc0


#define MPSC_CR4 4                      /* control register 4 */
#define MPSC_CR4_PARITY_ENABLE          0x01
#define MPSC_CR4_PARITY_EVEN            0x02

#define MPSC_CR4_STOPBITS_MASK          0x0c
#define MPSC_CR4_STOPBITS_SYNC          0x00
#define MPSC_CR4_STOPBITS_1             0x04
#define MPSC_CR4_STOPBITS_1_5           0x08
#define MPSC_CR4_STOPBITS_2             0x0c

#define MPSC_CR4_SYNCMODE_MASK          0x30
#define MPSC_CR4_SYNCMODE_MONO          0x00
#define MPSC_CR4_SYNCMODE_BI            0x10
#define MPSC_CR4_SYNCMODE_SDLC          0x20
#define MPSC_CR4_SYNCMODE_EXTERNAL      0x30

#define MPSC_CR4_CLOCKRATE_MASK         0xc0
#define MPSC_CR4_CLOCKRATE_X1           0x00
#define MPSC_CR4_CLOCKRATE_X16          0x40
#define MPSC_CR4_CLOCKRATE_X32          0x80
#define MPSC_CR4_CLOCKRATE_X64          0xc0

#define MPSC_CR5 5                      /* control register 5 */
#define MPSC_CR5_TX_CRC_ENABLE          0x01
#define MPSC_CR5_TX_RTS                 0x02
#define MPSC_CR5_TX_SDLC_CRC16          0x04
#define MPSC_CR5_TX_ENABLE              0x08
#define MPSC_CR5_TX_SEND_BREAK          0x10

#define MPSC_CR5_TX_BITS_MASK           0x60
#define MPSC_CR5_TX_BITS_5              0x00
#define MPSC_CR5_TX_BITS_7              0x20
#define MPSC_CR5_TX_BITS_6              0x40
#define MPSC_CR5_TX_BITS_8              0x60
#define MPSC_CR5_TX_DTR                 0x80

#define MPSC_CR6 6                      /* control register 6 */
/* SYNC character / secondary station address */

#define MPSC_CR7 7                      /* control register 7 */
/* SYNC character / flag */

#define MPSC_CR8 8                      /* control register 8 */
/* TX data length low */

#define MPSC_CR9 9                      /* control register 9 */
/* TX data length hi */


#define MPSC_CR10 10                    /* control register 10 */
/* 0 for asynchronous operation */

#define MPSC_CR11 11                    /* control register 11 */
/* E/S interrupt mask */

#define MPSC_CR12 12                    /* control register 12 */
/* BRG control */
#define MPSC_CR12_RX_BRG_REGISTER_SET   0x01
#define MPSC_CR12_TX_BRG_REGISTER_SET   0x02
#define MPSC_CR12_RX_BRG_INT_ENABLE     0x04
#define MPSC_CR12_TX_BRG_INT_ENABLE     0x08
#define MPSC_CR12_TX_BRG_FOR_DPLL       0x40
#define MPSC_CR12_RX_BRG_FOR_DPLL       0x00
#define MPSC_CR12_TX_BRG_FOR_TRXC       0x80
#define MPSC_CR12_RX_BRG_FOR_TRXC       0x00


#define MPSC_CR13 13                    /* control register 13 */
/* transmit data length */

#define MPSC_CR14 14                    /* control register 14 */
#define MPSC_CR14_TX_BRG_ENABLE         0x01
#define MPSC_CR14_RX_BRG_ENABLE         0x02
#define MPSC_CR14_BRG_SOURCE_SYS_CLOCK  0x04
#define MPSC_CR14_BRG_SOURCE_XTAL       0x00
#define MPSC_CR14_ECHO_LOOP_TEST        0x08
#define MPSC_CR14_LOCAL_SELF_TEST       0x10

#define MPSC_CR14_DPLL_CMD_MASK         0xe0
#define MPSC_CR14_DPLL_CMD_NOP          0x00
#define MPSC_CR14_DPLL_CMD_ENTER_SEARCH 0x20
#define MPSC_CR14_DPLL_CMD_RESET_M_CLK  0x40
#define MPSC_CR14_DPLL_CMD_DPLL_DISABLE 0x60
#define MPSC_CR14_DPLL_CMD_DPLL_SRC_BRG 0x80
#define MPSC_CR14_DPLL_CMD_DPLL_SRC_XTAL 0x90
#define MPSC_CR14_DPLL_CMD_FM_MODE      0xa0
#define MPSC_CR14_DPLL_CMD_NRZI_MODE    0xe0

#define MPSC_CR15 15                    /* control register 15 */
#define MPSC_CR15_TRXC_SOURCE_MASK      0x03
#define MPSC_CR15_TRXC_SOURCE_XTAL      0x00
#define MPSC_CR15_TRXC_SOURCE_TXCLK     0x01
#define MPSC_CR15_TRXC_SOURCE_BRG_CLK   0x02
#define MPSC_CR15_TRXC_SOURCE_DPLL_CLK  0x03

#define MPSC_CR15_TRXC_INPUT            0x00
#define MPSC_CR15_TRXC_OUTPUT           0x04

#define MPSC_CR15_TXCLK_SELECT_MASK     0x18
#define MPSC_CR15_TXCLK_SELECT_STRXC    0x00
#define MPSC_CR15_TXCLK_SELECT_TRXC     0x08
#define MPSC_CR15_TXCLK_SELECT_TXBRG    0x10
#define MPSC_CR15_TXCLK_SELECT_DPLL     0x18

#define MPSC_CR15_RXCLK_SELECT_MASK     0x60
#define MPSC_CR15_RXCLK_SELECT_STRXC    0x00
#define MPSC_CR15_RXCLK_SELECT_TRXC     0x20
#define MPSC_CR15_RXCLK_SELECT_RXBRG    0x40
#define MPSC_CR15_RXCLK_SELECT_DPLL     0x60

#define MPSC_CR15_XTAL_ENABLE           0x80

#endif /* __PD72001_MPSC_H */

/*
 *Local Variables:
 * mode: c
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * indent-tabs-mode: nil
 * c-file-style: "Linux"
 * fill-column: 76
 * tab-width: 8
 * time-stamp-pattern: "4/<%%>"
 * End:
 */
