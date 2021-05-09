/*
 *  Internal constants used for both SMCs and SCCs on the 68360's CPM.
 *
 *  Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *      <hamilton@sedsystems.ca>
 *
 *  Based on mcfserial.h which was:
 *  Copyright (C) 1999, Greg Ungerer (greg@snapgear.com)
 *  Copyright (C) 2000, Lineo Inc.   (www.lineo.com)
 *
 */
#ifndef __QUICC_SMC_I_H__
#define __QUICC_SMC_I_H__

#include <linux/config.h>
#ifdef __KERNEL__
#if defined(CONFIG_M68360)
#include <linux/serial.h>
#include <asm/m68360.h>
#include <asm/quicc_cpm_i.h>
#include <asm/quicc_sxc_i.h>

struct quicc_smc_serial_t
{
        int                             magic;
        struct smc_uart_pram            *smc_uart_ram;
        struct smc_regs                 *smc_registers;
        sxc_list_t                      smc;
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
        unsigned long                   pip_pbdir;
        unsigned long                   pip_pbpar;
        unsigned short                  pip_pbodr;
};

/* Interrupt sources on the SMC */
#define SMC_INT_BREAK_RECEIVED  ((unsigned char)(0x01 << 4))
#define SMC_INT_NO_BUFFERS      ((unsigned char)(0x01 << 2))
#define SMC_INT_TX_DONE         ((unsigned char)(0x01 << 1))
#define SMC_INT_RX_DATA         ((unsigned char)(0x01 << 0))

//Defines for configuring port b
#define PORTB_SMC1_SET_32       ((unsigned long)(0x000000C0))
#define PORTB_SMC1_CLEAR_32     ((unsigned long)(0xFFFFFF3F))
#define PORTB_SMC1_SET_16       ((unsigned short)(0x00C0))
#define PORTB_SMC1_CLEAR_16     ((unsigned short)(0xFF3F))

#define PORTB_SMC2_SET_32       ((unsigned long)(0x00000C00))
#define PORTB_SMC2_CLEAR_32     ((unsigned long)(0xFFFFF3FF))
#define PORTB_SMC2_SET_16       ((unsigned short)(0x0C00))
#define PORTB_SMC2_CLEAR_16     ((unsigned short)(0xF3FF))

//Defines for the SMCMR
#define SMCMR_TWO_STOP_BITS     ((unsigned short)(0x0001 << 10))
#define SMCMR_PARITY_ENABLE     ((unsigned short)(0x0001 <<  9))
#define SMCMR_EVEN_PARITY       ((unsigned short)(0x0001 <<  8))
#define SMCMR_UART_MODE         ((unsigned short)(0x0002 <<  4))
#define SMCMR_TX_ENABLE         ((unsigned short)(0x0001 <<  1))
#define SMCMR_RX_ENABLE         ((unsigned short)(0x0001 <<  0))

console_print_function_t        rs_quicc_smc_console_init(void);
int                             smc_mem_init(void);
int                             rs_quicc_smc_init(void);

extern int smc_console_baud_rate;
extern int smc_console_port;
#endif
#endif
#endif
