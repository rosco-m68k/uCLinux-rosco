/***********************************************************************
 * linux/include/asm-armnommu/arch-c5471/hardware.h
 *
 *   Copyright (C) 2003 Cadenux, LLC. All rights reserved.
 *   todd.fischer@cadenux.com  <www.cadenux.com>
 *
 *   Copyright (C) 2001 RidgeRun, Inc. All rights reserved.
 *         
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***********************************************************************/

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/autoconf.h>


/* Memory Map *************************************************************/

/* SDRAM start/size and FLASH start/size are defined using the kernel
   configuration tool
*/

#define C5471_IRAM_START 0xffc00000
#define C5471_IRAM_SIZE  0x00004000

/* UARTs ******************************************************************/

/* HARD_RESET_N0W: This is used in drivers/block/block.c.
 */

#define HARD_RESET_NOW()

/* UARTs ******************************************************************/

#define UART_IRDA_BASE         0xffff0800
#define UART_MODEM_BASE        0xffff1000
#define UARTn_IO_RANGE         0x00000800

/* Common UART Registers.  Expressed as offsets from the BASE address */

#define UART_RHR_OFFS          0x00000000 /* Rcv Holding Register */
#define UART_THR_OFFS          0x00000004 /* Xmit Holding Register */
#define UART_FCR_OFFS          0x00000008 /* FIFO Control Register */
#define UART_RFCR_OFFS         0x00000008 /* Rcv FIFO Control Register */
#define UART_TFCR_OFFS         0x00000008 /* Xmit FIFO Control Register */
#define UART_SCR_OFFS          0x0000000c /* Status Control Register */
#define UART_LCR_OFFS          0x00000010 /* Line Control Register */
#define UART_LSR_OFFS          0x00000014 /* Line Status Register */
#define UART_SSR_OFFS          0x00000018 /* Supplementary Status Register */
#define UART_MCR_OFFS          0x0000001c /* Modem Control Register */
#define UART_MSR_OFFS          0x00000020 /* Modem Status Register */
#define UART_IER_OFFS          0x00000024 /* Interrupt Enable Register */
#define UART_ISR_OFFS          0x00000028 /* Interrupt Status Register */
#define UART_EFR_OFFS          0x0000002c /* Enhanced Feature Register */
#define UART_XON1_OFFS         0x00000030 /* XON1 Character Register */
#define UART_XON2_OFFS         0x00000034 /* XON2 Character Register */
#define UART_XOFF1_OFFS        0x00000038 /* XOFF1 Character Register */
#define UART_XOFF2_OFFS        0x0000003c /* XOFF2 Character Register */
#define UART_SPR_OFFS          0x00000040 /* Scratch-pad Register */
#define UART_DIV_115K_OFFS     0x00000044 /* Divisor for baud generation */
#define UART_DIV_BIT_RATE_OFFS 0x00000048 /* For baud rate generation */
#define UART_TCR_OFFS          0x0000004c /* Transmission Control Register */
#define UART_TLR_OFFS          0x00000050 /* Trigger Level Register */
#define UART_MDR_OFFS          0x00000054 /* Mode Definition Register */

/* Registers available only for the IrDA UART (absolute address). */

#define UART_IRDA_MDR1         0xffff0854 /* Mode Definition Register 1 */
#define UART_IRDA_MDR2         0xffff0858 /* Mode Definition Register 2 */
#define UART_IRDA_TXFLL        0xffff085c /* LS Xmit Frame Length Register */
#define UART_IRDA_TXFLH        0xffff0860 /* MS Xmit Frame Length Register */
#define UART_IRDA_RXFLL        0xffff0864 /* LS Rcvd Frame Length Register */
#define UART_IRDA_RXFLH        0xffff0868 /* MS Rcvd Frame Length Register */
#define UART_IRDA_SFLSR        0xffff086c /* Status FIFO Line Status Reg */
#define UART_IRDA_SFREGL       0xffff0870 /* LS Status FIFO Register */
#define UART_IRDA_SFREGH       0xffff0874 /* MS Status FIFO Register */
#define UART_IRDA_BLR          0xffff0878 /* Begin of File Length Register */
#define UART_IRDA_PULSE_WIDTH  0xffff087c /* Pulse Width Register */
#define UART_IRDA_ACREG        0xffff0880 /* Auxiliary Control Register */
#define UART_IRDA_PULSE_START  0xffff0884 /* Start time of pulse */
#define UART_IRDA_RX_W_PTR     0xffff0888 /* RX FIFO write pointer */
#define UART_IRDA_RX_R_PTR     0xffff088c /* RX FIFO read pointer */
#define UART_IRDA_TX_W_PTR     0xffff0890 /* TX FIFO write pointer */
#define UART_IRDA_TX_R_PTR     0xffff0894 /* TX FIFO read pointer */
#define UART_IRDA_STATUS_W_PTR 0xffff0898 /* Write pointer of status FIFO */
#define UART_IRDA_STATUS_R_PTR 0xffff089c /* Read pointer of status FIFO */
#define UART_IRDA_RESUME       0xffff08a0 /* Resume register */
#define UART_IRDA_MUX          0xffff08a4 /* Selects UART_IRDA output mux */

