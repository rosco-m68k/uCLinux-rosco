/*****************************************************************************/
/* 
 *  pd72001_serial.c, v1.0 <2003-07-28 16:37:51 gc>
 * 
 *  linux/drivers/char/pd72001_serial.c
 *
 *  uClinux version 2.0.x NEC uPD72001 MPSC serial driver
 *
 *  Author:     Guido Classen (classeng@clagi.de)
 *
 *  This program is free software;  you can redistribute it and/or modify it
 *  under the  terms of the GNU  General Public License as  published by the
 *  Free Software Foundation.  See the file COPYING in the main directory of
 *  this archive for more details.
 *
 *  This program  is distributed  in the  hope that it  will be  useful, but
 *  WITHOUT   ANY   WARRANTY;  without   even   the   implied  warranty   of
 *  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR  PURPOSE.   See the  GNU
 *  General Public License for more details.
 *
 *  Thanks to:
 *    some initial code taken from sunserial.c by
 *    David S. Miller (davem@caip.rutgers.edu) 
 *
 *      (this driver is for an similar  chip, but it is not fully compatible
 *      to the NEC uPD72001)
 *
 *  Change history:
 *       2003-07-28 gc: initial version
 *
 */
 /****************************************************************************/

#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/config.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/delay.h>

#include <asm/traps.h> /* VEC_SPUR */

#include "pd72001.h"

#ifndef CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 9600
#endif

#define CONSOLE_PORT 0


/* Debugging... DEBUG_INTR is bad to use when one of the zs
 * lines is your console ;(
 */
#undef SERIAL_DEBUG_INTR
#undef SERIAL_DEBUG_OPEN
#undef SERIAL_DEBUG_FLOW
#undef SERIAL_DEBUG_THROTTLE


/* 
 * #define SERIAL_DEBUG_INTR 1
 * #define SERIAL_DEBUG_OPEN 1
 * #define SERIAL_DEBUG_FLOW 1
 */

#define MPSC_DEBUG_LEVEL 5
/* #define MPSC_DEBUG(level, args...) if ((level) <= MPSC_DEBUG_LEVEL) printk("mpsc dbg: " args) */
#define MPSC_DEBUG(level, args...) 

/*
 * For the close wait times, 0 means wait forever for serial port to
 * flush its output.  65535 means don't wait at all.
 */
#define MPSC_CLOSING_WAIT_INF	0
#define MPSC_CLOSING_WAIT_NONE	65535


/*
 * Definitions for MPSC_struct (and serial_struct) flags field
 */
#define MPSC_HUP_NOTIFY         0x0001  /* Notify getty on hangups and closes 
                                           on the callout port */
#define MPSC_FOURPORT           0x0002	/* Set OU1, OUT2 per AST Fourport 
                                           settings */
#define MPSC_SAK                0x0004	/* Secure Attention Key (Orange book)*/
#define MPSC_SPLIT_TERMIOS      0x0008  /* Separate termios for 
                                           dialin/callout */

#define MPSC_SPD_MASK           0x0030
#define MPSC_SPD_HI             0x0010	/* Use 56000 instead of 38400 bps */

#define MPSC_SPD_VHI            0x0020  /* Use 115200 instead of 38400 bps */
#define MPSC_SPD_CUST           0x0030  /* Use user-specified divisor */

#define MPSC_SKIP_TEST          0x0040  /* Skip UART test during 
                                           autoconfiguration */
#define MPSC_AUTO_IRQ           0x0080  /* Do automatic IRQ during 
                                           autoconfiguration */
#define MPSC_SESSION_LOCKOUT    0x0100  /* Lock out cua opens based on 
                                           session */
#define MPSC_PGRP_LOCKOUT       0x0200  /* Lock out cua opens based on pgrp */
#define MPSC_CALLOUT_NOHUP      0x0400  /* Don't do hangups for cua device */

#define MPSC_FLAGS              0x0fff	/* Possible legal MPSC flags */
#define MPSC_USR_MASK           0x0430	/* Legal flags that non-privileged
                                         * users can set or reset */

/* Internal flags used only by kernel/chr_drv/serial.c */
#define MPSC_INITIALIZED	0x80000000 /* Serial port was initialized */
#define MPSC_CALLOUT_ACTIVE	0x40000000 /* Call out device is active */
#define MPSC_NORMAL_ACTIVE	0x20000000 /* Normal device is active */
#define MPSC_BOOT_AUTOCONF	0x10000000 /* Autoconfigure port on bootup */
#define MPSC_CLOSING		0x08000000 /* Serial port is closing */
#define MPSC_CTS_FLOW		0x04000000 /* Do CTS flow control */
#define MPSC_CHECK_CD		0x02000000 /* i.e., CLOCAL */

/*
 * The size of the serial xmit buffer is 1 page, or 4096 bytes
 */
#define SERIAL_XMIT_SIZE        4096
#define SERIAL_MAGIC            0x5301

/*
 * Events are used to schedule things to happen at timer-interrupt
 * time, instead of at rs interrupt time.
 */
#define MPSC_EVENT_WRITE_WAKEUP	0


/* Misc macros */
/* #define MPSC_CLEARERR(mpsc)      MPSC_WRITE_COMMAND(mpsc, MPSC_CR0_CMD_RESET_ERROR); */

#define MPSC_CLEARFIFO(mpsc)   do { volatile unsigned char garbage; \
				     garbage = MPSC_READ_DATA(mpsc); \
				     udelay(2); \
				     garbage = MPSC_READ_DATA(mpsc); \
				     udelay(2); \
				     garbage = MPSC_READ_DATA(mpsc); \
				     udelay(2); } while(0)

struct mpsc_chip_info {
        Mpsc mpsca;     /* first channel of the chip (may be equal mpsc) */
        Mpsc mpscb;     /* second channel of the (may be equal mpsc) */
        u16 int_num;    /* interrupt vector number */
};

/* hardware configuration of the serial ports */
struct mpsc_channel_info {
        u8 chip;           /* number of chip (index of mpsc_chips) */
        u8 channel;        /* channel of chip (0=A, 1=B)   */
        u16 ledTx;         /* LED for TX 0=no TX-LED       */
        u16 ledRx;         /* LED for RX 0=no RX-LED       */
};

#ifdef CONFIG_SM2010
#include "../../arch/m68knommu/platform/68000/SM2010/sm2010_ser_conf.h"
#else
#error "please define your own board configuration for the PD72001 driver"
#endif

/* user mode serial struct */
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
	int	hub6;
	unsigned short	closing_wait; /* time to wait before closing */
	unsigned short	closing_wait2; /* no longer used... */
	int	reserved[4];
};

struct mpsc_serial
{
        u8 used;

	char break_abort;   /* Is serial console in, so process brk/abrt */
	char kgdb_channel;  /* Kgdb is running on this channel */
	char is_cons;       /* Is this our console. */


	char                    change_needed;

	int			magic;
	int			baud_base;
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


        Mpsc mpsc;              /* channel used by this driver instance */
        Mpsc mpsca;             /* first channel of the chip 
                                   (may be equal mpsc) */
        Mpsc mpscb;             /* second channel of the (may be equal mpsc) */
        struct mpsc_serial      *slave_device;
        const struct mpsc_channel_info *channel_info;
        
        u16 channel;            /* 0 = A; 1 = B */
        u8 cr[16];              /* values of hw write register */

        u32 baudrate;
        u16 ledTx;
        u16 ledRx;
        u16 ledTxTimer;
        u16 ledRxTimer;
};

struct mpsc_serial *console_driver;

#define NUM_CHANNELS            (NUM_SERIAL * 2)
struct mpsc_serial mpsc_soft[NUM_CHANNELS];
struct tty_struct mpsc_ttys[NUM_CHANNELS];


/* on time for LED's to 80ms */
#define LED_TIMEOUT             80 * (HZ/1000)

/*
 * This is used to figure out the divisor speeds and the timeouts
 */
static const int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 0 
};



DECLARE_TASK_QUEUE(tq_serial);

struct tty_driver serial_driver, callout_driver;
static int serial_refcount;

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2
  
/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS            256


#define MPSC_STROBE_TIME        10
#define MPSC_ISR_PASS_LIMIT     256

#define _INLINE_                inline

