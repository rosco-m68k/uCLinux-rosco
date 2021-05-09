/*
 *  linux/drivers/char/hitachi-sci.c
 *
 *  Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 * Based On:
 *
 *  linux/drivers/char/serial.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Extensively rewritten by Theodore Ts'o, 8/16/92 -- 9/14/92.  Now
 *  much more extensible to support other serial cards based on the
 *  16450/16550A UART's.  Added support for the AST FourPort and the
 *  Accent Async board.  
 *
 *  set_serial_info fixed to set the flags, custom divisor, and uart
 * 	type fields.  Fix suggested by Michael K. Johnson 12/12/92.
 *
 *  11/95: TIOCMIWAIT, TIOCGICOUNT by Angelo Haritsis <ah@doc.ic.ac.uk>
 *
 *  03/96: Modularised by Angelo Haritsis <ah@doc.ic.ac.uk>
 *
 *  rs_set_termios fixed to look also for changes of the input
 *      flags INPCK, BRKINT, PARMRK, IGNPAR and IGNBRK.
 *                                            Bernd AnhçÖpl 05/17/96.
 * 
 * Added Support for PCI serial boards which contain 16x50 Chips
 * 31.10.1998 Henning P. Schmiedehausen <hps@tanstaafl.de>
 * 
 * This module exports the following rs232 io functions:
 *
 *	int rs_init(void);
 * 	int rs_open(struct tty_struct * tty, struct file * filp)
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/config.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>

#include <asm/system.h>

#include <asm/io.h>

#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/delay.h>

#include "hitachi-sci.h"

static char *serial_name = "Hitachi SCI driver";
static char *serial_version = "0.01";

DECLARE_TASK_QUEUE(tq_serial);

struct tty_driver serial_driver, callout_driver;
static int serial_refcount;

static void change_speed(struct sci_struct *sci);

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2

/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

/*
 * Serial driver configuration section.  Here are the various options:
 *
 * CONFIG_HUB6
 *		Enables support for the venerable Bell Technologies
 *		HUB6 card.
 *
 * SERIAL_PARANOIA_CHECK
 * 		Check the magic number for the async_structure where
 * 		ever possible.
 */

#define SERIAL_PARANOIA_CHECK
#define CONFIG_SERIAL_NOPAUSE_IO
#define SERIAL_DO_RESTART

#undef SERIAL_DEBUG_INTR
#undef SERIAL_DEBUG_OPEN
#undef SERIAL_DEBUG_FLOW

#if 0
#define	SERIAL_DEBUG_INTR	1
#endif

#define RS_STROBE_TIME (10*HZ)
#define RS_ISR_PASS_LIMIT 256

/* #define _INLINE_ inline */
#define _INLINE_
  
#if defined(MODULE) && defined(SERIAL_DEBUG_MCOUNT)
#define DBG_CNT(s) printk("(%s): [%x] refc=%d, serc=%d, ttyc=%d -> %s\n", \
 kdevname(tty->device), (info->flags), serial_refcount,info->count,tty->count,s)
#else
#define DBG_CNT(s)
#endif

#if SCI_CHANNEL == 2
static struct sci_struct rs_table[] = {
	{0,SCI0_BASE,SCI0_IRQ,SCI0_TRXD_IO,SCI0_TRXD_DDR,SCI0_TXD_MASK,SCI0_RXD_MASK,0},
	{1,SCI1_BASE,SCI1_IRQ,SCI1_TRXD_IO,SCI1_TRXD_DDR,SCI1_TXD_MASK,SCI1_RXD_MASK,0},
};
#endif

#if SCI_CHANNEL == 3
static struct sci_struct rs_table[] = {
	{0,SCI0_BASE,SCI0_IRQ,SCI0_TRXD_IO,SCI0_TRXD_DDR,SCI0_TXD_MASK,SCI0_RXD_MASK,0},
	{1,SCI1_BASE,SCI1_IRQ,SCI1_TRXD_IO,SCI1_TRXD_DDR,SCI1_TXD_MASK,SCI1_RXD_MASK,0},
	{2,SCI2_BASE,SCI2_IRQ,SCI2_TRXD_IO,SCI2_TRXD_DDR,SCI2_TXD_MASK,SCI2_RXD_MASK,0},
};
#endif

#define NR_PORTS	(sizeof(rs_table)/sizeof(struct sci_struct))

static struct async_struct rs_info[NR_PORTS];

static struct tty_struct *serial_table[NR_PORTS];
static struct termios *serial_termios[NR_PORTS];
static struct termios *serial_termios_locked[NR_PORTS];
static struct tty_struct console_tty;

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
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
static unsigned char *tmp_buf = 0;
static struct semaphore tmp_buf_sem = MUTEX;

/*
 * This is used to figure out the divisor speeds and the timeouts
 */
#define BRR_VAL(baud,scale) (((CONFIG_CLK_FREQ*1000/32/scale)/baud)-1)

static const short sci_baud_table[] = {
	0,
	0,
	BRR_VAL(50,64)   +(3<<8),
	BRR_VAL(75,64)   +(3<<8),
	BRR_VAL(110,64)  +(3<<8),

	BRR_VAL(134,64)  +(3<<8),
	BRR_VAL(150,64)  +(3<<8),
	BRR_VAL(200,16)  +(2<<8),
	BRR_VAL(300,16)  +(2<<8),
	BRR_VAL(600,16)  +(2<<8),

	BRR_VAL(1200,4) +(1<<8),
	BRR_VAL(2400,4) +(1<<8),
	BRR_VAL(4800,1)  +(0<<8),
	BRR_VAL(9600,1)  +(0<<8),
	BRR_VAL(19200,1) +(0<<8),

	BRR_VAL(38400,1) +(0<<8),
	BRR_VAL(57600,1) +(0<<8),
	BRR_VAL(115200,1)+(0<<8),
	0
};

