/* 68kserial.h: Definitions for the Sparc Zilog serial driver.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */
#ifndef _MC683XX_SERIAL_H
#define _MC683XX_SERIAL_H
#define CONFIG_MC68328


struct serial_struct {
	int	type;
	int	line;
	int	port;
	int	irq;
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

struct m68k_serial {
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
	int baud;
	int			magic;
	int			baud_base;
	int			port;
	int			irq;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
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


#define SERIAL_MAGIC 0x5301

/*
 * The size of the serial xmit buffer is 1 page, or 4096 bytes
 */
#define SERIAL_XMIT_SIZE 4096

/*
 * Events are used to schedule things to happen at timer-interrupt
 * time, instead of at rs interrupt time.
 */
#define RS_EVENT_WRITE_WAKEUP	0

#endif /* __KERNEL__ */

#ifdef CONFIG_MC68328

#define BASE 0xfffff900
#define VSP(X) (*(volatile unsigned short *)(X))
#define VCP(X) (*(volatile unsigned char *)(X))

/* UART status and control */
#define USTCNT VSP(BASE)
#define USTCNT_EN		(1 << 15)
#define USTCNT_RXEN	(1 << 14)
#define USTCNT_TXEN	(1 << 13)
#define USTCNT_CLKMODE	(1 << 12)
#define USTCNT_PARITY	(1 << 11)
#define USTCNT_ODDEVEN	(1 << 10)
#define USTCNT_STOP	(1 <<  9)
#define USTCNT_WLEN	(1 <<  8)

#define USTCNT_CTSEN	(1 <<  6)
#define USTCNT_RXFULLEN	(1 <<  5)
#define USTCNT_RXHALFEN	(1 <<  4)
#define USTCNT_READYEN	(1 <<  3)
#define USTCNT_TXEMPTYEN	(1 <<  2)
#define USTCNT_TXHALFEN	(1 <<  1)
#define USTCNT_TXAVAILEN	(1 <<  0)

/* UART baud control */
#define UBAUD VSP(BASE+2)
#define UBAUD_GPIOF	(1 << 15)
#define UBAUD_GPIOSC	(1 << 14)
#define UBAUD_GPIODDR	(1 << 13)
#define UBAUD_GPIOSRC	(1 << 12)
#define UBAUD_BAUDSRC	(1 << 11)
#define UBAUD_DIVMASK	(0x7 << 8)
#define UBAUD_GETDIV(X)	((0x7 & (X)) >> 8)
#define UBAUD_SETDIV(X)	((0x7 & (X)) << 8)
#define UBAUD_PRESCALEMASK	(0x3f << 0)
#define UBAUD_PRESCALE(X)	((0x3f & (X)) << 0)

/* UART RX and status */
#define URX VSP(BASE + 4)
#define URX_FIFOFULL	(1<<15)
#define URX_FIFOHALF	(1<<14)
#define URX_DATARDY	(1<<13)
#define URX_OVERRUN	(1<<11)
#define URX_FRAMEERR	(1<<10)
#define URX_BREAK	(1<< 9)
#define URX_PARITYERR	(1<< 8)
#define URX_CHARSTAT	(URX_OVERRUN |URX_FRAMEERR |URX_BREAK |URX_PARITYERR)
#define URX_CHARMASK	(0xff << 0)
#define URX_CHAR(X)	((0xff & (X)) << 0)

/* UART TX and status */
#define UTX VCP(BASE + 6)
#define UTX_FIFOEMPTY	(1<< 7)
#define UTX_FIFOHALF	(1<< 6)
#define UTX_TXAVAIL	(1<< 5)
#define UTX_SENDBREAK	(1<< 4)
#define UTX_IGNCTS	(1<< 3)
#define UTX_CTSSTAT	(1<< 1)
#define UTX_CTSDELTA	(1<< 0)

#define UTX_CHARMASK	(0xff << 0)
#define UTX_CHAR VCP(BASE + 7)

/* UART misc */
#define UMISC VSP(BASE + 8)
#define UMISC_CLKSRC	(1<<14)
#define UMISC_GENPTERR	(1<<13)
#define UMISC_LOOP	(1<<12)
#define UMISC_RTSCTL	(1<< 7)
#define UMISC_RTS	(1<< 6)
#define UMISC_IRDAEN	(1<< 5)
#define UMISC_LOOPIR	(1<< 4)
#define UMISC_RXPOL	(1<< 3)
#define UMISC_TXPOL	(1<< 2)
#endif /* CONFIG_MC68328 */

#endif /* !(_MC683XX_SERIAL_H) */
