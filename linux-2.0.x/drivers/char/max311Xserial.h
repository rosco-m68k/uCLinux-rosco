/* max311Xserial.h: Definitions for the MAX311X serial driver.
 *
 * Copyright (C) 1995       David S. Miller    <davem@caip.rutgers.edu>
 * Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 * Copyright (C) 1998, 1999 D. Jeff Dionne     <jeff@rt-control.com>
 * Copyright (C) 1999       Vladimir Gurevich  <vgurevic@cisco.com>
 * Copyright (C) 1999       David Beckemeyer   <david@bdt.com>
 * 
 *
 */

#ifndef _MAX311X_SERIAL_H
#define _MAX311X_SERIAL_H

#include <linux/config.h>

struct serial_struct {
	int	type;
	int	line;
	int	flags;
	int	xmit_fifo_size;
	int	custom_divisor;
	int	baud_base;
	unsigned short	close_delay;
	char	reserved_char[2];
	int	hub6;  /* FIXME: We don't have AT&T Hub6 boards! */
	unsigned short	closing_wait; /* time to wait before closing */
	unsigned short	closing_wait2; /* no longer used... */
	int	reserved[4];
};

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

/* Software state per channel */

#ifdef __KERNEL__


/*
 * This is our internal structure for each serial port's state.
 * 
 * Many fields are paralleled by the structure used by the serial_struct
 * structure.
 *
 * For definitions of the flags field, see tty.h
 */

struct max311x_serial {
	char soft_carrier;  /* Use soft carrier on this channel */
	char break_abort;   /* Is serial console in, so process brk/abrt */

	unsigned config;    /* last written config register setting */

	int baud;
	int			magic;
	int			baud_base;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
	struct tty_struct 	*tty;
	int			read_status_mask;
	int			ignore_status_mask;
	int			timeout;
	int			xmit_fifo_size;
	int			x_char;	/* xon/xoff character */
	int			close_delay;
	unsigned short		closing_wait;
	unsigned short		closing_wait2;
	unsigned long		event;
	unsigned long		last_active;
	int			line;
	int			count;	    /* # of fd on device */
	int			blocked_open; /* # of blocked opens */
	int			blocked_flush;
	long			session; /* Session of opening process */
	long			pgrp; /* pgrp of opening process */
	unsigned char 		*xmit_buf;
	int			xmit_head;
	int			xmit_tail;
	int			xmit_cnt;
	int			cs_line;
	struct tq_struct	tqueue;
	struct tq_struct	tqueue_hangup;
	struct termios		normal_termios;
	struct wait_queue	*open_wait;
	struct wait_queue	*close_wait;
	struct wait_queue	*flush_wait;
};


#define SERIAL_MAGIC 0x4C4A

/*
 * The size of the serial xmit buffer is 1 page, or 4096 bytes
 */
#define SERIAL_XMIT_SIZE 4096

/*
 * Events are used to schedule things to happen at timer-interrupt
 * time, instead of at rs interrupt time.
 */
#define RSMAX_EVENT_WRITE_WAKEUP	0

/* 
 * Assumes MAX3111 UARTs wired to uCsimm as follows:
 *
 * First Max3111:
 *
 *   uCsimm    MAX3111
 *
 *    INT0  <---  IRQ
 *    STXD  --->  DIN
 *    SRXD  <---  DOUT
 *    SCLK  --->  SCLK
 *    PD4   --->  CS
 *
 * Second Max3111:
 *
 *   uCsimm    MAX3111
 *
 *    INT1  <---  IRQ
 *    STXD  --->  DIN
 *    SRXD  <---  DOUT
 *    SCLK  --->  SCLK
 *    PD3   --->  CS
 */


#define MAX3111_CS0 0x10                        // PD4
#define MAX3111_CS1 0x08                        // PD3

#define MAX3111_IRQ0 0x01                       // PD0/INT0
#define MAX3111_IRQ1 0x02                       // PD0/INT0

/* MAX3111 config write register bits */
#define  MAX3111_WC_R 0x8000
#define  MAX3111_WC_T 0x4000
#define  MAX3111_WC_FEN 0x2000
#define  MAX3111_WC_SHDN 0x1000
#define  MAX3111_WC_TM 0x0800
#define  MAX3111_WC_RM 0x0400
#define  MAX3111_WC_PM 0x0200
#define  MAX3111_WC_RAM 0x0100
#define  MAX3111_WC_IR 0x0080
#define  MAX3111_WC_ST 0x0040
#define  MAX3111_WC_PE 0x0020
#define  MAX3111_WC_L 0x0010

/* MAX3111 read data status bits */
#define  MAX3111_RD_R 0x8000
#define  MAX3111_RD_T 0x4000
#define  MAX3111_RD_FE 0x0400
#define  MAX3111_RD_CTS 0x0200
#define  MAX3111_RD_PR 0x0100

/* bit of a hack defining the MAJOR here, should probably be in major.h */
#define MAX3111_MAJOR 82

#endif /* __KERNEL__ */
#endif /* !(_MAX311X_SERIAL_H) */