static inline unsigned int sci_in(struct sci_struct *sci, int offset)
{
	return inb(sci->port + offset);
}

static inline unsigned int sci_inp(struct sci_struct *sci, int offset)
{
	return inb_p(sci->port + offset);
}

static inline void sci_out(struct sci_struct *sci, int offset, int value)
{
	outb(value, sci->port+offset);
}

static inline void sci_outp(struct sci_struct *sci, int offset,
			       int value)
{
    	outb_p(value, sci->port+offset);
}

static inline void sci_ctrl_set(struct sci_struct *sci,unsigned int bit)
{
	outb(inb(sci->port+SCI_SCR) | bit,sci->port+SCI_SCR);
}

static inline void sci_ctrl_reset(struct sci_struct *sci,unsigned int bit)
{
	outb(inb(sci->port+SCI_SCR) & ~bit,sci->port+SCI_SCR);
}

/*
 * ------------------------------------------------------------
 * rs_stop() and rs_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 * ------------------------------------------------------------
 */
static void rs_stop(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	unsigned long flags;

	save_flags(flags); cli();
	sci_ctrl_reset(sci,SCI_SCR_TIE);
	restore_flags(flags);
}

static void rs_start(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	unsigned long flags;
	
	save_flags(flags); cli();
	if (sci->info->xmit_cnt && sci->info->xmit_buf) {
		sci_ctrl_set(sci,SCI_SCR_TIE);
	}
	restore_flags(flags);
}

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * rs_interrupt().  They were separated out for readability's sake.
 *
 * Note: rs_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * rs_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer serial.c
 *
 * and look at the resulting assemble code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct sci_struct *sci,
				  int event)
{
	sci->info->event |= 1 << event;
	queue_task_irq_off(&sci->info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}

static _INLINE_ void receive_chars(struct sci_struct *sci)
{
	struct tty_struct *tty = sci->info->tty;
	unsigned char ch;

	do {
		ch = sci_inp(sci, SCI_RDR);
		sci_outp(sci,SCI_SSR,sci_in(sci,SCI_SSR) & ~SCI_SSR_RDRF);
		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			break;
		tty->flip.count++;
		*tty->flip.flag_buf_ptr++ = 0;
		*tty->flip.char_buf_ptr++ = ch;
	} while (sci_inp(sci, SCI_SSR) & SCI_SSR_RDRF);
	queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
#ifdef SERIAL_DEBUG_INTR
	printk("DR...");
#endif
}

static _INLINE_ void transmit_chars(struct sci_struct *sci)
{
	if (sci->info->x_char) {
		sci_outp(sci, SCI_TDR, sci->info->x_char);
		sci->info->x_char = 0;
		return;
	}
	if ((sci->info->xmit_cnt <= 0) || sci->info->tty->stopped) {
		sci_ctrl_reset(sci, SCI_SCR_TIE);
		return;
	}
	
	sci_outp(sci, SCI_TDR, sci->info->xmit_buf[sci->info->xmit_tail++]);
	sci->info->xmit_tail = sci->info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	sci_outp(sci,SCI_SSR,sci_in(sci,SCI_SSR) & ~SCI_SSR_TDRE);
	sci->info->xmit_cnt--;
	if (sci->info->xmit_cnt < WAKEUP_CHARS)
		rs_sched_event(sci, RS_EVENT_WRITE_WAKEUP);
#ifdef SERIAL_DEBUG_INTR
	printk("THRE...");
#endif
	if (sci->info->xmit_cnt <= 0) {
		sci_ctrl_reset(sci, SCI_SCR_TIE);
	}
}

static _INLINE_ int sci_break_det(struct sci_struct *sci)
{
	return (inb(sci->trxd_io) & sci->rxd_mask)==0;
}

static _INLINE_ void handle_error(struct sci_struct *sci)
{
	struct tty_struct *tty = sci->info->tty;
	int status;
	status=sci_in(sci,SCI_SSR);
	if (status & SCI_SSR_FER) {
		if(sci_break_det(sci)) {
#ifdef SERIAL_DEBUG_INTR
			printk("handling break....");
#endif
			*tty->flip.flag_buf_ptr++ = TTY_BREAK;
			if (sci->info->flags & ASYNC_SAK) {
				do_SAK(tty);
			}
		} else {
			*tty->flip.flag_buf_ptr++ = TTY_FRAME;
		}
        }
	else if (status & SCI_SSR_PER)
		*tty->flip.flag_buf_ptr++ = TTY_PARITY;
	else if (status & SCI_SSR_ORER) 
		*tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
}

static _INLINE_ void check_modem_status(struct async_struct *info)
{
}


static void sci_tx_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	int ch;
	for(ch=0;ch<NR_PORTS;ch++)
		if(rs_table[ch].irq+2==irq)
			transmit_chars(&rs_table[ch]);
}

static void sci_er_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	int ch;
	for(ch=0;ch<NR_PORTS;ch++)
		if(rs_table[ch].irq==irq) {
			handle_error(&rs_table[ch]);
			transmit_chars(&rs_table[ch]);
		}
}