static struct tty_struct *mpsc_tty_table[NUM_CHANNELS];
static struct termios *mpsc_termios[NUM_CHANNELS];
static struct termios *mpsc_termios_locked[NUM_CHANNELS];

#ifndef MIN
#define MIN(a,b)                ((a) < (b) ? (a) : (b))
#endif

/*
 * tmp_buf is used as a temporary buffer by serial_write.  We need to
 * lock it in case the memcpy_fromfs blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
static unsigned char tmp_buf[SERIAL_XMIT_SIZE];
static struct semaphore tmp_buf_sem = MUTEX;

u16 panel_led_state = 0;



static inline int mpsc_paranoia_check(struct mpsc_serial *info,
					dev_t device, const char *routine)
{
#ifdef MPSC_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%d, %d) in %s\n";
	static const char *badinfo =
		"Warning: null mpsc_serial for (%d, %d) in %s\n";

	if (!info) {
		printk(badinfo, MAJOR(device), MINOR(device), routine);
		return 1;
	}
	if (info->magic != SERIAL_MAGIC) {
		printk(badmagic, MAJOR(device), MINOR(device), routine);
		return 1;
	}
#endif
#ifdef  SERIAL_DEBUG_OPEN
        if (routine)
                printk("mpsc_paranoia_check: %s\n", routine);
#endif

	return 0;
}

/* this function is called by the initialization of the SM2010 */
static void mpsc_initialize_memory(void)
{
        memset(mpsc_soft, 0, sizeof(mpsc_soft));
}


static void mpsc_start_led_timer(u16 *timer)
{
        *timer = (u16) jiffies + LED_TIMEOUT;
        /* 0 = timer disabled */
        if (! *timer) *timer = 1;
}


static unsigned mpsc_get_cflag_for_baud_rate(long int baudrate)
{
        int i;
        switch (baudrate) {
        case 57600: 
                return B57600;
                
        case 115200:
                return B115200;
                
        case 230400:
                return B230400;

        case 460800:
                return B460800;

        default:

                for (i=0; i<sizeof(baud_table)/sizeof(baud_table[0]); ++i) {
                        if (baud_table[i] == baudrate) {
                                return i;
                        }
                }
        }
        /* unknown baudrate */
        return B9600;
}
 


static void mpsc_hw_initialize(register struct mpsc_serial *info)
{
        u8 temp;
        unsigned long flags;
        unsigned cflag;
        int i;
        u16 divisor;

	if (info->tty && info->tty->termios) {
                cflag = info->tty->termios->c_cflag;
        } else {
                cflag = mpsc_get_cflag_for_baud_rate(info->baudrate) | CS8;
        }
	i = cflag & CBAUD;
	if (i & CBAUDEX) {
                switch (i) {
                case B57600: 
                        info->baudrate = 57600;
                        break;

                case B115200:
                        info->baudrate = 115200;
                        break;

                case B230400:
                        info->baudrate = 230400;
                        break;

#if 0
                        /* over 8% difference to actual baudrate => not use */
                case B460800:
                        info->baudrate = 460800;
                        break;
#endif

                default:
                        /* don't know which baud rate wantet */
                        i = B9600;
                        info->baudrate = 9600;
                        printk("mpsc_serial: unsupported baudrate!\n");

                }
        } else {
                info->baudrate = baud_table[i];
        }

        save_flags(flags); cli();

#if 1
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_CHANNEL_RESET);
#endif
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_CHANNEL_RESET);


        info->cr[1] = 0;       /* ??? disable ints */
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1, info->cr[1]);               



        info->cr[2] = 0x00;    /* interrupt vector not used = 0 */

        MPSC_WRITE_REG(info->mpsca, MPSC_CR2, 
                       MPSC_CR2A_STATUS_AFFECTS_VECTOR 
                       | MPSC_CR2A_OUTPUT_VECTOR_TYPE_B1
                       | MPSC_CR2A_INT_DMA_MODE_BOTH_INT );



        MPSC_WRITE_REG(info->mpscb, MPSC_CR2, info->cr[2]);               



        if (info->baudrate == 14400 ||
            info->baudrate == 28800 ||
            info->baudrate >= 57600) {
                /* at 14400, 28800 und 57600 6 MHz and X1 Clock */
                info->cr[4] = MPSC_CR4_CLOCKRATE_X1;
        } else {
                /* use X16 clock */
                info->cr[4] = MPSC_CR4_CLOCKRATE_X16;
        }

	if (cflag & PARENB) {
                if (cflag & PARODD) {
                        info->cr[4] |= MPSC_CR4_PARITY_ENABLE;
                } else {
                        info->cr[4] |= MPSC_CR4_PARITY_ENABLE 
                                | MPSC_CR4_PARITY_EVEN;
                }
	} 

	if (cflag & CSTOPB) {
                info->cr[4] |= MPSC_CR4_STOPBITS_2;
	} else {
                info->cr[4] |= MPSC_CR4_STOPBITS_1;
	}

        MPSC_WRITE_REG(info->mpsc, MPSC_CR4, info->cr[4]);


        /* register 14 */
        /* rx tx brg disable with systemclock */
        /*  info->cr[14] = MPSC_CR14_BRG_SOURCE_SYS_CLOCK; */

        if (info->baudrate == 14400 ||
            info->baudrate == 28800 ||
            info->baudrate >= 57600) {
    

                divisor = ((MPSC_SIO_CLOCK_XTAL / 2UL) 
                           / (unsigned long) info->baudrate) - 2UL;
                info->baudrate = (MPSC_SIO_CLOCK_XTAL / 2UL) 
                        / (divisor + 2); /* calculate actual set baudrate */
                info->cr[14] = MPSC_CR14_BRG_SOURCE_XTAL 
                        | MPSC_CR14_RX_BRG_ENABLE
                        | MPSC_CR14_TX_BRG_ENABLE;

        } else {

                divisor = ((MPSC_SIO_CLOCK_SYS / 2UL / 16UL) / 
                           (unsigned long) info->baudrate) - 2UL;
                info->baudrate = (MPSC_SIO_CLOCK_SYS / 2UL / 16UL) 
                        / (divisor + 2); /* calculate actual set baudrate */

                info->cr[14] = MPSC_CR14_BRG_SOURCE_SYS_CLOCK
                        | MPSC_CR14_RX_BRG_ENABLE
                        | MPSC_CR14_TX_BRG_ENABLE;
        }
        printk("mpsc_serial: actual set baudrate: %d!\n", info->baudrate);


        /*  MPSC_WRITE_REG(info->mpsc, MPSC_CR14, info->cr[14]); */

        /* TX clock */
        info->cr[12] = MPSC_CR12_TX_BRG_REGISTER_SET;  
        MPSC_WRITE_REG(info->mpsc, MPSC_CR12, info->cr[12]);
        MPSC_WRITE_CONTROL(info->mpsc, divisor & 0xff);
        MPSC_WRITE_CONTROL(info->mpsc, (divisor >> 8) & 0xff);
        temp = MPSC_READ_CONTROL(info->mpsca); /* dummy read on a channel !!! */  
        /* RX clock */
        info->cr[12] = MPSC_CR12_TX_BRG_FOR_TRXC 
                | MPSC_CR12_RX_BRG_REGISTER_SET;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR12, info->cr[12]);
        MPSC_WRITE_CONTROL(info->mpsc, divisor & 0xff);
        MPSC_WRITE_CONTROL(info->mpsc, (divisor >> 8) & 0xff);
        temp = MPSC_READ_CONTROL(info->mpsca);
  
        info->cr[15] =   MPSC_CR15_TRXC_SOURCE_BRG_CLK
                | MPSC_CR15_TRXC_OUTPUT
                | MPSC_CR15_TXCLK_SELECT_TXBRG
                | MPSC_CR15_RXCLK_SELECT_RXBRG;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR15, info->cr[15]);

        MPSC_WRITE_REG(info->mpsc, MPSC_CR14, info->cr[14]);

        info->cr[10] = 0;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR10, info->cr[10]);

	switch (cflag & CSIZE) {
	case CS5:
                info->cr[3] |= MPSC_CR3_RX_BITS_5;
                info->cr[5] |= MPSC_CR5_TX_BITS_5;
		break;
	case CS6:
                info->cr[3] |= MPSC_CR3_RX_BITS_6;
                info->cr[5] |= MPSC_CR5_TX_BITS_6;
		break;
	case CS7:
                info->cr[3] |= MPSC_CR3_RX_BITS_7;
                info->cr[5] |= MPSC_CR5_TX_BITS_7;
		break;
	case CS8:
	default: /* defaults to 8 bits */
                info->cr[3] |= MPSC_CR3_RX_BITS_8;
                info->cr[5] |= MPSC_CR5_TX_BITS_8;
		break;
	}

        info->cr[3] |= MPSC_CR3_RX_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);

        info->cr[5] |= MPSC_CR5_TX_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);

        info->cr[11] = 0;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR11, info->cr[11]);

        info->cr[13] = 0;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR13, info->cr[13]);