/* Registers available for the Modem UART (absolute addresses) */

#define UART_MODEM_SER_RHR     0xffff1000 /* RX Hold Register */
#define UART_MODEM_SER_THR     0xffff1004 /* TX Hold Register */
#define UART_MODEM_FCR         0xffff1008 /* FIFO Control Register */
#define UART_MODEM_SER_LCR     0xffff1010 /* Line Control Register */
#define UART_MODEM_SER_SR      0xffff1014 /* Line Status Register */
#define UART_MODEM_115K_DIV    0xffff1044 /* ARM clock to 115K divisor Reg */
#define UART_MODEM_DIV         0xffff1048 /* Baud Rate Divisor Register */
#define UART_MODEM_MDR         0xffff1054 /* Mode Definition Register */
#define UART_MODEM_UASR        0xffff1058 /* UART Auto-baud Status Register */
#define UART_MODEM_RDPTR_URX   0xffff105c /* RX FIFO Read Pointer Register */
#define UART_MODEM_WRPTR_URX   0xffff1060 /* RX FIFO Write Pointer Register */
#define UART_MODEM_RDPTR_UTX   0xffff1064 /* TX FIFO Read Pointer Register */
#define UART_MODEM_WRPTR_UTX   0xffff1068 /* TX FIFO Write Pointer Register */

/* UART Settings **********************************************************/

/* Miscellaneous UART settings. */

#define UART_RX_FIFO_NOEMPTY 0x00000001
#define UART_SSR_TXFULL      0x00000001
#define UART_LSR_TREF        0x00000020

#define UART_XMIT_FIFO_SIZE      64
#define UART_IRDA_XMIT_FIFO_SIZE 64

/* UART_LCR Register */
                                        /* Bits 31-7: Reserved */
#define UART_LCR_BOC         0x00000040 /* Bit 6: Break Control */
                                        /* Bit 5: Parity Type 2 */
#define UART_LCR_ParEven     0x00000010 /* Bit 4: Parity Type 1 */
#define UART_LCR_ParOdd      0x00000000
#define UART_LCR_ParEn       0x00000008 /* Bit 3: Paity Enable */
#define UART_LCR_ParDis      0x00000000
#define UART_LCR_2stop       0x00000004 /* Bit 2: Number of stop bits */
#define UART_LCR_1stop       0x00000000
#define UART_LCR_5bits       0x00000000 /* Bits 0-1: Word-length */
#define UART_LCR_6bits       0x00000001
#define UART_LCR_7bits       0x00000002
#define UART_LCR_8bits       0x00000003

#define UART_FCR_FTL         0x00000000
#define UART_FCR_FIFO_EN     0x00000001
#define UART_FCR_TX_CLR      0x00000002
#define UART_FCR_RX_CLR      0x00000004

#define UART_IER_RecvInt     0x00000001
#define UART_IER_XmitInt     0x00000002
#define UART_IER_LineStsInt  0x00000004
#define UART_IER_ModemStsInt 0x00000008 /* IrDA UART only */
#define UART_IER_XoffInt     0x00000020
#define UART_IER_RtsInt      0x00000040 /* IrDA UART only */
#define UART_IER_CtsInt      0x00000080 /* IrDA UART only */
#define UART_IER_AllInts     0x000000ff

#define BAUD_115200          0x00000001
#define BAUD_57600           0x00000002
#define BAUD_38400           0x00000003
#define BAUD_19200           0x00000006
#define BAUD_9600            0x0000000C
#define BAUD_4800            0x00000018
#define BAUD_2400            0x00000030
#define BAUD_1200            0x00000060

#define MDR_UART_MODE        0x00000000  /* Both IrDA and Modem UARTs */
#define MDR_SIR_MODE         0x00000001  /* IrDA UART only */
#define MDR_AUTOBAUDING_MODE 0x00000002  /* Modem UART only */
#define MDR_RESET_MODE       0x00000007  /* Both IrDA and Modem UARTs */

/* SPI ********************************************************************/

#define MAX_SPI 3

#define SPI_REGISTER_BASE    0xffff2000

/* GIO ********************************************************************/

#define MAX_GIO (35)

#define GIO_REGISTER_BASE    0xffff2800