static void sci_rx_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	int ch;
	for(ch=0;ch<NR_PORTS;ch++)
		if(rs_table[ch].irq+1==irq)
			receive_chars(&rs_table[ch]);
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
 * rs_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using rs_sched_event(), and they get done here.
 */
static void do_serial_bh(void)
{
	run_task_queue(&tq_serial);
}

static void do_softint(void *private_)
{
	struct async_struct	*info = (struct async_struct *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	if (clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) {
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
 * 	do_serial_hangup() -> tty->hangup() -> rs_hangup()
 * 
 */
static void do_serial_hangup(void *private_)
{
	struct async_struct	*info = (struct async_struct *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	tty_hangup(tty);
}


/*
 * ---------------------------------------------------------------
 * Low level utility subroutines for the serial driver:  routines to
 * figure out the appropriate timeout for an interrupt chain, routines
 * to initialize and startup a serial port, and routines to shutdown a
 * serial port.  Useful stuff like that.
 * ---------------------------------------------------------------
 */

static int startup(struct sci_struct * sci)
{
	unsigned long flags;
	unsigned long page;

	page = get_free_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	
	save_flags(flags); cli();

	if (sci->info->flags & ASYNC_INITIALIZED) {
		free_page(page);
		restore_flags(flags);
		return 0;
	}

	if (!sci->info->port || !sci->info->type) {
		if (sci->info->tty)
			set_bit(TTY_IO_ERROR, &sci->info->tty->flags);
		free_page(page);
		restore_flags(flags);
		return 0;
	}
	if (sci->info->xmit_buf)
		free_page(page);
	else
		sci->info->xmit_buf = (unsigned char *) page;

#ifdef SERIAL_DEBUG_OPEN
	printk("starting up ttys%d (irq %d)...", info->line, info->irq);
#endif

	if (sci->info->tty)
		clear_bit(TTY_IO_ERROR, &sci->info->tty->flags);
	sci->info->xmit_cnt = sci->info->xmit_head = sci->info->xmit_tail = 0;

	/*
	 * Set up serial timers...
	 */
	timer_table[RS_TIMER].expires = jiffies + 2*HZ/100;
	timer_active |= 1 << RS_TIMER;

	/*
	 * and set the speed of the serial port
	 */
	change_speed(sci);

	sci->info->flags |= ASYNC_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct sci_struct * sci)
{
	unsigned long	flags;

	if (!(sci->info->flags & ASYNC_INITIALIZED))
		return;

#ifdef SERIAL_DEBUG_OPEN
	printk("Shutting down serial port %d (irq %d)....", sci->info->line,
	       sci->info->irq);
#endif
	
	save_flags(flags); cli(); /* Disable interrupts */

	/*
	 * clear delta_msr_wait queue to avoid mem leaks: we may free the irq
	 * here so the queue might never be waken up
	 */
	wake_up_interruptible(&sci->info->delta_msr_wait);
	

	if (sci->info->xmit_buf) {
		free_page((unsigned long) sci->info->xmit_buf);
		sci->info->xmit_buf = 0;
	}

	sci_outp(sci, SCI_SCR, 0x00);	/* disable all intrs */
	
	if (sci->info->tty)
		set_bit(TTY_IO_ERROR, &sci->info->tty->flags);
	
	sci->info->flags &= ~ASYNC_INITIALIZED;
	restore_flags(flags);
}

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct sci_struct *sci)
{
	unsigned cflag;
	unsigned int status;
	unsigned int smr_val;
	unsigned int ddr;
	unsigned int i,t;

	if (!sci->info->tty || !sci->info->tty->termios)
		return;
	cflag = sci->info->tty->termios->c_cflag;

	do
		status = sci_in(sci, SCI_SSR);
	while (!(status & SCI_SSR_TEND));

	sci_out(sci, SCI_SCR, 0x00);	/* TE=0, RE=0, CKE1=0 */

	smr_val = sci_in(sci, SCI_SMR) & 3;
	if ((cflag & CSIZE) == CS7)
		smr_val |= 0x40;
	if (cflag & PARENB)
		smr_val |= 0x20;
	if (cflag & PARODD)
		smr_val |= 0x10;
	if (cflag & CSTOPB)
		smr_val |= 0x08;
	t=0;
	cflag = sci->info->tty->termios->c_cflag;
	i = cflag & CBAUD;
	if (i & CBAUDEX) {
		i &= ~CBAUDEX;
		if (i < 1 || i > 2) 
			sci->info->tty->termios->c_cflag &= ~CBAUDEX;
		else
			i += 15;
	}
	if (i == 15) {
		if ((sci->info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
			i += 1;
		if ((sci->info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
			i += 2;
		if ((sci->info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST)
			t = sci->info->custom_divisor;
	}
	if (t==0)
		t=sci_baud_table[i];

	if (t > 0) {
		if(t >= 256) {
			smr_val = (t >> 8) & 3;
		}
		sci_out(sci, SCI_BRR, t & 0xff);
		udelay(32*(1 << (t*2))*(t & 0xff)*CONFIG_CLK_FREQ); /* Wait one bit interval */
	}
	sci_out(sci, SCI_SMR, smr_val);
	sci_out(sci, SCI_SCR, 0xf0);
	ddr=inb(sci->trxd_ddr);
	ddr &= ~sci->rxd_mask;
        ddr |= sci->txd_mask;
	outb(ddr,sci->trxd_ddr);
}

static void rs_put_char(struct tty_struct *tty, unsigned char ch)
{
        struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	struct async_struct *info = sci->info;
	unsigned long flags;


	if (!tty || !info->xmit_buf)
		return;

	save_flags(flags); cli();
	if (info->xmit_cnt >= SERIAL_XMIT_SIZE - 1) {
		restore_flags(flags);
		return;
	}

	info->xmit_buf[info->xmit_head++] = ch;
	info->xmit_head &= SERIAL_XMIT_SIZE-1;
	info->xmit_cnt++;
	if (sci->info->xmit_cnt && !tty->stopped && !tty->hw_stopped &&
	    !(sci_in(sci,SCI_SCR) & SCI_SCR_TIE)) {
		sci_ctrl_set(sci, SCI_SCR_TIE);
	}
	restore_flags(flags);
	if (sci_in(sci,SCI_SSR) & SCI_SSR_TDRE)
	  /* transmit_chars(&rs_table[ch]); */  
	  transmit_chars(sci);  
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	unsigned long flags;
				
	if (sci->info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
	    !sci->info->xmit_buf)
		return;

	save_flags(flags); cli();
	sci_ctrl_set(sci, SCI_SCR_TIE);
	restore_flags(flags);
}

static int rs_write(struct tty_struct * tty, int from_user,
		    const unsigned char *buf, int count)
{
	int	c, total = 0;
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	unsigned long flags;
				

	if (!tty || !sci->info->xmit_buf || !tmp_buf)
		return 0;
	    
	if (from_user)
		down(&tmp_buf_sem);
	save_flags(flags);
	while (1) {
		cli();		
		c = MIN(count, MIN(SERIAL_XMIT_SIZE - sci->info->xmit_cnt - 1,
				   SERIAL_XMIT_SIZE - sci->info->xmit_head));
		if (c <= 0)
			break;

		if (from_user) {
			memcpy_fromfs(tmp_buf, buf, c);
			c = MIN(c, MIN(SERIAL_XMIT_SIZE - sci->info->xmit_cnt - 1,
				       SERIAL_XMIT_SIZE - sci->info->xmit_head));
			memcpy(sci->info->xmit_buf + sci->info->xmit_head, tmp_buf, c);
		} else
			memcpy(sci->info->xmit_buf + sci->info->xmit_head, buf, c);
		sci->info->xmit_head = (sci->info->xmit_head + c) & (SERIAL_XMIT_SIZE-1);
		sci->info->xmit_cnt += c;
		restore_flags(flags);
		buf += c;
		count -= c;
		total += c;
	}
	if (from_user)
		up(&tmp_buf_sem);
	if (sci->info->xmit_cnt && !tty->stopped && !tty->hw_stopped &&
	    !(sci_in(sci,SCI_SCR) & SCI_SCR_TIE)) {
		sci_ctrl_set(sci, SCI_SCR_TIE);
	}
	restore_flags(flags);
	if (sci_in(sci,SCI_SSR) & SCI_SSR_TDRE)
	        transmit_chars(sci);  
	return total;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	int	ret;
				
	ret = SERIAL_XMIT_SIZE - sci->info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int rs_chars_in_buffer(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
				
	return sci->info->xmit_cnt;
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
				
	cli();
	sci->info->xmit_cnt = sci->info->xmit_head = sci->info->xmit_tail = 0;
	sti();
	wake_up_interruptible(&tty->write_wait);
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);
}

/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct * tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
	if (I_IXOFF(tty))
		sci->info->x_char = STOP_CHAR(tty);

}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	printk("unthrottle %s: %d....\n", _tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif
	
	if (I_IXOFF(tty)) {
		if (sci->info->x_char)
			sci->info->x_char = 0;
		else
			sci->info->x_char = START_CHAR(tty);
	}
}

/*
 * ------------------------------------------------------------
 * rs_ioctl() and friends
 * ------------------------------------------------------------
 */

static int get_serial_info(struct sci_struct * sci,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
  
	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = sci->info->type;
	tmp.line = sci->info->line;
	tmp.port = sci->port;
	tmp.irq = sci->irq;
	tmp.flags = sci->info->flags;
	tmp.baud_base = CONFIG_CLK_FREQ;
	tmp.close_delay = sci->info->close_delay;
	tmp.closing_wait = sci->info->closing_wait;
	tmp.custom_divisor = sci->info->custom_divisor;
	tmp.hub6 = 0;
	memcpy_tofs(retinfo,&tmp,sizeof(*retinfo));
	return 0;
}

static int set_serial_info(struct sci_struct * sci,
			   struct serial_struct * new_info)
{
	struct serial_struct new_serial;
	struct sci_struct old_sci;
	int 			retval = 0;

	if (!new_info)
		return -EFAULT;
	memcpy_fromfs(&new_serial,new_info,sizeof(new_serial));
	old_sci = *sci;

	if (!suser()) {
		if ((new_serial.baud_base != sci->info->baud_base) ||
		    (new_serial.type != sci->info->type) ||
		    (new_serial.close_delay != sci->info->close_delay) ||
		    ((new_serial.flags & ~ASYNC_USR_MASK) !=
		     (sci->info->flags & ~ASYNC_USR_MASK)))
			return -EPERM;
		sci->info->flags = ((sci->info->flags & ~ASYNC_USR_MASK) |
			       (new_serial.flags & ASYNC_USR_MASK));
		sci->info->custom_divisor = new_serial.custom_divisor;
		goto check_and_exit;
	}

	/*
	 * OK, past this point, all the error checking has been done.
	 * At this point, we start making changes.....
	 */

	sci->info->flags = ((sci->info->flags & ~ASYNC_FLAGS) |
			(new_serial.flags & ASYNC_FLAGS));
	sci->info->custom_divisor = new_serial.custom_divisor;
	sci->info->close_delay = new_serial.close_delay * HZ/100;
	sci->info->closing_wait = new_serial.closing_wait * HZ/100;

check_and_exit:
	if (sci->info->flags & ASYNC_INITIALIZED) {
		if (((old_sci.info->flags & ASYNC_SPD_MASK) !=
		     (sci->info->flags & ASYNC_SPD_MASK)) ||
		    (old_sci.info->custom_divisor != sci->info->custom_divisor))
			change_speed(sci);
	} else
		retval = startup(sci);
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
static int get_lsr_info(struct sci_struct * sci, unsigned int *value)
{
	unsigned char status;
	unsigned int result;

	cli();
	status = sci_in(sci, SCI_SSR);
	sti();
	result = ((status & SCI_SSR_TEND) ? TIOCSER_TEMT : 0);
	put_user(result,value);
	return 0;
}


static int get_modem_info(struct sci_struct * sci, unsigned int *value)
{
	unsigned int result;

	result =  TIOCM_RTS|TIOCM_DTR|TIOCM_DSR|TIOCM_CTS;
	put_user(result,value);
	return 0;
}

static int set_modem_info(struct sci_struct * sci, unsigned int cmd,
			  unsigned int *value)
{
	int error;
	unsigned int arg;

	error = verify_area(VERIFY_READ, value, sizeof(int));
	if (error)
		return error;
	arg = get_user(value);
	switch (cmd) {
	case TIOCMBIS: 
		break;
	case TIOCMBIC:
		break;
	case TIOCMSET:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * rs_break() --- routine which turns the break handling on or off
 * adapted from 2.1.124
 */
static void rs_break(struct sci_struct * sci, int break_state)
{
        unsigned long flags;
        
        if (!sci->info->port)
                return;
        save_flags(flags);cli();
        if (break_state == -1) {
		outb(inb(sci->trxd_io) | sci->txd_mask,sci->trxd_io);
		sci_ctrl_reset(sci,SCI_SCR_TE);
	}
        else
		sci_ctrl_set(sci,SCI_SCR_TE);
        restore_flags(flags);
}

/*
 * This routine sends a break character out the serial port.
 */
static void send_break(	struct sci_struct * sci, int duration)
{
	if (!sci->info->port)
		return;
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();
	outb(inb(sci->trxd_io) | sci->txd_mask,sci->trxd_io);
	sci_ctrl_reset(sci,SCI_SCR_TE);
	schedule();
	sci_ctrl_set(sci,SCI_SCR_TE);
	sti();
}

static int rs_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct sci_struct * sci = (struct sci_struct *)tty->driver_data;
	int retval;
	struct async_icount cprev, cnow;	/* kernel counter temps */
	struct serial_icounter_struct *p_cuser;	/* user space */

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
	    (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT) &&
	    (cmd != TIOCMIWAIT) && (cmd != TIOCGICOUNT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
                case TIOCSBRK:  /* Turn break on, unconditionally */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			rs_break(sci,-1);
			return 0;
                case TIOCCBRK:  /* Turn break off, unconditionally */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			rs_break(sci,0);
			return 0;
		case TCSBRK:	/* SVID version: non-zero arg --> no break */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (!arg)
				send_break(sci, HZ/4);	/* 1/4 second */
			return 0;
		case TCSBRKP:	/* support for POSIX tcsendbreak() */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			send_break(sci, arg ? arg*(HZ/10) : HZ/4);
			return 0;
		case TIOCGSOFTCAR:
			error = verify_area(VERIFY_WRITE, (void *) arg,sizeof(long));
			if (error)
				return error;
			put_fs_long(C_CLOCAL(tty) ? 1 : 0,
				    (unsigned long *) arg);
			return 0;
		case TIOCSSOFTCAR:
			error = verify_area(VERIFY_READ, (void *) arg,sizeof(long));
			if (error)
				return error;
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
			return get_modem_info(sci, (unsigned int *) arg);
		case TIOCMBIS:
		case TIOCMBIC:
		case TIOCMSET:
			return set_modem_info(sci, cmd, (unsigned int *) arg);
		case TIOCGSERIAL:
			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct serial_struct));
			if (error)
				return error;
			return get_serial_info(sci,
					       (struct serial_struct *) arg);
		case TIOCSSERIAL:
			error = verify_area(VERIFY_READ, (void *) arg,
						sizeof(struct serial_struct));
			if (error)
				return error;
			return set_serial_info(sci,
					       (struct serial_struct *) arg);
		case TIOCSERCONFIG:
			return 0;

		case TIOCSERGETLSR: /* Get line status register */
			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(unsigned int));
			if (error)
				return error;
			else
			    return get_lsr_info(sci, (unsigned int *) arg);
			return 0;

		case TIOCSERGSTRUCT:
			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct async_struct));
			if (error)
				return error;
			memcpy_tofs((struct async_struct *) arg,
				    sci->info, sizeof(struct async_struct));
			return 0;
			
		/*
		 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
		 * - mask passed in arg for lines of interest
 		 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
		 * Caller should use TIOCGICOUNT to see which one it was
		 */
		 case TIOCMIWAIT:
			cli();
			cprev = sci->info->icount;	/* note the counters on entry */
			sti();
			while (1) {
				interruptible_sleep_on(&sci->info->delta_msr_wait);
				/* see if a signal did it */
				if (current->signal & ~current->blocked)
					return -ERESTARTSYS;
				cli();
				cnow = sci->info->icount;	/* atomic copy */
				sti();
				if (cnow.rng == cprev.rng && cnow.dsr == cprev.dsr && 
				    cnow.dcd == cprev.dcd && cnow.cts == cprev.cts)
					return -EIO; /* no change => error */
				if ( ((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
				     ((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
				     ((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
				     ((arg & TIOCM_CTS) && (cnow.cts != cprev.cts)) ) {
					return 0;
				}
				cprev = cnow;
			}
			/* NOTREACHED */

		/* 
		 * Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
		 * Return: write counters to the user passed counter struct
		 * NB: both 1->0 and 0->1 transitions are counted except for
		 *     RI where only 0->1 is counted.
		 */
		case TIOCGICOUNT:
			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(struct serial_icounter_struct));
			if (error)
				return error;
			cli();
			cnow = sci->info->icount;
			sti();
			p_cuser = (struct serial_icounter_struct *) arg;
			put_user(cnow.cts, &p_cuser->cts);
			put_user(cnow.dsr, &p_cuser->dsr);
			put_user(cnow.rng, &p_cuser->rng);
			put_user(cnow.dcd, &p_cuser->dcd);
			return 0;

		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

static void rs_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct sci_struct *sci = (struct sci_struct *)tty->driver_data;

	if (   (tty->termios->c_cflag == old_termios->c_cflag)
	    && (   RELEVANT_IFLAG(tty->termios->c_iflag) 
		== RELEVANT_IFLAG(old_termios->c_iflag)))
	  return;

	change_speed(sci);

	if ((old_termios->c_cflag & CRTSCTS) &&
	    !(tty->termios->c_cflag & CRTSCTS)) {
		tty->hw_stopped = 0;
		rs_start(tty);
	}
}

/*
 * ------------------------------------------------------------
 * rs_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * async structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void rs_close(struct tty_struct *tty, struct file * filp)
{
	struct sci_struct * sci = (struct sci_struct *)tty->driver_data;
	unsigned long flags;
	unsigned long timeout;

	save_flags(flags); cli();
	
	if (tty_hung_up_p(filp)) {
		DBG_CNT("before DEC-hung");
		MOD_DEC_USE_COUNT;
		restore_flags(flags);
		return;
	}
	
#ifdef SERIAL_DEBUG_OPEN
	printk("rs_close ttys%d, count = %d\n", info->line, info->count);
#endif
	if ((tty->count == 1) && (sci->info->count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  Info->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk("rs_close: bad serial port count; tty->count is 1, "
		       "info->count is %d\n", sci->info->count);
		sci->info->count = 1;
	}
	if (--sci->info->count < 0) {
		printk("rs_close: bad serial port count for ttySC%d: %d\n",
		       sci->info->line, sci->info->count);
		sci->info->count = 0;
	}
	if (sci->info->count) {
		DBG_CNT("before DEC-2");
		MOD_DEC_USE_COUNT;
		restore_flags(flags);
		return;
	}
	sci->info->flags |= ASYNC_CLOSING;
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if (sci->info->flags & ASYNC_NORMAL_ACTIVE)
		sci->info->normal_termios = *tty->termios;
	if (sci->info->flags & ASYNC_CALLOUT_ACTIVE)
		sci->info->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify 
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if (sci->info->closing_wait != ASYNC_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, sci->info->closing_wait);
	/*
	 * At this point we stop accepting input.  To do this, we
	 * disable the receive line status interrupts, and tell the
	 * interrupt driver to stop checking the data ready bit in the
	 * line status register.
	 */
	if (sci->info->flags & ASYNC_INITIALIZED) {
		/*
		 * Before we drop DTR, make sure the UART transmitter
		 * has completely drained; this is especially
		 * important if there is a transmit FIFO!
		 */
		timeout = jiffies+HZ;
		while (!(sci_inp(sci, SCI_SSR) & SCI_SSR_TEND)) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + sci->info->timeout;
			schedule();
			if (jiffies > timeout)
				break;
		}
	}
	shutdown(sci);
	if (tty->driver.flush_buffer)
		tty->driver.flush_buffer(tty);
	if (tty->ldisc.flush_buffer)
		tty->ldisc.flush_buffer(tty);
	tty->closing = 0;
	sci->info->event = 0;
	sci->info->tty = 0;
	if (sci->info->blocked_open) {
		if (sci->info->close_delay) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + sci->info->close_delay;
			schedule();
		}
		wake_up_interruptible(&sci->info->open_wait);
	}
	sci->info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE|
			 ASYNC_CLOSING);
	wake_up_interruptible(&sci->info->close_wait);
	MOD_DEC_USE_COUNT;
	restore_flags(flags);
}

/*
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void rs_hangup(struct tty_struct *tty)
{
	struct sci_struct * sci = (struct sci_struct *)tty->driver_data;
	
	rs_flush_buffer(tty);
	shutdown(sci);
	sci->info->event = 0;
	sci->info->count = 0;
	sci->info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE);
	sci->info->tty = 0;
	wake_up_interruptible(&sci->info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file * filp,
			   struct async_struct *info)
{
	struct wait_queue wait = { current, NULL };
	int		retval;
	int		do_clocal = 0;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (tty_hung_up_p(filp) ||
	    (info->flags & ASYNC_CLOSING)) {
		if (info->flags & ASYNC_CLOSING)
			interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		if (info->flags & ASYNC_HUP_NOTIFY)
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
		if (info->flags & ASYNC_NORMAL_ACTIVE)
			return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_SESSION_LOCKOUT) &&
		    (info->session != current->session))
		    return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_PGRP_LOCKOUT) &&
		    (info->pgrp != current->pgrp))
		    return -EBUSY;
		info->flags |= ASYNC_CALLOUT_ACTIVE;
		return 0;
	}
	
	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
	    (tty->flags & (1 << TTY_IO_ERROR))) {
		if (info->flags & ASYNC_CALLOUT_ACTIVE)
			return -EBUSY;
		info->flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}

	if (info->flags & ASYNC_CALLOUT_ACTIVE) {
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
	 * rs_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->open_wait, &wait);
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready before block: ttys%d, count = %d\n",
	       info->line, info->count);
#endif
	cli();
	if (!tty_hung_up_p(filp)) 
		info->count--;
	sti();
	info->blocked_open++;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		if (tty_hung_up_p(filp) ||
		    !(info->flags & ASYNC_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
			if (info->flags & ASYNC_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;	
#else
			retval = -EAGAIN;
#endif
			break;
		}
		if (!(info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    !(info->flags & ASYNC_CLOSING))
			break;
		if (current->signal & ~current->blocked) {
			retval = -ERESTARTSYS;
			break;
		}
#ifdef SERIAL_DEBUG_OPEN
		printk("block_til_ready blocking: ttys%d, count = %d\n",
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
	printk("block_til_ready after blocking: ttys%d, count = %d\n",
	       info->line, info->count);
#endif
	if (retval)
		return retval;
	info->flags |= ASYNC_NORMAL_ACTIVE;
	return 0;
}	

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its async structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
int sci_open(struct tty_struct *tty, struct file * filp)
{
	struct sci_struct	*sci;
	int 			retval, line;
	unsigned long		page;

	line = MINOR(tty->device) - tty->driver.minor_start;
	if ((line < 0) || (line >= NR_PORTS))
		return -ENODEV;
	sci = &rs_table[line];
	sci->info = &rs_info[line];

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open %s%d, count = %d\n", tty->driver.name, sci->info->line,
	       sci->info->count);
#endif
	sci->info->count++;
	tty->driver_data = sci;
	sci->info->tty = tty;

	if (!tmp_buf) {
		page = get_free_page(GFP_KERNEL);
		if (!page)
			return -ENOMEM;
		if (tmp_buf)
			free_page(page);
		else
			tmp_buf = (unsigned char *) page;
	}
	
	/*
	 * Start up serial port
	 */
	retval = startup(sci);
	if (retval)
		return retval;

	MOD_INC_USE_COUNT;
	retval = block_til_ready(tty, filp, sci->info);
	if (retval) {
#ifdef SERIAL_DEBUG_OPEN
		printk("rs_open returning after block_til_ready with %d\n",
		       retval);
#endif
		return retval;
	}

	if ((sci->info->count == 1) && (sci->info->flags & ASYNC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = sci->info->normal_termios;
		else 
			*tty->termios = sci->info->callout_termios;
		change_speed(sci);
	}

	sci->info->session = current->session;
	sci->info->pgrp = current->pgrp;
	tty->flags = 0;

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open ttys%d successful...", sci->info->line);
#endif
	return 0;
}

/*
 * ---------------------------------------------------------------------
 * rs_init() and friends
 *
 * rs_init() is called at boot-time to initialize the serial driver.
 * ---------------------------------------------------------------------
 */

/*
 * This routine prints out the appropriate serial driver version
 * number, and identifies which options were configured into this
 * driver.
 */
static void show_serial_version(void)
{
 	printk(KERN_INFO "%s version %s\n", serial_name, serial_version);
}

static void init_port(struct sci_struct *sci, int num)
{
  sci->ch=num; 
  sci->info->magic = SERIAL_MAGIC;
  sci->info->line = num;

  printk("%s(%d): info=%x num=%x\n", __FILE__, __LINE__, (int) sci->info, num);

  sci->info->tty = 0;
  sci->info->type = PORT_UNKNOWN;
  sci->info->custom_divisor = 0;
  sci->info->close_delay = 5*HZ/10;
  sci->info->closing_wait = 30*HZ;
  sci->info->x_char = 0;
  sci->info->event = 0;
  sci->info->count = 0;
  sci->info->blocked_open = 0;
  sci->info->tqueue.routine = do_softint;
  sci->info->tqueue.data = sci->info;
  sci->info->tqueue_hangup.routine = do_serial_hangup;
  sci->info->tqueue_hangup.data = sci->info;
  sci->info->callout_termios =callout_driver.init_termios;
  sci->info->normal_termios = serial_driver.init_termios;
  sci->info->open_wait = 0;
  sci->info->close_wait = 0;
  sci->info->delta_msr_wait = 0;
  sci->info->icount.cts = sci->info->icount.dsr = 
	sci->info->icount.rng = sci->info->icount.dcd = 0;
  sci->info->next_port = 0;
  sci->info->prev_port = 0;

  request_irq(sci->irq,sci_er_interrupt,0,"sci",NULL);
  request_irq(sci->irq+1,sci_rx_interrupt,0,"sci",NULL);
  request_irq(sci->irq+2,sci_tx_interrupt,0,"sci",NULL);

  printk(KERN_INFO "ttySC%d at 0x%06x (irq = %d - %d)\n", sci->info->line, 
			 sci->port, sci->irq, sci->irq+3);
}

/*
 * The serial driver boot-time initialization code!
 */

static void init_sci_console(int ch);
static void sci_console_print(const char *msg);

#if defined(CONFIG_CONSOLE_2400BPS)
#define CONSOLE_BAUD B2400
#endif
#if defined(CONFIG_CONSOLE_4800BPS)
#define CONSOLE_BAUD B4800
#endif
#if defined(CONFIG_CONSOLE_9600BPS)
#define CONSOLE_BAUD B9600
#endif
#if defined(CONFIG_CONSOLE_19200BPS)
#define CONSOLE_BAUD B19200
#endif
#if defined(CONFIG_CONSOLE_38400BPS)
#define CONSOLE_BAUD B38400
#endif

int sci_init(void)
{
	int i;
	struct async_struct * info;
	struct sci_struct * sci;
	init_bh(SERIAL_BH, do_serial_bh);
#if 0
	timer_table[RS_TIMER].fn = rs_timer;
	timer_table[RS_TIMER].expires = 0;
#endif
	show_serial_version();

	/* Initialize the tty_driver structure */
	
	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic = TTY_DRIVER_MAGIC;
	serial_driver.name = "ttySC";
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64;
	serial_driver.num = NR_PORTS;
	serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	serial_driver.subtype = SERIAL_TYPE_NORMAL;
	serial_driver.init_termios = tty_std_termios;
	serial_driver.init_termios.c_cflag =
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	serial_driver.flags = TTY_DRIVER_REAL_RAW;
	serial_driver.refcount = &serial_refcount;
	serial_driver.table = serial_table;
	serial_driver.termios = serial_termios;
	serial_driver.termios_locked = serial_termios_locked;

	serial_driver.open = sci_open;
	serial_driver.close = rs_close;
	serial_driver.write = rs_write;
	serial_driver.put_char = rs_put_char;
	serial_driver.flush_chars = rs_flush_chars;
	serial_driver.write_room = rs_write_room;
	serial_driver.chars_in_buffer = rs_chars_in_buffer;
	serial_driver.flush_buffer = rs_flush_buffer;
	serial_driver.ioctl = rs_ioctl;
	serial_driver.throttle = rs_throttle;
	serial_driver.unthrottle = rs_unthrottle;
	serial_driver.set_termios = rs_set_termios;
	serial_driver.stop = rs_stop;
	serial_driver.start = rs_start;
	serial_driver.hangup = rs_hangup;

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
	
	for (i = 0, sci = rs_table, info=rs_info ; i < NR_PORTS; i++,sci++,info++) {
		sci->info = info;
		init_port(sci, i);
	};
	init_sci_console(CONFIG_CONSOLE_CHANNEL);
	register_console(sci_console_print);
	return 0;
}

static void init_sci_console(int ch)
{
	short t=sci_baud_table[CONSOLE_BAUD];
	long page;
	console_tty.driver_data=&rs_table[CONFIG_CONSOLE_CHANNEL];
	console_tty.stopped=0;
        console_tty.hw_stopped=0;
	page = get_free_page(GFP_KERNEL);
	rs_table[CONFIG_CONSOLE_CHANNEL].info->xmit_buf=page;
	outb(0x00,rs_table[ch].port+SCI_SCR);
	outb((t>>8) & 0xff,rs_table[ch].port+SCI_SMR);
	outb((t & 0xff),rs_table[ch].port+SCI_BRR);
	udelay(32*(1 << (t*2))*(t & 0xff)*CONFIG_CLK_FREQ);
	outb(0xf0,rs_table[ch].port+SCI_SCR);
}	

static void sci_console_print(const char *msg)
{
	while(*msg) {
		if(*msg == '\n')
			rs_put_char(&console_tty,'\r');
		rs_put_char(&console_tty,*msg);
		msg++;
	}
}

#ifdef MODULE
int init_module(void)
{
	return sci_init();
}

void cleanup_module(void) 
{
	unsigned long flags;
	int e1, e2;
	int i;

	/* printk("Unloading %s: version %s\n", serial_name, serial_version); */
	save_flags(flags);
	cli();
	timer_active &= ~(1 << RS_TIMER);
	timer_table[RS_TIMER].fn = NULL;
	timer_table[RS_TIMER].expires = 0;
	if ((e1 = tty_unregister_driver(&serial_driver)))
		printk("SERIAL: failed to unregister serial driver (%d)\n",
		       e1);
	if ((e2 = tty_unregister_driver(&callout_driver)))
		printk("SERIAL: failed to unregister callout driver (%d)\n", 
		       e2);
	restore_flags(flags);

	for (i = 0; i < NR_PORTS; i++) {
		release_region(rs_table[i].port, 8);
	}

	if (tmp_buf) {
		free_page((unsigned long) tmp_buf);
		tmp_buf = NULL;
	}
}
#endif /* MODULE */