#if 0
        /* reset Tx underrun/eom latch*/
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_TX_UNDERRUN_EOM);
        /* reset ext/status interrupt */
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_EXT_STAT_INT);
        /* reset TxINT pending        */  
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_TX_INT);
        /* error reset                */
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_ERROR);

#endif
        restore_flags(flags);
}



/*
 * This routine is called to setup the UART registers to match
 * the setting of the termios structure for this tty
 */
static void mpsc_hw_reinitialize(struct mpsc_serial *info)
{
        unsigned long flags;

        mpsc_hw_initialize(info);

        save_flags(flags); cli();

        /* enable mpsc RX and TX interrupt */
        info->cr[1] |= MPSC_CR1_RX_INT_ALL_1 | MPSC_CR1_TX_INT_ENABLE;

        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1 | MPSC_CR0_CMD_RESET_EXT_STAT_INT, 
                       info->cr[1]);

        restore_flags(flags);
}


/* Sets or clears DTR/RTS on the requested line */
static inline void mpsc_rtsdtr(struct mpsc_serial *info, int set)
{
        unsigned long flags;

	if(set) {
                save_flags(flags); cli();
                info->cr[5] |= MPSC_CR5_TX_RTS | MPSC_CR5_TX_DTR;
                MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
                restore_flags(flags);
	} else {
                save_flags(flags); cli();
                info->cr[5] &= ~MPSC_CR5_TX_RTS & ~MPSC_CR5_TX_DTR;
                MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
                restore_flags(flags);
	}
}

#if 0
static inline void kgdb_chaninit(struct mpsc_serial *ss, int intson, int bps)
{
}
#endif

/*
 * ------------------------------------------------------------
 * mpsc_stop() and mpsc_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 * ------------------------------------------------------------
 */
static void mpsc_stop(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
	unsigned long flags;

        MPSC_DEBUG(5, "%s\n", __FUNCTION__);
	if (mpsc_paranoia_check(info, tty->device, "mpsc_stop"))
		return;
	
	save_flags(flags); cli();

	if (info->cr[5] & MPSC_CR5_TX_ENABLE) {
		info->cr[5] &= ~MPSC_CR5_TX_ENABLE;
		/* info->pendregs[5] &= ~MPSC_CR5_TX_ENABLE; */
                MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
	}

	restore_flags(flags);
}

static void mpsc_start(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
	unsigned long flags;
	
        MPSC_DEBUG(5, "%s\n", __FUNCTION__);
	if (mpsc_paranoia_check(info, tty->device, "mpsc_start"))
		return;
	
	save_flags(flags); cli();

	if (info->xmit_cnt && info->xmit_buf 
            && !(info->cr[5] & MPSC_CR5_TX_ENABLE)) {
		info->cr[5] |= MPSC_CR5_TX_ENABLE;
                MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
	}

	restore_flags(flags);
}

/* On receive, this clears errors and the receiver interrupts */
static inline void mpsc_recv_clear(register Mpsc *mpsc)
{
	unsigned long flags;
        /* error reset                */
	save_flags(flags); cli();
        MPSC_WRITE_COMMAND(*mpsc, MPSC_CR0_CMD_RESET_ERROR);
	restore_flags(flags);
}

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * mpsc_interrupt().  They were separated out for readability's sake.
 *
 * Note: mpsc_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * mpsc_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer mpsc_serial.c
 *
 * and look at the resulting assemble code in mpsc_serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */


/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void 
mpsc_sched_event(struct mpsc_serial *info,
				  int event)
{
	info->event |= 1 << event;
	queue_task_irq_off(&info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}


static _INLINE_ void 
receive_chars(struct mpsc_serial *info, struct pt_regs *regs)
{
	struct tty_struct *tty = info->tty;
	unsigned char ch, status;

        /*while*/ if ((status = MPSC_READ_REG(info->mpsc, MPSC_SR0)) 
                      & MPSC_SR0_RX_CHAR_AVAIL) {
                ch = MPSC_READ_DATA(info->mpsc);
                /* only insert character in input queue if parity checking is
                 * set of or no parity / framing error occured */


#if 0
                /* Look for kgdb 'stop' character, consult the gdb
                 * documentation for remote target debugging and
                 * arch/sparc/kernel/sparc-stub.c to see how all this
                 * works.
                 */
                if((info->kgdb_channel) && (ch =='\003')) {
                        breakpoint();
                        goto clear_and_exit;
                }
#endif

                if(!tty)
                        goto clear_and_exit;

                if (tty->flip.count >= TTY_FLIPBUF_SIZE)
                        queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
                tty->flip.count++;
                if(status & MPSC_SR0_PARITY_ERROR)
                        *tty->flip.flag_buf_ptr++ = TTY_PARITY;
                else if(status & MPSC_SR0_RX_OVERRUN)
                        *tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
                else if(status & MPSC_SR0_CRC_FRAMING_ERROR)
                        *tty->flip.flag_buf_ptr++ = TTY_FRAME;
                else
                        *tty->flip.flag_buf_ptr++ = 0; /* XXX */
                *tty->flip.char_buf_ptr++ = ch;

#ifdef SERIAL_DEBUG_INTR
                MPSC_DEBUG(5, "mpsc_receive_char: ok %c\n", ch);
#endif

                queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
        }
        panel_led_on(info->ledRx);
        mpsc_start_led_timer(&info->ledRxTimer);

        return;
clear_and_exit:
	mpsc_recv_clear(&info->mpsc);
}

static _INLINE_ void transmit_chars(struct mpsc_serial *info)
{
	/* Is this relevant for the uPD72001
         * P3: In theory we have to test readiness here because a
	 * serial console can clog the chip through mpsc_put_char().
	 * David did not do this. I think he relies on 3-chars FIFO in 8530.
	 * Let's watch for lost _output_ characters. XXX
	 */


	if (info->x_char) {
		/* Send next char */
                MPSC_WRITE_DATA(info->mpsc, info->x_char);
		/* udelay(5); */
		info->x_char = 0;
                panel_led_on(info->ledTx);
                mpsc_start_led_timer(&info->ledTxTimer);
		goto clear_and_return;
	}

	if((info->xmit_cnt <= 0) || info->tty->stopped) {
		/* That's peculiar... */
                MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_TX_INT); 
		/* udelay(5); */
		goto clear_and_return;
	}

	/* Send char */
        MPSC_WRITE_DATA(info->mpsc, info->xmit_buf[info->xmit_tail++]);
	/* udelay(5); */
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	info->xmit_cnt--;

        panel_led_on(info->ledTx);
        mpsc_start_led_timer(&info->ledTxTimer);

	if (info->xmit_cnt < WAKEUP_CHARS)
		mpsc_sched_event(info, MPSC_EVENT_WRITE_WAKEUP);

	if(info->xmit_cnt <= 0) {
                MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_TX_INT); 
		/* udelay(5); */
		goto clear_and_return;
	}

clear_and_return:
	/* Clear interrupt */
	/* done in main interrupt handler
         * info->zs_channel->control = RES_H_IUS;
	 * udelay(5);
         */
	return;
}

static _INLINE_ void 
status_handle(struct mpsc_serial *info)
{
	unsigned char status;

	/* Get status from Read Register 1 */
        status = MPSC_READ_REG(info->mpsc, MPSC_SR1);
	/* Clear status condition... */
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_EXT_STAT_INT);