#define GPIO_IO              0xffff2800 /* Writeable when I/O is configured
					 * as an output; reads value on I/O
					 * pin when I/O is configured as an
					 * input */
#define GPIO_CIO             0xffff2804 /* GPIO configuration register */
#define GPIO_IRQA            0xffff2808 /* In conjunction with GPIO_IRQB
					 * determines the behavior when GPIO
					 * pins configured as input IRQ */
#define GPIO_IRQB            0xffff280c /* Determines the behavior when GPIO
					 * pins configured as input IRQ */
#define GPIO_DDIO            0xffff2810 /* Delta Detect Register
					 * (detects changes in the I/O pins) */
#define GPIO_EN              0xffff2814 /* Selects register for muxed GPIOs */

#define KGIO_REGISTER_BASE   0xffff2900

#define KBGPIO_IO            0xffff2900 /* Keyboard I/O bits: Writeable
					 * when KBGPIO is configured as an
					 * output; reads value on I/O pin
					 * when KBGPIO is configured as an
					 * input */
#define KBGPIO_CIO           0xffff2904 /* KBGPIO configuration register */
#define KBGPIO_IRQA          0xffff2908 /* In conjunction with KBGPIO_IRQB
					 * determines the behavior when
					 * KBGPIO pins configured as input
					 * IRQ */
#define KBGPIO_IRQB          0xffff290c /* In conjunction with KBGPIO_IRQA
					 * determines the behavior when
					 * KBGPIO pins configured as input
					 * IRQ */
#define KBGPIO_DDIO          0xffff2910 /* Delta Detect Register (detects
					 * changes in the KBGPIO pins) */
#define KBGPIO_EN            0xffff2914 /* Selects register for muxed
					 * KBGPIOs */

/* Timers *****************************************************************/

#define C5471_TIMER0_CTRL    0xffff2a00
#define C5471_TIMER0_CNT     0xffff2a04
#define C5471_TIMER1_CTRL    0xffff2b00
#define C5471_TIMER1_CNT     0xffff2b04
#define C5471_TIMER2_CTRL    0xffff2c00

#define C5471_TIMER2_CNT     0xffff2c04

/* Interrupts
 *
 * NOTE:  The 2.0.x implementation uses SRC_IRQ_BIN_REG to decode
 * interrupts (see entry-armv.S).  However, this register is
 * not documented int the TMS320VC5472 User Guide???
 * The switch, HAVE_SRC_IRQ_BIN_REG will enable use of this
 * register.
 */

#define HAVE_SRC_IRQ_BIN_REG 0

#define INT_FIRST_IO         0xffff2d00
#define INT_IO_RANGE         0x5C

#define IT_REG               0xffff2d00
#define MASK_IT_REG          0xffff2d04
#define SRC_IRQ_REG          0xffff2d08
#define SRC_FIQ_REG          0xffff2d0c
#define SRC_IRQ_BIN_REG      0xffff2d10
#define INT_CTRL_REG         0xffff2d18

#define ILR_IRQ0_REG         0xffff2d1C /*  0-Timer 0       */
#define ILR_IRQ1_REG         0xffff2d20 /*  1-Timer 1       */
#define ILR_IRQ2_REG         0xffff2d24 /*  2-Timer 2       */
#define ILR_IRQ3_REG         0xffff2d28 /*  3-GPIO0         */
#define ILR_IRQ4_REG         0xffff2d2c /*  4-Ethernet      */
#define ILR_IRQ5_REG         0xffff2d30 /*  5-KBGPIO[7:0]   */
#define ILR_IRQ6_REG         0xffff2d34 /*  6-Uart   serial */
#define ILR_IRQ7_REG         0xffff2d38 /*  7-Uart   IRDA   */
#define ILR_IRQ8_REG         0xffff2d3c /*  8-KBGPIO[15:8]  */
#define ILR_IRQ9_REG         0xffff2d40 /*  9-GPIO3         */
#define ILR_IRQ10_REG        0xffff2d44 /* 10-GPIO2         */
#define ILR_IRQ11_REG        0xffff2d48 /* 11-I2C           */
#define ILR_IRQ12_REG        0xffff2d4c /* 12-GPIO1         */
#define ILR_IRQ13_REG        0xffff2d50 /* 13-SPI           */
#define ILR_IRQ14_REG        0xffff2d54 /* 14-GPIO[19:4]    */
#define ILR_IRQ15_REG        0xffff2d58 /* 15-API           */

/* I2C ********************************************************************/

#define MAX_I2C 1

/* API ********************************************************************/


#define DSPRAM_BASE          0xffe00000  /* DSPRAM base address */
#define DSPRAM_END           0xffe03fff

#endif  /* __ASM_ARCH_HARDWARE_H */
