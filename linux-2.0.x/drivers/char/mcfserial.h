/*
 * mcfserial.h -- Definitions for the ColdFire serial driver.
 *
 * Copyright (C) 1999, Greg Ungerer (gerg@snapgear.com)
 * (C) Copyright 2000, Lineo Inc. (www.lineo.com) 
 *
 * Based on 68332serial.h, which was:
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1998 TSHG
 */
#ifndef _MCF_SERIAL_H
#define _MCF_SERIAL_H

#include <linux/config.h>
#include <linux/serial.h>

#ifdef __KERNEL__

/*
 *	Define a local serial stats structure.
 */

struct mcf_stats {
	unsigned int	rx;
	unsigned int	tx;
	unsigned int	rxbreak;
	unsigned int	rxframing;
	unsigned int	rxparity;
	unsigned int	rxoverrun;
};


/*
 * This is our internal structure for each serial port's state.
 * Each serial port has one of these structures associated with it.
 */

struct mcf_serial {
	int			magic;
	unsigned int		addr;		/* UART memory address */
	int			irq;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
	struct tty_struct 	*tty;
	unsigned char		imr;		/* Software imr register */
	unsigned int		baud;
	int			sigs;
	int			custom_divisor;
	int			x_char;	/* xon/xoff character */
	int			baud_base;
	int			close_delay;
	unsigned short		closing_wait;
	unsigned short		closing_wait2;
	unsigned long		event;
	int			line;
	int			count;	    /* # of fd on device */
	int			blocked_open; /* # of blocked opens */
	long			session; /* Session of opening process */
	long			pgrp; /* pgrp of opening process */
	unsigned char 		*xmit_buf;
	int			xmit_head;
	int			xmit_tail;
	int			xmit_cnt;
	struct mcf_stats	stats;
	struct tq_struct	tqueue;
	struct tq_struct	tqueue_hangup;
	struct termios		normal_termios;
	struct termios		callout_termios;
	struct wait_queue	*open_wait;
	struct wait_queue	*close_wait;
};

#endif /* __KERNEL__ */

#endif /* _MCF_SERIAL_H */