#if 1
        /* 2003-07-17 gc: do we realy want this? */
	if(status & MPSC_SR1_DCD) {
		if((info->tty->termios->c_cflag & CRTSCTS) &&
		   ((info->cr[3] & MPSC_CR3_AUTO_ENABLE)==0)) {
			info->cr[3] |= MPSC_CR3_AUTO_ENABLE;
                        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);
		}
	} else {
		if((info->cr[3] & MPSC_CR3_AUTO_ENABLE)) {
			info->cr[3] &= ~MPSC_CR3_AUTO_ENABLE;
                        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);
		}
	}
#endif

	/* Whee, if this is console input and this is a
	 * 'break asserted' status change interrupt, call
	 * the boot prom.
	 */
	return;
}

/*
 * This is the serial driver's generic interrupt routine
 */
void mpsc_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
        struct mpsc_serial *info = (struct mpsc_serial *) dev_id;
        u8 intStatus = MPSC_READ_REG(info->mpscb, MPSC_SR2);
        u8 thisChannel = (intStatus & MPSC_SR2B_INT_CHANNEL_A) ? 0 : 1;
	/* unsigned char zs_intreg; */
        unsigned char dat;
        
	if (!info || info->magic != SERIAL_MAGIC) {
                printk("mpsc_interrupt: unexpected interrupt\n");
		return;
        }

        if (thisChannel != info->channel) {
                struct mpsc_serial *info_temp = info->slave_device;
                if (!info_temp || info_temp->magic != SERIAL_MAGIC) {
                        printk("mpsc_interrupt: slave device not active\n");
                        MPSC_WRITE_COMMAND(info->mpsca, 
                                           MPSC_CR0_CMD_END_OF_INT); 
                        return;
                }
                info = info_temp;
        }


        /* vector should be 0 */
        if ((intStatus & ~(MPSC_SR2B_EVENT_MASK|MPSC_SR2B_INT_CHANNEL_A)) 
            == 0) { 
                switch (intStatus & MPSC_SR2B_EVENT_MASK) {
                case MPSC_SR2B_INT_TX_EMPTY:
                        transmit_chars(info);
                        break;

                case MPSC_SR2B_INT_EXT_STATUS:
                        status_handle(info);
                        break;

                case MPSC_SR2B_INT_RX_AVAIL: 
                        receive_chars(info, regs);
                        break;
                case MPSC_SR2B_INT_SPECIAL_RX: 
                        dat = MPSC_READ_REG(info->mpsc, MPSC_SR0);
            
                        dat = MPSC_READ_DATA(info->mpsc);      
                        MPSC_WRITE_COMMAND(info->mpsc, 
                                           MPSC_CR0_CMD_RESET_ERROR);
                        break;    
                }
        } 
        MPSC_WRITE_COMMAND(info->mpsca, MPSC_CR0_CMD_END_OF_INT); 
}
/*
 * -------------------------------------------------------------------
 * Here ends the serial interrupt routines.
 * -------------------------------------------------------------------
 */

/*
 * This routine is used to handle the "bottom half" processing for the
 * serial driver, known also the "software interrupt" processing.
 * This processing is done at the kernel interrupt level, after the
 * mpsc_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using mpsc_sched_event(), and they get done here.
 */
static void do_serial_bh(void)
{
#ifdef SERIAL_DEBUG_INTR
        MPSC_DEBUG(5, "mpsc do_serial_bh\n");
#endif
	run_task_queue(&tq_serial);
}

static void do_softint(void *private_)
{
	struct mpsc_serial	*info = (struct mpsc_serial *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	if (clear_bit(MPSC_EVENT_WRITE_WAKEUP, &info->event)) {
		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		    tty->ldisc.write_wakeup)
			(tty->ldisc.write_wakeup)(tty);
		wake_up_interruptible(&tty->write_wait);
	}
}

/*
 * This routine is called from the scheduler tqueue when the interrupt
 * routine has signalled that a hangup has occurred.  The path of
 * hangup processing is:
 *
 * 	serial interrupt routine -> (scheduler tqueue) ->
 * 	do_serial_hangup() -> tty->hangup() -> mpsc_hangup()
 * 
 */
static void do_serial_hangup(void *private_)
{
	struct mpsc_serial	*info = (struct mpsc_serial *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	tty_hangup(tty);
}


/*
 * This subroutine is called when the RS_TIMER goes off.  It is used
 * by the serial driver to turn off RX / TX LED's
 */
static void mpsc_timer(void)
{
        s16 i;
        register struct mpsc_serial *drv;
        u16 now = (u16) jiffies;

        /* reactivate timer */
	timer_table[RS_TIMER].expires = jiffies+(HZ/10);
        timer_active |= 1<<RS_TIMER;

        for (i=0, drv=mpsc_soft; i<sizeof(mpsc_soft)/sizeof(mpsc_soft[0]); 
             i++, drv++)
                if (drv->used ) {
                        if (drv->ledTxTimer > 0 
                            && (s16) (now-drv->ledTxTimer) >= 0) {
                                panel_led_off(drv->ledTx);
                        }
                        if (drv->ledRxTimer > 0 
                            && (s16) (now-drv->ledRxTimer) >= 0) {
                                panel_led_off(drv->ledRx);
                        }
                }
}

static int startup(struct mpsc_serial *info)
{
	unsigned long flags;
        MPSC_DEBUG(5, "%s\n", __FUNCTION__);


	if (info->flags & MPSC_INITIALIZED)
		return 0;

	if (!info->xmit_buf) {
		info->xmit_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!info->xmit_buf)
			return -ENOMEM;
	}

	save_flags(flags); cli();

#if 1 /* def SERIAL_DEBUG_OPEN */
	MPSC_DEBUG(5, "starting up ttys%d (irq %d)...", info->line, info->irq);
#endif

	/*
	 * Clear the FIFO buffers and disable them
	 * (they will be reenabled in mpsc_hw_reinitialize())
	 */
	MPSC_CLEARFIFO(info->mpsc); 
	info->xmit_fifo_size = 1;


        /* initialize hardware */
        mpsc_hw_initialize(info);

	/*
	 * Now, initialize 
	 */
	mpsc_rtsdtr(info, 1);

        /* enable mpsc RX and TX interrupt */
        info->cr[1] |= MPSC_CR1_RX_INT_ALL_1 | MPSC_CR1_TX_INT_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);

        info->cr[3] |= MPSC_CR3_RX_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1 | MPSC_CR0_CMD_RESET_EXT_STAT_INT, 
                       info->cr[1]);

        info->cr[5] |= MPSC_CR5_TX_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);

        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_ERROR);

        MPSC_WRITE_COMMAND(info->mpsca, MPSC_CR0_CMD_END_OF_INT); 

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * Set up serial timers...
	 */
#if 0  /* Works well and stops the machine. */
	timer_table[RS_TIMER].expires = jiffies + 2;
	timer_active |= 1 << RS_TIMER;
#endif

	/*
	 * and set the speed of the serial port
	 */
	mpsc_hw_reinitialize(info);

	info->flags |= MPSC_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct mpsc_serial * info)
{
	unsigned long	flags;

	if (!(info->flags & MPSC_INITIALIZED))
		return;

#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, "Shutting down serial port %d (irq %d)....", info->line,
	       info->irq);
#endif
	
	save_flags(flags); cli(); /* Disable interrupts */
	
	if (info->xmit_buf) {
		free_page((unsigned long) info->xmit_buf);
		info->xmit_buf = 0;
	}

	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);
	
	info->flags &= ~MPSC_INITIALIZED;
	restore_flags(flags);
}


/* This is for console output over ttya/ttyb */
static void mpsc_put_char(struct mpsc_serial *info, char ch)
{
	int flags, loops = 0;

	save_flags(flags); cli();
	while((MPSC_READ_REG(info->mpsc, MPSC_SR0) 
               & MPSC_SR0_TX_BUF_EMPTY)==0 && loops < 10000) {
		loops++;
		udelay(5);
	}

        MPSC_WRITE_DATA(info->mpsc, ch);
        panel_led_on(info->ledTx);
        mpsc_start_led_timer(&info->ledTxTimer);    
	udelay(5);
	restore_flags(flags);
}

#if 0
/* These are for receiving and sending characters under the kgdb
 * source level kernel debugger.
 */
void putDebugChar(struct mpsc_serial *info, char kgdb_char)
{
	while((MPSC_READ_REG(info->mpsc, MPSC_SR0) 
               & MPSC_SR0_TX_BUF_EMPTY)==0) {
		udelay(5);
	}
        MPSC_WRITE_DATA(info->mpsc, kgdb_char);
        panel_led_on(info->ledTx);
        mpsc_start_led_timer(&info->ledTxTimer);    
}

