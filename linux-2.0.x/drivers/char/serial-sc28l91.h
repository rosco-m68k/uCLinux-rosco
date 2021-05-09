/* serial-sc28l91.h: Definitions for the Philips SC28L91 serial driver.
 *
 * Copyright (C) 2001  Erik Andersen <andersen@lineo.com>
 *
 * Based on:
 *
 * drivers/char/serial-atmel.[ch]
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#ifndef _PHILIPS_SERIAL_H
#define _PHILIPS_SERIAL_H

#include <linux/config.h>
#include <asm/page.h>
#include <asm/arch/hardware.h>

/* This is the register to name mapping for struct uart_sc28l91 for
 * the Philips UART.  
 */
#define	MR			0
#define	CSR			1
#define	SR			1
#define	CR			2
#define	RxFIFO		3
#define	TxFIFO		3
#define	IPCR		4
#define	ACR			4
#define	ISR			5
#define	IMR			5
#define	CTU			6
#define	CTPU		6
#define	CTL			7
#define	CTPL		7
#define	RESERVED_0	8
#define	RESERVED_1	9
#define	RESERVED_2	10
#define	RESERVED_3	11
#define	RESERVED_4	12
#define	IPR			13
#define	OPCR		13
#define	StrtCT		14
#define	SOPR		14
#define	StpCT		15
#define	ROPR		15

// status register (SR) flags
#define RxRDY 0x01
#define TxRDY 0x04
#define RxFULL 0x20
#define TxEMT 0x08

typedef volatile unsigned char * uart_sc28l91;


/*
 * For the close wait times, 0 means wait forever for serial port to
 * flush its output.  65535 means don't wait at all.
 */
#define S_CLOSING_WAIT_INF	0
#define S_CLOSING_WAIT_NONE	65535

/*
 * Definitions for S_struct (and serial_struct) flags field
 */
#define S_HUP_NOTIFY 0x0001 /* Notify getty on hangups and closes 
				   on the callout port */
#define S_FOURPORT  0x0002	/* Set OU1, OUT2 per AST Fourport settings */
#define S_SAK	0x0004	/* Secure Attention Key (Orange book) */
#define S_SPLIT_TERMIOS 0x0008 /* Separate termios for dialin/callout */

#define S_SPD_MASK	0x0030
#define S_SPD_HI	0x0010	/* Use 56000 instead of 38400 bps */

#define S_SPD_VHI	0x0020  /* Use 115200 instead of 38400 bps */
#define S_SPD_CUST	0x0030  /* Use user-specified divisor */

#define S_SKIP_TEST	0x0040 /* Skip UART test during autoconfiguration */
#define S_AUTO_IRQ  0x0080 /* Do automatic IRQ during autoconfiguration */
#define S_SESSION_LOCKOUT 0x0100 /* Lock out cua opens based on session */
#define S_PGRP_LOCKOUT    0x0200 /* Lock out cua opens based on pgrp */
#define S_CALLOUT_NOHUP   0x0400 /* Don't do hangups for cua device */

#define S_FLAGS	0x0FFF	/* Possible legal S flags */
#define S_USR_MASK 0x0430	/* Legal flags that non-privileged
				 * users can set or reset */

/* Internal flags used only by kernel/chr_drv/serial.c */
#define S_INITIALIZED	0x80000000 /* Serial port was initialized */
#define S_CALLOUT_ACTIVE	0x40000000 /* Call out device is active */
#define S_NORMAL_ACTIVE	0x20000000 /* Normal device is active */
#define S_BOOT_AUTOCONF	0x10000000 /* Autoconfigure port on bootup */
#define S_CLOSING		0x08000000 /* Serial port is closing */
#define S_CTS_FLOW		0x04000000 /* Do CTS flow control */
#define S_CHECK_CD		0x02000000 /* i.e., CLOCAL */
/*
 * This is our internal structure for each serial port's state.
 * 
 * Many fields are paralleled by the structure used by the serial_struct
 * structure.
 *
 * For definitions of the flags field, see tty.h
 */

struct sc28l91_serial {
	char soft_carrier;  /* Use soft carrier on this channel */
	char break_abort;   /* Is serial console in, so process brk/abrt */
#if 0
	char cons_keyb;     /* Channel runs the keyboard */
	char cons_mouse;    /* Channel runs the mouse */
	char kgdb_channel;  /* Kgdb is running on this channel */
#endif
	char is_cons;       /* Is this our console. */

	/* We need to know the current clock divisor
	 * to read the bps rate the chip has currently
	 * loaded.
	 */
	unsigned char clk_divisor;  /* May be 1, 16, 32, or 64 */
	int			baud;
	int			magic;
	int			baud_base;
	int			port;
	int			irq;
	int			irqmask;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
	int			use_ints;
	uart_sc28l91	uart;
	int			cts_state;
	struct tty_struct 	*tty;
	int			read_status_mask;
	int			ignore_status_mask;
	int			timeout;
	int			xmit_fifo_size;
	int			custom_divisor;
	int			x_char;	/* xon/xoff character */
	int			close_delay;
	unsigned short		closing_wait;
	unsigned short		closing_wait2;
	unsigned long		event;
	unsigned long		last_active;
	int			line;
	int			count;	    /* # of fd on device */
	int			blocked_open; /* # of blocked opens */
	long			session; /* Session of opening process */
	long			pgrp; /* pgrp of opening process */
	unsigned char 		*xmit_buf;
	unsigned char 		*rx_buf;
	int			xmit_head;
	int			xmit_tail;
	int			xmit_cnt;
	struct tq_struct	tqueue;
	struct tq_struct	tqueue_hangup;
	struct termios		normal_termios;
	struct termios		callout_termios;
	struct wait_queue	*open_wait;
	struct wait_queue	*close_wait;
};


#endif /* _PHILIPS_SERIAL_H */ 
