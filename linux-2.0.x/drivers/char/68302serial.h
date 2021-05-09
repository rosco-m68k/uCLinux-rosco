/* 68302serial.h: Definitions for the mc68302 serial driver.
 *
 * Based on:
 *
 * drivers/char/68328serial.h
 *
 */

#ifndef _MC68302_SERIAL_H
#define _MC68302_SERIAL_H

#include <linux/config.h>

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


#define BASE 0xF00000
//#define BASE 0xFFF000

#define VSP(X) (*(volatile unsigned short *)(X))
#define VCP(X) (*(volatile unsigned char *)(X))
#define VLP(X) (*(volatile unsigned long *)(X))


#define GIMR    VSP(BASE+0x812) /* Global Interrupt Mode Reg's Memory Address */
#define IMR		VSP(BASE+0x816) /* Interrupt Mask Reg'S Memory Address */
#define ISR		VSP(BASE+0x818)

#define IxR_SCC1	0x2000
#define IxR_SCC2	0x400
#define IxR_SCC3	0x100

#define SCCBASE BASE+0x880
#define SCCOFFSET 0x10

#define BDBASE BASE+0x400
#define BDOFFSET 0x100


/* SCC registers */

// Common SCC
#define SIMODE    VSP(BASE+0x8B4)
#define SIMASK    VSP(BASE+0x8B2)
#define SPMODE    VSP(BASE+0x8B0)
#define SCR		  VSP(BASE+0x860)
#define SMC2Rx    VSP(BASE+0x66A)
#define SMC1Rx    VSP(BASE+0x666)
#define SMC2Tx    VSP(BASE+0x66C)

#define BDSIZE	    4						// Each BD has 4 ushorts

	/* CR - command register */

#define	CR_RST								0x80
#define	CR_GCI								0x40
#define	CR_OP_STOP_TX						0x00
#define	CR_OP_REST_TX						0x10
#define	CR_OP_HUNT_MODE						0x20
#define	CR_OP_RST_BCS						0x30
#define	CR_SCC1								0x00
#define	CR_SCC2								0x02
#define	CR_SCC3								0x04
#define	CR_FLG								0x01


/* SCM - SCC Mode Register common bits */
#define SCCM_ENT	4
#define SCCM_ENR	8

/* SCM - SCC Mode Register UART bits */
#define SCM_CL		0x100
#define SCM_SL		0x40
#define SCM_PEN		0x1000
#define SCM_ODD		0
#define SCM_EVEN	0x8000

/* SCCE - SCC Event Register  */
#define SCCE_CTS	0x80
#define SCCE_CD		0x40
#define SCCE_IDLE	0x20
#define SCCE_BRK	0x10
#define SCCE_CCR	0x8
#define SCCE_BSY	0x4
#define SCCE_TX		0x2
#define SCCE_RX		0x1

/* BD RX control bits */
#define RX_BD_OV	0x2
#define RX_BD_PR	0x8
#define RX_BD_FR	0x10
#define RX_BD_BR	0x20

/*  SCC 0,1,2 */
#define SCON(X)   	VSP(SCCBASE+SCCOFFSET*(X)+2)  
#define SCM(X)    	VSP(SCCBASE+SCCOFFSET*(X)+4)
#define SCCE(X)		VCP(SCCBASE+SCCOFFSET*(X)+8)
#define SCCM(X)		VCP(SCCBASE+SCCOFFSET*(X)+0xA)

#define SCC_TXBD_1W(X,Y)  	VSP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x40)  
#define SCC_TXBD_2W(X,Y)  	VSP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x42)
#define SCC_TXBD_1L(X,Y)  	VLP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x44)
#define SCC_RXBD_1W(X,Y)  	VSP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x00) 
#define SCC_RXBD_2W(X,Y)  	VSP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x02)
#define SCC_RXBD_1L(X,Y)  	VLP(BDBASE+BDOFFSET*(X)+BDSIZE*(Y)+0x04)

#define SCC_MAX_IDL(X)  	VSP(BDBASE+BDOFFSET*(X)+0x9C)
#define SCC_BRKCR(X)		VSP(BDBASE+BDOFFSET*(X)+0xA0)
#define SCC_PAREC(X)		VSP(BDBASE+BDOFFSET*(X)+0xA2)
#define SCC_FRMEC(X)		VSP(BDBASE+BDOFFSET*(X)+0xA4)
#define SCC_NOSEC(X)		VSP(BDBASE+BDOFFSET*(X)+0xA6)
#define SCC_BRKEC(X)		VSP(BDBASE+BDOFFSET*(X)+0xA8) 
#define SCC_MRBLR(X)		VSP(BDBASE+BDOFFSET*(X)+0x82)
#define SCC_RFCR(X)			VSP(BDBASE+BDOFFSET*(X)+0x80)
#define SCC_CARACT(X)		VSP(BDBASE+BDOFFSET*(X)+0xB0)
#define SCC_UADDR1(X)		VSP(BDBASE+BDOFFSET*(X)+0xAA)
#define SCC_UADDR2(X)		VSP(BDBASE+BDOFFSET*(X)+0xAC)

#define MAX_BUFFER_SIZE		0xFFFF

// port A 
#define PACNT    VSP(BASE+0x81E)
#define PADDR    VSP(BASE+0x820)
#define PADAT    VSP(BASE+0x822)

/* port B */
#define PBCNT   	VSP(BASE+0x824)
#define PBDDR   	VSP(BASE+0x826) 
#define PBDAT   	VSP(BASE+0x828)

#endif /* !(_MC68302_SERIAL_H) */