char getDebugChar(void)
{
#if 0
	struct sun_zschannel *chan = zs_kgdbchan;

	while((chan->control & Rx_CH_AV)==0)
		barrier();
	return chan->data;
#endif
        return '\0';
}
#endif

/*
 * Fair output driver allows a process to speak.
 */
static void mpsc_fair_output(struct mpsc_serial *info)
{
	int left;		/* Output no more than that */
	unsigned long flags;
	char c;

	if (info == 0) return;
	if (info->xmit_buf == 0) return;

	save_flags(flags);  cli();
	left = info->xmit_cnt;
	while (left != 0) {
		c = info->xmit_buf[info->xmit_tail];
		info->xmit_tail = (info->xmit_tail+1) & (SERIAL_XMIT_SIZE-1);
		info->xmit_cnt--;
		restore_flags(flags);

		mpsc_put_char(console_driver, c);

		save_flags(flags);  cli();
		left = MIN(info->xmit_cnt, left-1);
	}

	/* Last character is being transmitted now (hopefully). */
        MPSC_WRITE_COMMAND(info->mpsc, MPSC_CR0_CMD_RESET_TX_INT); 
	udelay(5);

	restore_flags(flags);
	return;
}

/*
 * mpsc_console_print is registered for printk.
 */
static void mpsc_console_print(const char *p)
{
	char c;

        console_driver->cr[5] |= MPSC_CR5_TX_ENABLE;
        MPSC_WRITE_REG(console_driver->mpsc, MPSC_CR5, console_driver->cr[5]);

	while ((c=*(p++)) != 0) {
		if (c == '\n')
			mpsc_put_char(console_driver, '\r');
		mpsc_put_char(console_driver, c);
	}

	/* Comment this if you want to have a strict interrupt-driven output */
	mpsc_fair_output(console_driver);

	return;
}

static inline void mpsc_hw_flush_chars(struct mpsc_serial *info)
{
	unsigned long flags;
	/* Enable transmitter */
	save_flags(flags); cli();
	info->cr[1] |= MPSC_CR1_TX_INT_ENABLE  | MPSC_CR1_EXT_INT_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1, info->cr[1]);

	info->cr[5] |= MPSC_CR5_TX_ENABLE;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);

	/*
	 * Send a first (bootstrapping) character. 
	 */
	if (MPSC_READ_REG(info->mpsc, MPSC_SR0) & MPSC_SR0_TX_BUF_EMPTY) {
		/* Send char */
                MPSC_WRITE_DATA(info->mpsc, info->xmit_buf[info->xmit_tail++]);
                panel_led_on(info->ledTx);
                mpsc_start_led_timer(&info->ledTxTimer);    

		udelay(5);
		info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
		info->xmit_cnt--;
	}
	restore_flags(flags);
}

static void mpsc_flush_chars(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;

        MPSC_DEBUG(6, "%s\n", __FUNCTION__);

	if (mpsc_paranoia_check(info, tty->device, "mpsc_flush_chars"))
		return;

	if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
	    !info->xmit_buf)
		return;

        mpsc_hw_flush_chars(info);
}

static int mpsc_write(struct tty_struct * tty, int from_user,
                      const unsigned char *buf, int count)
{
	int c, total = 0;
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
	unsigned long flags;

        MPSC_DEBUG(6, "%s\n", __FUNCTION__);

	if (mpsc_paranoia_check(info, tty->device, "mpsc_write"))
		return 0;

	if (!tty || !info->xmit_buf)
		return 0;

	save_flags(flags);
	while (1) {
		cli();		
		c = MIN(count, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
				   SERIAL_XMIT_SIZE - info->xmit_head));
		if (c <= 0)
			break;

		if (from_user) {
			down(&tmp_buf_sem);
			memcpy_fromfs(tmp_buf, buf, c);
			c = MIN(c, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
				       SERIAL_XMIT_SIZE - info->xmit_head));
			memcpy(info->xmit_buf + info->xmit_head, tmp_buf, c);
			up(&tmp_buf_sem);
		} else
			memcpy(info->xmit_buf + info->xmit_head, buf, c);
		info->xmit_head = (info->xmit_head + c) & (SERIAL_XMIT_SIZE-1);
		info->xmit_cnt += c;
		restore_flags(flags);
		buf += c;
		count -= c;
		total += c;
	}        
	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped &&
	    !(info->cr[5] & MPSC_CR1_TX_INT_ENABLE)) {
                mpsc_hw_flush_chars(info);
	}
	restore_flags(flags);
	return total;
}

static int mpsc_write_room(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
	int	ret;
				
	if (mpsc_paranoia_check(info, tty->device, "mpsc_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int mpsc_chars_in_buffer(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
				
	if (mpsc_paranoia_check(info, tty->device, "mpsc_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void mpsc_flush_buffer(struct tty_struct *tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
        unsigned long flags;

        MPSC_DEBUG(5, "%s\n", __FUNCTION__);
				
	if (mpsc_paranoia_check(info, tty->device, "mpsc_flush_buffer"))
		return;
        save_flags(flags); cli();
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
        restore_flags(flags);
	wake_up_interruptible(&tty->write_wait);
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);
}

/*
 * ------------------------------------------------------------
 * mpsc_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void mpsc_throttle(struct tty_struct * tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
        unsigned long flags;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	MPSC_DEBUG(5, "throttle %s: %d....\n", _tty_name(tty, buf),
                   tty->ldisc.chars_in_buffer(tty));
#endif

	if (mpsc_paranoia_check(info, tty->device, "mpsc_throttle"))
		return;
	
	if (I_IXOFF(tty))
		info->x_char = STOP_CHAR(tty);

	/* Turn off RTS line */
        save_flags(flags); cli();

	info->cr[5] &= ~MPSC_CR5_TX_RTS;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);

        restore_flags(flags);
}

static void mpsc_unthrottle(struct tty_struct * tty)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
        unsigned long flags;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	MPSC_DEBUG(5, "unthrottle %s: %d....\n", _tty_name(tty, buf),
                   tty->ldisc.chars_in_buffer(tty));
#endif

	if (mpsc_paranoia_check(info, tty->device, "mpsc_unthrottle"))
		return;
	
	if (I_IXOFF(tty)) {
		if (info->x_char)
			info->x_char = 0;
		else
			info->x_char = START_CHAR(tty);
	}

	/* Assert RTS line */
        save_flags(flags); cli();
	info->cr[5] |= MPSC_CR5_TX_RTS;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
        restore_flags(flags);
}

/*
 * ------------------------------------------------------------
 * mpsc_ioctl() and friends
 * ------------------------------------------------------------
 */

static int get_serial_info(struct mpsc_serial * info,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
  
	if (!retinfo)
		return -EFAULT;
        MPSC_DEBUG(5, "%s\n", __FUNCTION__);

	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.port = (int) info->mpsc.control;
	tmp.irq = info->irq;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	tmp.custom_divisor = info->custom_divisor;
	memcpy_tofs(retinfo,&tmp,sizeof(*retinfo));

	return 0;
}

static int set_serial_info(struct mpsc_serial * info,
			   struct serial_struct * new_info)
{
	int retval = 0;
	struct serial_struct new_serial;
	/* struct mpsc_serial old_info; */

        MPSC_DEBUG(5, "%s\n", __FUNCTION__);

	if (!new_info)
		return -EFAULT;
	memcpy_fromfs(&new_serial, new_info, sizeof(new_serial));
	/* old_info = *info; */

	if (!suser()) {
		if ((new_serial.baud_base != info->baud_base) ||
		    (new_serial.type != info->type) ||
		    (new_serial.close_delay != info->close_delay) ||
		    ((new_serial.flags & ~MPSC_USR_MASK) !=
		     (info->flags & ~MPSC_USR_MASK)))
			return -EPERM;
		info->flags = ((info->flags & ~MPSC_USR_MASK) |
			       (new_serial.flags & MPSC_USR_MASK));
		info->custom_divisor = new_serial.custom_divisor;
		goto check_and_exit;
	}

	if (info->count > 1)
		return -EBUSY;

	/*
	 * OK, past this point, all the error checking has been done.
	 * At this point, we start making changes.....
	 */

