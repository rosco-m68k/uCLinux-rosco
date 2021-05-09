/*
 *  Internal constants and structures used for as UART mode of SCCs on the
 *  68360's CPM.
 *
 *  Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *      <hamilton@sedsystems.ca>
 *
 *  Based on mcfserial.h which was:
 *  Copyright (C) 1999, Greg Ungerer (greg@snapgear.com)
 *  Copyright (C) 2000, Lineo Inc.   (www.lineo.com)
 *
 */
#ifndef __QUICC_SCC_I_H__
#define __QUICC_SCC_I_H__

#include <linux/config.h>
#ifdef __KERNEL__
#if defined(CONFIG_M68360)
#include <linux/serial.h>
#include <asm/m68360.h>
#include <asm/quicc_cpm_i.h>
#include <asm/quicc_sxc_i.h>

struct quicc_scc_serial_t
{
        int                             magic;
        struct uart_pram                *scc_uart_ram;
        struct scc_regs                 *scc_registers;
        sxc_list_t                      scc;
        struct serial_buffer_t          *tx_buffer;
        struct serial_buffer_t          *rx_buffer;
        QUICC_BD                        *tx_buffer_descriptor;
        QUICC_BD                        *rx_buffer_descriptor;
        unsigned short                  current_tx_buffer;
        unsigned short                  current_rx_buffer;
        int                             irq;
        int                             flags;
        int                             irq_aquired;
        int                             type;
        struct tty_struct               *tty;
        unsigned int                    baud;
        unsigned int                    custom_divisor;
        int                             x_char;
        int                             close_delay;
        unsigned short                  closing_wait;
        unsigned long                   event;
        int                             line;
        int                             count;
        int                             blocked_open;
        long                            session;
        long                            pgrp;
        struct quicc_serial_stats       stats;
        struct tq_struct                tqueue_receive;
        struct tq_struct                tqueue_transmit;
        struct tq_struct                tqueue_hangup;
        struct termios                  normal_termios;
        struct termios                  callout_termios;
        struct wait_queue               *open_wait;
        struct wait_queue               *close_wait;
        baudrate_generator_t            baudrate_generator;
        int                             brg_aquired;
        int                             tx_disabled;
        struct serial_buffer_t          put_char_buffer;
        
        int                             pc_stored;
        unsigned short                  pio_pcdir;
        unsigned short                  pio_pcpar;
        unsigned short                  pio_pcso;
        unsigned short                  pio_pcint;
};

/* Interrupt sources on the SCC */
#define SCC_INT_GLITCH_ON_RX        ((unsigned short)(0x0001 << 12))
#define SCC_INT_GLITCH_ON_TX        ((unsigned short)(0x0001 << 11))
#define SCC_INT_AUTOBAUD_LOCK       ((unsigned short)(0x0001 <<  9))
#define SCC_INT_IDLE                ((unsigned short)(0x0001 <<  8))
#define SCC_INT_GRACEFUL_STOP       ((unsigned short)(0x0001 <<  7))
#define SCC_INT_BREAK_END           ((unsigned short)(0x0001 <<  6))
#define SCC_INT_BREAK_RECEIVED      ((unsigned short)(0x0001 <<  5))
#define SCC_INT_CONTROL_CHARACTER   ((unsigned short)(0x0001 <<  3))
    /*Only if the character is set to be rejected.*/
#define SCC_INT_NO_BUFFERS          ((unsigned short)(0x0001 <<  2))
#define SCC_INT_TX_DONE             ((unsigned short)(0x0001 <<  1))
#define SCC_INT_RX_DATA             ((unsigned short)(0x0001 <<  0))


/*Defines for configuring port a and port c */
#define PORTA_SCC1_SET          ((unsigned short)(0x0003))
#define PORTA_SCC1_CLEAR        ((unsigned short)(0xFFFC))
#define PORTA_SCC2_SET          ((unsigned short)(0x000C))
#define PORTA_SCC2_CLEAR        ((unsigned short)(0xFFF3))
#define PORTA_SCC3_SET          ((unsigned short)(0x0030))
#define PORTA_SCC3_CLEAR        ((unsigned short)(0xFFCF))
#define PORTA_SCC4_SET          ((unsigned short)(0x00C0))
#define PORTA_SCC4_CLEAR        ((unsigned short)(0xFF3F))


/*
   Defines for the General Scc Mode Register
   Defines for the GSMR_H
   Defines for the GSMR_L
*/
#define GSMR_L_TX_ENABLE        ((unsigned long)(0x00000001 <<  4))
#define GSMR_L_RX_ENABLE        ((unsigned long)(0x00000001 <<  5))

#define SCC_GSMR_H              ((unsigned long)(0x00000060))
/*Small tx and rx fifos */
#define SCC_GSMR_L              ((unsigned long)(0x00028004))
/*16x clock mode on transmit and receive UART mode, TX and RX DISABLED */

/*Force only ful stop bits, no half stop bits */
#define SCC_DSR                 ((unsigned short)(0x7e7e))

/*Defines for the SCC PSMR in UART mode */
#define SCC_TWO_STOP_BITS       ((unsigned short)(0x0001 << 15))
#define SCC_PARITY_ENABLE       ((unsigned short)(0x0001 <<  4))
#define SCC_EVEN_PARITY         ((unsigned short)(0x000A))
#define SCC_ODD_PARITY          ((unsigned short)(0x0000))

/* Defines for the SCC Event Register. */
#define SCCE_CLEAR              ((unsigned short)(0x1bef))

/*Defines for the SCC Trigger character recognition */
#define SCC_RCCM                ((unsigned short)(0xC0FF))
#define SCC_END_OF_CC_TABLE     ((unsigned short)(0x0001 << 15))
enum {NUMBER_OF_CONTROL_CHARS = 8};
#define SCC_CHAR_MASK           ((unsigned short)(0x00ff))

console_print_function_t        rs_quicc_scc_console_init(void);
int                             scc_mem_init(void);
int                             rs_quicc_scc_init(void);

/* Defines for transmit out of sequence. */
#define SCC_TOSEQ_READY         ((unsigned short)(0x0001 << 13))
extern int scc_console_baud_rate;
extern int scc_console_port;
#endif
#endif
#endif