	info->baud_base = new_serial.baud_base;
	info->flags = ((info->flags & ~MPSC_FLAGS) |
			(new_serial.flags & MPSC_FLAGS));
	info->type = new_serial.type;
	info->close_delay = new_serial.close_delay;
	info->closing_wait = new_serial.closing_wait;

check_and_exit:
	retval = startup(info);
	return retval;
}

/*
 * get_lsr_info - get line status register info
 *
 * Purpose: Let user call ioctl() to get info when the UART physically
 * 	    is emptied.  On bus types like RS485, the transmitter must
 * 	    release the bus after transmitting. This must be done when
 * 	    the transmit shift register is empty, not be done when the
 * 	    transmit holding register is empty.  This functionality
 * 	    allows an RS485 driver to be written in user space. 
 */
static int get_lsr_info(struct mpsc_serial *info, unsigned int *value)
{
	unsigned char status;
        unsigned long flags;

        save_flags(flags); cli();

	status = MPSC_READ_REG(info->mpsc, MPSC_SR0);

        restore_flags(flags);

	put_user(status, value);
	return 0;
}

static int get_modem_info(struct mpsc_serial *info, unsigned int *value)
{
	unsigned char control, status;
        unsigned long flags;
	unsigned int result;


        save_flags(flags); cli();

	/* Get status from Read Register 1 */
        status = MPSC_READ_REG(info->mpsc, MPSC_SR1);

        restore_flags(flags);

	control = info->cr[5];

        /* it seems that the PD72001 don't have RI and DSR signals?  */
	result =  ((control & MPSC_CR5_TX_RTS) ? TIOCM_RTS : 0)
		| ((control & MPSC_CR5_TX_DTR) ? TIOCM_DTR : 0)
		| ((status  & MPSC_SR1_DCD) ? TIOCM_CAR : 0)
		| ((status  & MPSC_SR1_CTS) ? TIOCM_CTS : 0);
	put_user(result, value);
	return 0;
}

static int set_modem_info(struct mpsc_serial *info, unsigned int cmd,
			  unsigned int *value)
{
	int error;
        unsigned long flags;
	unsigned int arg;

	error = verify_area(VERIFY_READ, value, sizeof(int));
	if (error) {
		return error;
        }
	arg = get_user(value);
	switch (cmd) {
	case TIOCMBIS: 
		if (arg & TIOCM_RTS) {
                        info->cr[5] |= MPSC_CR5_TX_RTS;
		}
		if (arg & TIOCM_DTR) {
                        info->cr[5] |= MPSC_CR5_TX_DTR;
		}
		break;
	case TIOCMBIC:
		if (arg & TIOCM_RTS) {
                        info->cr[5] &= ~MPSC_CR5_TX_RTS;
		}
		if (arg & TIOCM_DTR) {
                        info->cr[5] &= ~MPSC_CR5_TX_DTR;
		}
		break;
	case TIOCMSET:
                info->cr[5] = (info->cr[5] 
                               & ~MPSC_CR5_TX_RTS 
                               & ~MPSC_CR5_TX_DTR)
                        | ((arg & TIOCM_RTS) ? MPSC_CR5_TX_RTS : 0)
                        | ((arg & TIOCM_DTR) ? MPSC_CR5_TX_DTR : 0);
		break;
	default:
		return -EINVAL;
	}
        save_flags(flags); cli();
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
        restore_flags(flags);
	return 0;
}



/*
 * This routine sends a break character out the serial port.
 */
static void send_break(	struct mpsc_serial *info, int duration)
{
	if (info->mpsc.control)
		return;
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();

        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, 
                       (info->cr[5] | MPSC_CR5_TX_SEND_BREAK));


	schedule();
        MPSC_WRITE_REG(info->mpsc, MPSC_CR5, info->cr[5]);
	sti();
}

static int mpsc_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;
	int retval;

	if (mpsc_paranoia_check(info, tty->device, "mpsc_ioctl"))
		return -ENODEV;
        MPSC_DEBUG(5, "mpsc_ioctl: cmd=0x%x\n", cmd);

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
	    (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
		case TCSBRK:	/* SVID version: non-zero arg --> no break */
                        MPSC_DEBUG(5, "mpsc_ioctl: TCSBRK\n");

			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (!arg)
				send_break(info, HZ/4);	/* 1/4 second */
			return 0;
		case TCSBRKP:	/* support for POSIX tcsendbreak() */
                        MPSC_DEBUG(5, "mpsc_ioctl: TCSBRKP\n");
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			send_break(info, arg ? arg*(HZ/10) : HZ/4);
			return 0;
		case TIOCGSOFTCAR:
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCGSOFTCAR\n");

			error = verify_area(VERIFY_WRITE, (void *) arg,
                                            sizeof(long));
			if (error)
				return error;
			put_fs_long(C_CLOCAL(tty) ? 1 : 0,
				    (unsigned long *) arg);
			return 0;
		case TIOCSSOFTCAR:
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCSSOFTCAR\n");

			arg = get_fs_long((unsigned long *) arg);
			tty->termios->c_cflag =
				((tty->termios->c_cflag & ~CLOCAL) |
				 (arg ? CLOCAL : 0));
			return 0;


		case TIOCMGET:
			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(unsigned int));
			if (error)
				return error;
			return get_modem_info(info, (unsigned int *) arg);

		case TIOCMBIS:
		case TIOCMBIC:
		case TIOCMSET:
			return set_modem_info(info, cmd, (unsigned int *) arg);

		case TIOCGSERIAL:
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCGSERIAL\n");

			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct serial_struct));
			if (error)
				return error;
			return get_serial_info(info,
					       (struct serial_struct *) arg);
		case TIOCSSERIAL:
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCSSERIAL\n");

			return set_serial_info(info,
					       (struct serial_struct *) arg);

		case TIOCSERGETLSR: /* Get line status register */
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCSERGETLSR\n");

			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(unsigned int));
			if (error)
				return error;
			else
			    return get_lsr_info(info, (unsigned int *) arg);

		case TIOCSERGSTRUCT:
                        MPSC_DEBUG(5, "mpsc_ioctl: TIOCSERGSTRUCT\n");

			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct mpsc_serial));
			if (error)
				return error;
			memcpy_tofs((struct mpsc_serial *) arg,
				    info, sizeof(struct mpsc_serial));
			return 0;
			
		default:
                        MPSC_DEBUG(5, "mpsc_ioctl: unknown: ENOIOCTLCMD\n");

			return -ENOIOCTLCMD;
		}

	return 0;
}

static void mpsc_set_termios(struct tty_struct *tty, 
                             struct termios *old_termios)
{
	struct mpsc_serial *info = (struct mpsc_serial *)tty->driver_data;

	if (tty->termios->c_cflag == old_termios->c_cflag)
		return;

        MPSC_DEBUG(5, "%s\n", __FUNCTION__);

	mpsc_hw_reinitialize(info);

	if ((old_termios->c_cflag & CRTSCTS) &&
	    !(tty->termios->c_cflag & CRTSCTS)) {
                MPSC_DEBUG(5, "%s hw stopped\n", __FUNCTION__);
		tty->hw_stopped = 0;
		mpsc_start(tty);
	}
}

/*
 * ------------------------------------------------------------
 * mpsc_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * MPSC structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void mpsc_close(struct tty_struct *tty, struct file * filp)
{
	struct mpsc_serial * info = (struct mpsc_serial *)tty->driver_data;
	unsigned long flags;

	if (!info || mpsc_paranoia_check(info, tty->device, "mpsc_close"))
		return;
	
	save_flags(flags); cli();
	
	if (tty_hung_up_p(filp)) {
		restore_flags(flags);
		return;
	}
	
#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, "mpsc_close ttys%d, count = %d\n", 
                   info->line, info->count);
#endif
	if ((tty->count == 1) && (info->count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  Info->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk("mpsc_close: bad serial port count; tty->count is 1, "
		       "info->count is %d\n", info->count);
		info->count = 1;
	}
	if (--info->count < 0) {
		printk("mpsc_close: bad serial port count for ttys%d: %d\n",
		       info->line, info->count);
		info->count = 0;
	}
	if (info->count) {
		restore_flags(flags);
		return;
	}
	info->flags |= MPSC_CLOSING;
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if (info->flags & MPSC_NORMAL_ACTIVE)
		info->normal_termios = *tty->termios;
	if (info->flags & MPSC_CALLOUT_ACTIVE)
		info->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify 
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if (info->closing_wait != MPSC_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, info->closing_wait);
	/*
	 * At this point we stop accepting input.  To do this, we
	 * disable the receive line status interrupts, and tell the
	 * interrupt driver to stop checking the data ready bit in the
	 * line status register.
	 */
	/** if (!info->iscons) ... **/
	info->cr[3] &= ~MPSC_CR3_RX_ENABLE ;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);

	info->cr[1] &= ~MPSC_CR1_RX_INT_MASK ;
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1, info->cr[1]);
	MPSC_CLEARFIFO(info->mpsc);

	shutdown(info);
	if (tty->driver.flush_buffer)
		tty->driver.flush_buffer(tty);
	if (tty->ldisc.flush_buffer)
		tty->ldisc.flush_buffer(tty);
	tty->closing = 0;
	info->event = 0;
	info->tty = 0;
	if (tty->ldisc.num != ldiscs[N_TTY].num) {
		if (tty->ldisc.close)
			(tty->ldisc.close)(tty);
		tty->ldisc = ldiscs[N_TTY];
		tty->termios->c_line = N_TTY;
		if (tty->ldisc.open)
			(tty->ldisc.open)(tty);
	}
	if (info->blocked_open) {
		if (info->close_delay) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + info->close_delay;
			schedule();
		}
		wake_up_interruptible(&info->open_wait);
	}
	info->flags &= ~(MPSC_NORMAL_ACTIVE|MPSC_CALLOUT_ACTIVE|
			 MPSC_CLOSING);
	wake_up_interruptible(&info->close_wait);
        panel_led_off(info->ledTx);
        panel_led_off(info->ledRx);
        info->used = 0;
	restore_flags(flags);
}

/*
 * mpsc_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void mpsc_hangup(struct tty_struct *tty)
{
	struct mpsc_serial * info = (struct mpsc_serial *)tty->driver_data;
	
	if (mpsc_paranoia_check(info, tty->device, "mpsc_hangup"))
		return;
	
	mpsc_flush_buffer(tty);
	shutdown(info);
	info->event = 0;
	info->count = 0;
	info->flags &= ~(MPSC_NORMAL_ACTIVE|MPSC_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * mpsc_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file *filp,
			   struct mpsc_serial *info)
{
	struct wait_queue wait = { current, NULL };
	int		retval;
	int		do_clocal = 0;

        MPSC_DEBUG(5, "%s\n", __FUNCTION__);

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (info->flags & MPSC_CLOSING) {
		interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		if (info->flags & MPSC_HUP_NOTIFY)
			return -EAGAIN;
		else
			return -ERESTARTSYS;
#else
		return -EAGAIN;
#endif
	}

	/*
	 * If this is a callout device, then just make sure the normal
	 * device isn't being used.
	 */
	if (tty->driver.subtype == SERIAL_TYPE_CALLOUT) {
		if (info->flags & MPSC_NORMAL_ACTIVE)
			return -EBUSY;
		if ((info->flags & MPSC_CALLOUT_ACTIVE) &&
		    (info->flags & MPSC_SESSION_LOCKOUT) &&
		    (info->session != current->session))
		    return -EBUSY;
		if ((info->flags & MPSC_CALLOUT_ACTIVE) &&
		    (info->flags & MPSC_PGRP_LOCKOUT) &&
		    (info->pgrp != current->pgrp))
		    return -EBUSY;
		info->flags |= MPSC_CALLOUT_ACTIVE;
		return 0;
	}
	
	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
	    (tty->flags & (1 << TTY_IO_ERROR))) {
		if (info->flags & MPSC_CALLOUT_ACTIVE)
			return -EBUSY;
		info->flags |= MPSC_NORMAL_ACTIVE;
		return 0;
	}

	if (info->flags & MPSC_CALLOUT_ACTIVE) {
		if (info->normal_termios.c_cflag & CLOCAL)
			do_clocal = 1;
	} else {
		if (tty->termios->c_cflag & CLOCAL)
			do_clocal = 1;
	}
	
	/*
	 * Block waiting for the carrier detect and the line to become
	 * free (i.e., not in use by the callout).  While we are in
	 * this loop, info->count is dropped by one, so that
	 * mpsc_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->open_wait, &wait);
#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, 
                   "block_til_ready before block: ttys%d, count = %d\n",
                   info->line, info->count);
#endif
	info->count--;
	info->blocked_open++;
	while (1) {
                unsigned long flags;
                save_flags(flags); cli();

		if (!(info->flags & MPSC_CALLOUT_ACTIVE))
			mpsc_rtsdtr(info, 1);

                restore_flags(flags);
                        
		current->state = TASK_INTERRUPTIBLE;
		if (tty_hung_up_p(filp) ||
		    !(info->flags & MPSC_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
			if (info->flags & MPSC_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;	
#else
			retval = -EAGAIN;
#endif
			break;
		}
		if (!(info->flags & MPSC_CALLOUT_ACTIVE) &&
		    !(info->flags & MPSC_CLOSING) && do_clocal)
			break;
		if (current->signal & ~current->blocked) {
			retval = -ERESTARTSYS;
			break;
		}
#ifdef SERIAL_DEBUG_OPEN
		MPSC_DEBUG(5, 
                           "block_til_ready blocking: ttys%d, count = %d\n",
		       info->line, info->count);
#endif
		schedule();
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&info->open_wait, &wait);
	if (!tty_hung_up_p(filp))
		info->count++;
	info->blocked_open--;
#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, 
                   "block_til_ready after blocking: ttys%d, count = %d\n",
                   info->line, info->count);
#endif
	if (retval)
		return retval;
	info->flags |= MPSC_NORMAL_ACTIVE;
	return 0;
}	

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its MPSC structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
int mpsc_open(struct tty_struct *tty, struct file * filp)
{
	struct mpsc_serial	*info;
	int 			retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;
	if ((line < 0) || (line >= NUM_CHANNELS))
		return -ENODEV;
	info = mpsc_soft + line;
	/* Is the kgdb running over this line? */
	if (info->kgdb_channel)
		return -ENODEV;
	if (mpsc_paranoia_check(info, tty->device, "mpsc_open"))
		return -ENODEV;
#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, 
                   "mpsc_open %s%d, count = %d\n", 
                   tty->driver.name, info->line,
                   info->count);
#endif
	info->count++;
	tty->driver_data = info;
	info->tty = tty;
        /* get flag for console baud rate */
        if (info->line == CONSOLE_PORT) {
                tty->termios->c_cflag &= ~CBAUD;
                tty->termios->c_cflag |= 
                        mpsc_get_cflag_for_baud_rate(info->baudrate);
        }

	/*
	 * Start up serial port
	 */
	retval = startup(info);
	if (retval)
		return retval;

	retval = block_til_ready(tty, filp, info);
	if (retval) {
#ifdef SERIAL_DEBUG_OPEN
		MPSC_DEBUG(5, 
                           "mpsc_open returning after "
                           "block_til_ready with %d\n",
                           retval);
#endif
		return retval;
	}

	if ((info->count == 1) && (info->flags & MPSC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = info->normal_termios;
		else 
			*tty->termios = info->callout_termios;
		mpsc_hw_reinitialize(info);
	}

	info->session = current->session;
	info->pgrp = current->pgrp;
        info->used = 1;
#ifdef SERIAL_DEBUG_OPEN
	MPSC_DEBUG(5, "mpsc_open ttys%d successful...", info->line);
#endif
        
	return 0;
}

/* Finally, routines used to initialize the serial driver. */

static void show_serial_version(void)
{
	printk("NEC µPD72001 serial driver version 1.00\n");
}



extern void register_console(void (*proc)(const char *));


static void mpsc_init_channel(struct mpsc_serial *info, int port_num,
                              const struct mpsc_channel_info *port_info)
{
        const struct mpsc_channel_info *channel_info;
        register const struct mpsc_chip_info *chip_info;
        struct mpsc_serial *iter;               
        unsigned long flags;

        if (port_num < 0 
            || port_num >= sizeof(mpsc_channels)/sizeof(mpsc_channels[0]) ) {
                return ;
        }

        info->magic = SERIAL_MAGIC;
        info->line = port_num;
        info->tty = 0;
        info->custom_divisor = 16;
        info->close_delay = 50;
        info->closing_wait = 3000;
        info->x_char = 0;
        info->event = 0;
        info->count = 0;
        info->blocked_open = 0;
        info->tqueue.routine = do_softint;
        info->tqueue.data = info;
        info->tqueue_hangup.routine = do_serial_hangup;
        info->tqueue_hangup.data = info;
        info->callout_termios =callout_driver.init_termios;
        info->normal_termios = serial_driver.init_termios;
        info->open_wait = 0;
        info->close_wait = 0;

        info->kgdb_channel = 0;

        /* hardware setup */

        channel_info = &mpsc_channels[info->line];
        info->channel_info = channel_info;

        info->channel = channel_info->channel;

        chip_info = &mpsc_chips[channel_info->chip];
        /* info->intNum = chip_info->irq; */

        info->mpsca.control = chip_info->mpsca.control;
        info->mpsca.data = chip_info->mpsca.data;
        info->mpscb.control = chip_info->mpscb.control;
        info->mpscb.data = chip_info->mpscb.data;

        info->irq = chip_info->int_num-VEC_SPUR;

        for (iter=mpsc_soft; 
             iter<mpsc_soft+sizeof(mpsc_soft)/sizeof(mpsc_soft[0]); 
             ++iter)
                if (iter->channel_info->chip == channel_info->chip
                    && iter != info
                    && info->magic == SERIAL_MAGIC) {
                        iter->slave_device = info;
                        info->slave_device = iter;
                        printk("mpsc: device %d: slave device %d assigned\n",
                               (int) (info-mpsc_soft), (int) (iter-mpsc_soft));
                        
                        break;
                }                

        if (info->channel == 0) {
                info->mpsc.control = chip_info->mpsca.control;
                info->mpsc.data = chip_info->mpsca.data;
        } else {
                info->mpsc.control = chip_info->mpscb.control;
                info->mpsc.data = chip_info->mpscb.data;
        }

        info->ledTx = channel_info->ledTx;
        info->ledRx = channel_info->ledRx;

        info->baudrate = info->line == CONSOLE_PORT ? CONSOLE_BAUD_RATE: 9600;

        memset(info->cr, 0, sizeof(info->cr));

        mpsc_hw_initialize(info);

        /* enable mpsc RX and TX interrupt */
        info->cr[1] |= MPSC_CR1_RX_INT_ALL_1 | MPSC_CR1_TX_INT_ENABLE;

        save_flags(flags); cli(); /* Disable interrupts */
        MPSC_WRITE_REG(info->mpsc, MPSC_CR3, info->cr[3]);
        MPSC_WRITE_REG(info->mpsc, MPSC_CR1 | MPSC_CR0_CMD_RESET_EXT_STAT_INT, 
                       info->cr[1]);

        restore_flags(flags);
}


/* mpsc_init inits the driver */
int mpsc_init(void)
{
	int /* chip, channel, */ i, flags;
	struct mpsc_serial *info;

	/* Setup base handler, and timer table. */
	init_bh(SERIAL_BH, do_serial_bh);
	timer_table[RS_TIMER].fn = mpsc_timer;
	timer_table[RS_TIMER].expires = jiffies+(HZ/10);
        timer_active |= 1<<RS_TIMER;


	show_serial_version();

	/* Initialize the tty_driver structure */
	/* uClinux: Not all of this is exactly right for us. */
	
	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic = TTY_DRIVER_MAGIC;
	serial_driver.name = "ttyS";
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64;

#if 0
        /* 2003-07-28 gc: additional initializations for devfs 
        *                 (not supportet in uclinux 2.0.x) */
        serial_driver.devfs_name = "ttys/";
        serial_driver.driver_name = "pd72001_serial";
        serial_driver.owner = THIS_MODULE;
#endif


	serial_driver.num = NUM_CHANNELS;
	serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	serial_driver.subtype = SERIAL_TYPE_NORMAL;
	serial_driver.init_termios = tty_std_termios;

	serial_driver.init_termios.c_cflag =
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	serial_driver.flags = TTY_DRIVER_REAL_RAW;
	serial_driver.refcount = &serial_refcount;
	serial_driver.table = mpsc_tty_table;
	serial_driver.termios = mpsc_termios;
	serial_driver.termios_locked = mpsc_termios_locked;

	serial_driver.open = mpsc_open;
	serial_driver.close = mpsc_close;
	serial_driver.write = mpsc_write;
	serial_driver.flush_chars = mpsc_flush_chars;
	serial_driver.write_room = mpsc_write_room;
	serial_driver.chars_in_buffer = mpsc_chars_in_buffer;
	serial_driver.flush_buffer = mpsc_flush_buffer;
	serial_driver.ioctl = mpsc_ioctl;
	serial_driver.throttle = mpsc_throttle;
	serial_driver.unthrottle = mpsc_unthrottle;
	serial_driver.set_termios = mpsc_set_termios;
	serial_driver.stop = mpsc_stop;
	serial_driver.start = mpsc_start;
	serial_driver.hangup = mpsc_hangup;

	/*
	 * The callout device is just like normal device except for
	 * major number and the subtype code.
	 */
	callout_driver = serial_driver;
	callout_driver.name = "cua";
	callout_driver.major = TTYAUX_MAJOR;
	callout_driver.subtype = SERIAL_TYPE_CALLOUT;

	if (tty_register_driver(&serial_driver))
		panic("Couldn't register serial driver\n");
	if (tty_register_driver(&callout_driver))
		panic("Couldn't register callout driver\n");
	
	save_flags(flags); cli();

        {
                const struct mpsc_channel_info *portInfo;
                save_flags(flags); cli();
         
                for(info=mpsc_soft, portInfo=mpsc_channels, i=0; 
                    info<mpsc_soft+NUM_CHANNELS; 
                    ++portInfo, ++info, ++i)
                {
                        if (i != CONSOLE_PORT) {
                                /* console port is allready initialized */
                                mpsc_init_channel(info, i, portInfo);
                        }

                        printk("tty%02d at 0x%08x (irq = %d, channel=%c)", 
                               info->line, 
                               (unsigned)info->mpsc.control, info->irq,
                               info->channel == 1 ? 'B' : 'A');
                        printk(" is a NEC µPD72001\n");
                        if (info->channel == 1) {
                                /* channel 0 and channel 1 share one irq */
                                if (info->line != CONSOLE_PORT) {
                                        if (request_irq(info->irq,
                                                        mpsc_interrupt,
                                                        IRQ_FLG_LOCK,
                                                        "NEC µPD72001", 
                                                        info))
                                                panic("Unable to "
                                                      "attach intr\n");
                                }
                        }

                }
                restore_flags(flags);        
        }

	return 0;
}

/*
 * register_serial and unregister_serial allows for serial ports to be
 * configured at run-time, to support PCMCIA modems.
 */
/* uClinux: Unused at this time, just here to make things link. */
int register_serial(struct serial_struct *req)
{
	return -1;
}

void unregister_serial(int line)
{
	return;
}


void mpsc_console_initialize(void)
{
        console_driver = mpsc_soft + CONSOLE_PORT;
        
        if (!console_driver)
                return;

        mpsc_initialize_memory();

        /* console_driver = (struct mpsc_serial *) mpsc_open("4");  */
        mpsc_init_channel(console_driver, CONSOLE_PORT, 
                          mpsc_channels + CONSOLE_PORT);
        console_driver->count = 0;
        console_driver->cr[1] &= ~(MPSC_CR1_EXT_INT_ENABLE
                               | MPSC_CR1_TX_INT_ENABLE 
                               | MPSC_CR1_FIRST_TRANSMIT_INT
                                       | MPSC_CR1_RX_INT_MASK
                               | MPSC_CR1_FIRST_RX_INT_MASK );  
        MPSC_WRITE_REG(console_driver->mpsc, MPSC_CR1, console_driver->cr[1]);
        /* startup(console_driver); */

        register_console(mpsc_console_print);
        /* register_console(console_print_mpsc); */
        request_irq(console_driver->irq,
                    mpsc_interrupt,
                    IRQ_FLG_LOCK,
                    "NEC µPD72001 console", console_driver);
}

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
