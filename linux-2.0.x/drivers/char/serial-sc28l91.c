/* serial port driver for Philips UARTs.
 *
 * Copyright (C) 2000, 2001  Erik Andersen <andersen@lineo.com>
 *
 * Based on:
 * drivers/char/68302serial.c
 */

/* Enable this to force it to always be 57600 */
#undef FORCE_57600

#include <linux/serial.h>
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
#include <asm/arch-atmel/irq.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/delay.h>
#include "serial-sc28l91.h"

#define USE_INTS	1
#define US_NB		1
#define UART_CLOCK	(ARM_CLK/16)

//#define XMIT_SERIAL_SIZE	PAGE_SIZE
#define SERIAL_XMIT_SIZE	16
#define RX_SERIAL_SIZE		16

#define BR_EXT_9600	3
#define BR_EXT_57600	3


static uart_sc28l91 uarts[US_NB] = {
	(uart_sc28l91) CONFIG_SC28L91_UART_BASE
};

static struct sc28l91_serial sc28l91_info[US_NB];

static unsigned char uart_mem[16];

static struct tty_struct sc28l91_ttys[US_NB];

/* Console hooks... */
static struct sc28l91_serial *sc28l91_consinfo = 0;

DECLARE_TASK_QUEUE(tq_philips_serial);

static struct tq_struct serialpoll;
static struct tty_driver serial_driver, callout_driver;
static static int serial_refcount;

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2

/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

/* Debugging... DEBUG_INTR is bad to use when one of the zs
 * lines is your console ;(
 */
#undef SERIAL_DEBUG_INTR
#undef SERIAL_DEBUG_OPEN
#undef SERIAL_DEBUG_FLOW

#define RS_ISR_PASS_LIMIT 256

#define _INLINE_

static void serpoll(void *data);

static void change_speed(struct sc28l91_serial *info);

static struct tty_struct *serial_table[US_NB];
static struct termios *serial_termios[US_NB];
static struct termios *serial_termios_locked[US_NB];

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

static char prompt0;
static void xmit_char(struct sc28l91_serial *info, char ch);
static void xmit_string(struct sc28l91_serial *info, char *p, int len);
static void start_rx(struct sc28l91_serial *info);
static void wait_EOT(uart_sc28l91);
static void uart_init(struct sc28l91_serial *info);
static void uart_speed(struct sc28l91_serial *info, unsigned cflag);

static void tx_enable(uart_sc28l91 uart);
static void rx_enable(uart_sc28l91 uart);
static void tx_disable(uart_sc28l91 uart);
static void rx_disable(uart_sc28l91 uart);
static void tx_stop(uart_sc28l91 uart);
static void tx_start(uart_sc28l91 uart, int ints);
static void rx_stop(uart_sc28l91 uart);
static void rx_start(uart_sc28l91 uart, int ints);
static void set_ints_mode(int yes, struct sc28l91_serial *info);
static void rs_interrupt(struct sc28l91_serial *info);
extern void show_net_buffers(void);
extern void hard_reset_now(void);
static void pause(int msecs);

static int global;

static void pause(int msecs)
{
    int i,j=msecs*700;
    for(i=0; i<j; i++);
}

static void coucou1(void)
{
	global = 0;
}

static void coucou2(void)
{
	global = 1;
}

/*
static
void
uart_reg_set(uart_sc28l91 uart, int reg, unsigned char byte)
{
	uart_mem[reg] = byte;
	uart[reg] = uart_mem[reg];
}

static
unsigned char
uart_reg_or(uart_sc28l91 uart, int reg, unsigned char byte)
{
	uart_mem[reg] |= byte;
	uart[reg] = uart_mem[reg];

	return uart_mem[reg];
}

static
unsigned char
uart_reg_and(uart_sc28l91 uart, int reg, unsigned char byte)
{
	uart_mem[reg] &= byte;
	uart[reg] = uart_mem[reg];

	return uart_mem[reg];
}
*/
static void _INLINE_ tx_enable(volatile uart_sc28l91 uart)
{
	uart[CR] = 0x04;
}
static void _INLINE_ rx_enable(volatile uart_sc28l91 uart)
{
	uart[CR] = 0x01;
}
static void _INLINE_ tx_disable(volatile uart_sc28l91 uart)
{
	uart[CR] = 0x08;
}
static void _INLINE_ rx_disable(volatile uart_sc28l91 uart)
{
	uart[CR] = 0x02;
}
static void _INLINE_ tx_stop(volatile uart_sc28l91 uart)
{
	tx_disable(uart);
}
static void _INLINE_ tx_start(volatile uart_sc28l91 uart, int ints)
{
//	if (ints) {
		tx_enable(uart);
//	}
}
static void _INLINE_ rx_stop(volatile uart_sc28l91 uart)
{
	rx_disable(uart);
}
static void _INLINE_ rx_start(volatile uart_sc28l91 uart, int ints)
{
//	if (ints) {
		rx_enable(uart);
//	}
}
static void _INLINE_ reset_status(volatile uart_sc28l91 uart)
{
	/* FIXME  -- This needs to be adapted to the philips UART */
#ifdef FIXME
	uart->cr = US_RSTSTA;
#endif	/* FIXME */
}
static void set_ints_mode(int yes, struct sc28l91_serial *info)
{
	info->use_ints = yes;
	(yes) ? unmask_irq(info->irq) : mask_irq(info->irq);
}

/*
 * tmp_buf is used as a temporary buffer by serial_write.  We need to
 * lock it in case the memcpy_fromfs blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
//static unsigned char tmp_buf[XMIT_SERIAL_SIZE];	/* This is cheating */
static unsigned char tmp_buf[SERIAL_XMIT_SIZE];	/* This is cheating */
static struct semaphore tmp_buf_sem = MUTEX;

static unsigned char rx_buf1[RX_SERIAL_SIZE];
static unsigned char rx_buf2[RX_SERIAL_SIZE];


static void sc28l91_cts_off(struct sc28l91_serial *info)
{
	uart_sc28l91 uart;
	unsigned char junk;

	uart = info->uart;
/*
	// Tell REGISTER0 to be MR1
	uart[CR] = 0x10;
	pause(1);
	// Now read something worthless from MR1 so REGISTER0 will go to MR2
	junk=uart[MR];
	// Now (finally) we can poke at MR2
	uart_reg_or(uart, MR,
*/
	// clear CTS bit in MR2
	uart[MR] &= 0xef;
}
static void sc28l91_cts_on(struct sc28l91_serial *info)
{
	uart_sc28l91 uart;
	unsigned char junk;

	uart = info->uart;
/*
	// Tell REGISTER0 to be MR1
	uart[CR] |= 0x10;
	pause(1);
	// Now read something worthless from MR1 so REGISTER0 will go to MR2
	junk=uart[MR];
	// Now (finally) we can poke at MR2
*/
	// set CTS bit in MR2
	uart[MR] |= 0x10;
}


static inline int serial_paranoia_check(struct sc28l91_serial *info,
										dev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%d, %d) in %s\n";
	static const char *badinfo =
		"Warning: null sc28l91_serial struct for (%d, %d) in %s\n";

	if (!info) {
		printk(badinfo, MAJOR(device), MINOR(device), routine);
		return 1;
	}
	if (info->magic != SERIAL_MAGIC) {
		printk(badmagic, MAJOR(device), MINOR(device), routine);
		return 1;
	}
#endif
	return 0;
}

/* Sets or clears DTR/RTS on the requested line */
static inline void sc28l91_rtsdtr(struct sc28l91_serial *ss, int set)
{
	uart_sc28l91 uart;

	uart = ss->uart;
	if (set) {
		uart[CR] = 0x80;
	} else {
		uart[CR] = 0x90;
	}
	pause(1);
	return;
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
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_stop"))
		return;

	save_flags(flags);
	cli();
	tx_stop(info->uart);
	rx_stop(info->uart);
	restore_flags(flags);
}

static void rs_put_char(struct sc28l91_serial *info, char ch)
{
	int flags = 0;

	save_flags(flags);
	cli();
	wait_EOT(info->uart);
	xmit_char(info, ch);
	restore_flags(flags);
}

static void rs_start(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_start"))
		return;

	save_flags(flags);
	cli();
	tx_start(info->uart, info->use_ints);
	start_rx(info);
	restore_flags(flags);
}

/* Drop into either the boot monitor or kadb upon receiving a break
 * from keyboard/console input.
 */
static void batten_down_hatches(void)
{
#if 0
	/* If we are doing kadb, we call the debugger
	 * else we just drop into the boot monitor.
	 * Note that we must flush the user windows
	 * first before giving up control.
	 */
	if ((((unsigned long) linux_dbvec) >= DEBUG_FIRSTVADDR) &&
		(((unsigned long) linux_dbvec) <= DEBUG_LASTVADDR))
		sp_enter_debugger();
	else
		panic("sc28l91_serial: batten_down_hatches");
	return;
#endif
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
 * and look at the resulting assembly code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct sc28l91_serial *info, int event)
{
	info->event |= 1 << event;
	queue_task_irq_off(&info->tqueue, &tq_philips_serial);
	mark_bh(SERIAL_BH);
}

extern void breakpoint(void);	/* For the KGDB frame character */

static _INLINE_ void receive_chars(struct sc28l91_serial *info, u_32 status)
{
//	unsigned char ch;
	int count;
	int counter;
	uart_sc28l91 uart = info->uart;

	struct tty_struct *tty = info->tty;

	if (!(info->flags & S_INITIALIZED))
		return;
	
	count = 0;
	while ((info->uart[SR] & 0x01) && count < RX_SERIAL_SIZE)
	{
		info->rx_buf[count++] = info->uart[RxFIFO];
	}
	
	if (count == 0) {
		return;
	}
	
//	ch = info->rx_buf[0];
#if 0
	if (info->is_cons) {
		if (status & US_RXBRK) {	/* whee, break received */
			batten_down_hatches();
			/*rs_recv_clear(info->sc28l91_channel); */
			return;
		} else if (ch == 0x10) {	/* ^P */
			show_state();
			show_free_areas();
			show_buffers();
			show_net_buffers();
			return;
		} else if (ch == 0x12) {	/* ^R */
			hard_reset_now();
			return;
		}
		/* It is a 'keyboard interrupt' ;-) */
		wake_up(&keypress_wait);
	}
#endif

	if (!tty)
		goto clear_and_exit;

	if (tty->flip.count >= TTY_FLIPBUF_SIZE)
		queue_task_irq_off(&tty->flip.tqueue, &tq_timer);

	if ((count + tty->flip.count) >= TTY_FLIPBUF_SIZE) {
		sc28l91_cts_off(info);
		serialpoll.data = (void *) info;
		queue_task_irq_off(&serialpoll, &tq_timer);
	}
	memset(tty->flip.flag_buf_ptr, 0, count);
	memcpy(tty->flip.char_buf_ptr, info->rx_buf, count);
	tty->flip.char_buf_ptr += count;

	if (status & US_PARE)
		*(tty->flip.flag_buf_ptr - 1) = TTY_PARITY;
	else if (status & US_OVRE)
		*(tty->flip.flag_buf_ptr - 1) = TTY_OVERRUN;
	else if (status & US_FRAME)
		*(tty->flip.flag_buf_ptr - 1) = TTY_FRAME;

	tty->flip.count += count;

	queue_task_irq_off(&tty->flip.tqueue, &tq_timer);

  clear_and_exit:
	start_rx(info);
	return;

}

static _INLINE_ void transmit_chars(struct sc28l91_serial *info)
{
	if (info->x_char) {
		/* Send next char */
		xmit_char(info, info->x_char);
		info->x_char = 0;
		goto clear_and_return;
	}

	if ((info->xmit_cnt <= 0) || info->tty->stopped) {
		/* That's peculiar... */
		tx_stop(info->uart);
		goto clear_and_return;
	}

	if (info->xmit_tail + info->xmit_cnt < SERIAL_XMIT_SIZE) {
		xmit_string(info, info->xmit_buf + info->xmit_tail,
					info->xmit_cnt);
		info->xmit_tail =
			(info->xmit_tail + info->xmit_cnt) & (SERIAL_XMIT_SIZE - 1);
		info->xmit_cnt = 0;
	} else {
		coucou1();
		xmit_string(info, info->xmit_buf + info->xmit_tail,
					SERIAL_XMIT_SIZE - info->xmit_tail);
		//xmit_string(info, info->xmit_buf, info->xmit_tail + info->xmit_cnt - SERIAL_XMIT_SIZE);
		info->xmit_cnt =
			info->xmit_cnt - (SERIAL_XMIT_SIZE - info->xmit_tail);
		info->xmit_tail = 0;
	}

#if 0
	/* Send char */
	xmit_char(info, info->xmit_buf[info->xmit_tail++]);
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE - 1);
	info->xmit_cnt--;
#endif

	if (info->xmit_cnt < WAKEUP_CHARS)
		rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);

	if (info->xmit_cnt <= 0) {
		//tx_stop(info->uart);
		goto clear_and_return;
	}

  clear_and_return:
	/* Clear interrupt (should be auto) */
	return;
}

static _INLINE_ void status_handle(struct sc28l91_serial *info, u_32 status)
{
#if 0
	if (status & DCD) {
		if ((info->tty->termios->c_cflag & CRTSCTS) &&
			((info->curregs[3] & AUTO_ENAB) == 0)) {
			info->curregs[3] |= AUTO_ENAB;
			info->pendregs[3] |= AUTO_ENAB;
			write_zsreg(info->sc28l91_channel, 3, info->curregs[3]);
		}
	} else {
		if ((info->curregs[3] & AUTO_ENAB)) {
			info->curregs[3] &= ~AUTO_ENAB;
			info->pendregs[3] &= ~AUTO_ENAB;
			write_zsreg(info->sc28l91_channel, 3, info->curregs[3]);
		}
	}
#endif
	/* Whee, if this is console input and this is a
	 * 'break asserted' status change interrupt, call
	 * the boot prom.
	 */
	if ((status & US_RXBRK) && info->break_abort)
		batten_down_hatches();

	/* XXX Whee, put in a buffer somewhere, the status information
	 * XXX whee whee whee... Where does the information go...
	 */
	reset_status(info->uart);
	return;
}


/*
 * This is the serial driver's generic interrupt routine
 */
static void rs_interrupta(int irq, void *dev_id, struct pt_regs *regs)
{
	rs_interrupt(&sc28l91_info[0]);
}
static void rs_interruptb(int irq, void *dev_id, struct pt_regs *regs)
{
	rs_interrupt(&sc28l91_info[1]);
}
static static void rs_interrupt(struct sc28l91_serial *info)
{
	u_32 status;
	uart_sc28l91 uart=info->uart;

	status = uart[SR];
//	if (status & (0x1 | (0x1<<3))) {
	// receive if RxRDY set
	if (status & 0x01) {
		receive_chars(info, status);
	}
	// transmit if TxRDy set
	if (status & 0x04) {
		transmit_chars(info);
	}
	status_handle(info, status);

	if (!info->cts_state) {
		if (info->tty->flip.count < TTY_FLIPBUF_SIZE - RX_SERIAL_SIZE) {
			sc28l91_cts_on(info);
		}
	}
	if (!info->use_ints) {
//	if(1){
		serialpoll.data = (void *) info;
		queue_task_irq_off(&serialpoll, &tq_timer);
	}
	return;
}
static void serpoll(void *data)
{
	struct sc28l91_serial *info = data;

	rs_interrupt(info);
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
	run_task_queue(&tq_philips_serial);
}

static void do_softint(void *private_)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) private_;
	struct tty_struct *tty;

	tty = info->tty;
	if (!tty)
		return;

	if (clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) {
		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
			tty->ldisc.write_wakeup) (tty->ldisc.write_wakeup) (tty);
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
	struct sc28l91_serial *info = (struct sc28l91_serial *) private_;
	struct tty_struct *tty;

	tty = info->tty;
	if (!tty)
		return;

	tty_hangup(tty);
}


/*
 * This subroutine is called when the RS_TIMER goes off.  It is used
 * by the serial driver to handle ports that do not have an interrupt
 * (irq=0).  This doesn't work at all for 16450's, as a sun has a Z8530.
 */

static void rs_timer(void)
{
	panic("rs_timer called\n");
	return;
}

static u_32 calcCD(u_32 br)
{
	return (UART_CLOCK / br);
}

static void uart_init(struct sc28l91_serial *info)
{
	uart_sc28l91 *uart;
	int counter;

	if (info) {
		uart = info->uart;
	} else {
		uart = uarts[0];
	}

/*
    // init uart_mem
    for (counter = 0; counter < 16; counter++)
    	uart_mem[counter] = 0;
*/

#if 0
    // disable tx and rx
    uart[CR] = 0x0a;

    // reset UART
    //
    uart[CR] = 0x20; // reset receiver
    pause(1);

    uart[CR] = 0x30; // reset transmitter
    pause(1);

    uart[CR] = 0x40; // reset error status
    pause(1);

    uart[CR] = 0x50; // reset break change interrupt
    pause(1);

    // set mode
    //
    uart[CR] = 0x80; // assert RTS
    pause(1);

    uart[CR] = 0xb0; // set MR0
    pause(1);

    uart[MR] = 0x0c; // MR0, 16 byte FIFO, baudrate extended mode II

    uart[CR] = 0x10; // set MR1
    pause(1);

    uart[MR] = 0x13; // MR1, Rx controls RTS, no parity, 8 bits

    uart[MR] = 0x07; // MR2, , disable CTS, 1 stop bit

    // CTU (MSB)
//    uart[CTU] = 0x00;

    // set baud rate
#ifdef FORCE_57600
    uart[CSR] = 0x33; //(BR_EXT_57600 << 4) + BR_EXT_57600; // Baudrate, Extended mode II & ACR[7] = 0
#else
    uart[CSR] = 0x33; //(BR_EXT_9600 << 4) + BR_EXT_9600; // Baudrate, Extended mode II & ACR[7] = 0
#endif

//    uart[ACR] = 0x60; // Timer mode-CLK, TxC, set ISR
    // set interrupt mask
    uart[IMR] = 0x02; // RxRDY

    // enable rx and tx
    uart[CR] = 0x05;
#endif
}

static void uart_speed(struct sc28l91_serial *info, unsigned cflag)
{
	unsigned baud = info->baud;
	uart_sc28l91 uart = info->uart;

	/* FIXME  -- This needs to be adapted to the philips UART */
#ifdef	FIXME

#ifdef FORCE_57600
	return;
#endif
	uart->cr = US_TXDIS | US_RXDIS;
	uart->ier = 0;
	uart->idr = US_ALL_INTS;
	uart->brgr = calcCD(baud);
	uart->rtor = 20;			// timeout = value * 4 *bit period  
	uart->ttgr = 0;				// no guard time    
	uart->rpr = 0;
	uart->rcr = 0;
	uart->tpr = 0;
	uart->tcr = 0;
	uart->mc = 0;
	if (cflag != 0xffff) {
		uart->mr = US_USCLKS(0) | US_CLK0 | US_CHMODE(0) | US_PAR(0);

		if ((cflag & CSIZE) == CS8)
			uart->mr |= US_CHRL(3);	// 8 bit char
		else
			uart->mr |= US_CHRL(2);	// 7 bit char

		if (cflag & CSTOPB)
			uart->mr |= US_NBSTOP(2);	// 2 stop bits

		if (!(cflag & PARENB))
			uart->mr |= US_PAR(4);	// parity disabled
		else if (cflag & PARODD)
			uart->mr |= US_PAR(1);	// odd parity
	}
	tx_start(uart, info->use_ints);
	start_rx(info);
#endif	 /*FIXME*/
}
static void wait_EOT(uart_sc28l91 uart)
{
	volatile u_32 status;

	tx_enable(uart);
	while (1) {
//		status = uart[SR];
		if (uart[SR] & TxRDY)
			break;
	}
}
static int startup(struct sc28l91_serial *info)
{
	unsigned long flags;

	if (info->flags & S_INITIALIZED)
		return 0;

	if (!info->xmit_buf) {
		info->xmit_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!info->xmit_buf)
			return -ENOMEM;
	}
	if (!info->rx_buf) {
		info->rx_buf = rx_buf1;
		if (!info->rx_buf)
			return -ENOMEM;
	}
	save_flags(flags);
	cli();
#ifdef SERIAL_DEBUG_OPEN
	printk("starting up ttyS%d (irq %d)...\n", info->line, info->irq);
#endif
	/*
	 * Clear the FIFO buffers and disable them
	 * (they will be reenabled in change_speed())
	 */

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * and set the speed of the serial port
	 */

	uart_init(info);
//	set_ints_mode(0, info);
	change_speed(info);
	info->flags |= S_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct sc28l91_serial *info)
{
	unsigned long flags;

	tx_disable(info->uart);
	rx_disable(info->uart);
	rx_stop(info->uart);		/* All off! */
	if (!(info->flags & S_INITIALIZED))
		return;

#ifdef SERIAL_DEBUG_OPEN
	printk("Shutting down serial port %d (irq %d)....\n", info->line,
		   info->irq);
#endif

	save_flags(flags);
	cli();						/* Disable interrupts */

	if (info->xmit_buf) {
		free_page((unsigned long) info->xmit_buf);
		info->xmit_buf = 0;
	}

	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);

	info->flags &= ~S_INITIALIZED;
	restore_flags(flags);
}

/* rate = 1036800 / ((65 - prescale) * (1<<divider)) */

static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 0
};

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct sc28l91_serial *info)
{
	unsigned cflag;
	int i;

	if (!info->tty || !info->tty->termios)
		return;
	cflag = info->tty->termios->c_cflag;

	/* First disable the interrupts */
//	set_ints_mode(0, info);
	tx_stop(info->uart);
	rx_stop(info->uart);
	
	/* set the baudrate */
	i = cflag & CBAUD;

	info->baud = baud_table[i];
	uart_speed(info, cflag);
	
	/* start it and enable interrupts */
	tx_start(info->uart, info->use_ints);
	rx_start(info->uart, info->use_ints);
//	start_rx(info);
//	set_ints_mode(1, info);
	
	return;
}
static void start_rx(struct sc28l91_serial *info)
{
	uart_sc28l91 uart = info->uart;

	rx_stop(uart);
	
	/* FIXME  -- This needs to be adapted to the philips UART */
#ifdef FIXME
	if (info->rx_buf == rx_buf1) {
		info->rx_buf = rx_buf2;
	} else {
		info->rx_buf = rx_buf1;
	}
	uart->rpr = (u_32) info->rx_buf;
	uart->rcr = (u_32) RX_SERIAL_SIZE;
#endif	/* FIXME */
	rx_start(uart, info->use_ints);
}
static void xmit_char(struct sc28l91_serial *info, char ch)
{
	uart_sc28l91 uart=info->uart;

	prompt0 = ch;
//    uart[TxFIFO] = ch;
	xmit_string(info, &prompt0, 1);
}
static void xmit_string(struct sc28l91_serial *info, char *p, int len)
{
    int counter = 0;

    while(counter < len)
    {
    	// wait until TxRDY
    	while(!(info->uart[SR] & 0x04));
   		
		// write byte to fifo
		info->uart[TxFIFO] = p[counter++];
    }

#ifdef FIXME
	info->uart->tcr = 0;
	info->uart->tpr = (u_32) p;
	tx_start(info->uart, info->use_ints);
	info->uart->tcr = (u_32) len;
#endif	/* FIXME */
}

/*
 * sc28l91_console_print is registered for printk.
 */
static int sc28l91_console_initialized;

static void init_console(struct sc28l91_serial *info)
{
	memset(info, 0, sizeof(struct sc28l91_serial));

	info->uart = (uart_sc28l91) CONFIG_SC28L91_UART_BASE;
	info->irqmask = AIC_IRQ2;
	info->irq = IRQ_IRQ2;
	
	info->tty = 0;
	info->port = 0;
	info->use_ints = 0;
	info->cts_state = 1;
	info->is_cons = 1;
	sc28l91_console_initialized = 1;
}


void console_print_sc28l91(const char *p)
{
	char c;
	struct sc28l91_serial *info;

	info = &sc28l91_info[0];

	if (!sc28l91_console_initialized) {
		init_console(info);
		uart_init(info);
		info->baud = 57600;
		uart_speed(info, 0xffff);
	}

	while ((c = *(p++)) != 0) {
		if (c == '\n')
			rs_put_char(info, '\r');
		rs_put_char(info, c);
	}

	/* Comment this if you want to have a strict interrupt-driven output */
#if 0
	if (!info->use_ints)
	    rs_fair_output(info);
#endif

	return;
}

static void rs_set_ldisc(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_set_ldisc"))
		return;

	info->is_cons = (tty->termios->c_line == N_TTY);

	printk("ttyS%d console mode %s\n", info->line,
		   info->is_cons ? "on" : "off");
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_flush_chars"))
		return;
	if (!info->use_ints) {
		for (;;) {
			if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
				!info->xmit_buf) return;

			/* Enable transmitter */
			save_flags(flags);
			cli();
			tx_start(info->uart, info->use_ints);
		}
	} else {
		if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
			!info->xmit_buf) return;

		/* Enable transmitter */
		save_flags(flags);
		cli();
		tx_start(info->uart, info->use_ints);
	}

	if (!info->use_ints)
		wait_EOT(info->uart);
	/* Send char */
	xmit_char(info, info->xmit_buf[info->xmit_tail++]);
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE - 1);
	info->xmit_cnt--;

	restore_flags(flags);
}

extern void console_printn(const char *b, int count);

static int rs_write(struct tty_struct *tty, int from_user,
					const unsigned char *buf, int count)
{
	int c, total = 0;
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_write"))
		return 0;

	if (!tty || !info->xmit_buf)
		return 0;

	/*buf = "123456";
	   count = 6;

	   printk("Writing '%s' to serial port\n", buf); */

	/*printk("rs_write of %d bytes\n", count); */


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
			memcpy(info->xmit_buf + info->xmit_head, tmp_buf, c);
			up(&tmp_buf_sem);
		} else {
			memcpy(info->xmit_buf + info->xmit_head, buf, c);
		}
		info->xmit_head = (info->xmit_head + c) & (SERIAL_XMIT_SIZE - 1);
		info->xmit_cnt += c;
		restore_flags(flags);
		buf += c;
		count -= c;
		total += c;
	}

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped) {
		/* Enable transmitter */

		cli();
		/*printk("Enabling transmitter\n"); */

		if (!info->use_ints) {
			while (info->xmit_cnt) {
				wait_EOT(info->uart);
				/* Send char */
				xmit_char(info, info->xmit_buf[info->xmit_tail++]);
				wait_EOT(info->uart);
				info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE - 1);
				info->xmit_cnt--;
			}
		} else {
			if (info->xmit_cnt) {
				/* Send char */
				wait_EOT(info->uart);
				if (info->xmit_tail + info->xmit_cnt < SERIAL_XMIT_SIZE) {
					xmit_string(info, info->xmit_buf + info->xmit_tail,
								info->xmit_cnt);
					info->xmit_tail =
						(info->xmit_tail +
						 info->xmit_cnt) & (SERIAL_XMIT_SIZE - 1);
					info->xmit_cnt = 0;
				} else {
					coucou2();
					xmit_string(info, info->xmit_buf + info->xmit_tail,
								SERIAL_XMIT_SIZE - info->xmit_tail);
					//xmit_string(info, info->xmit_buf, info->xmit_tail + info->xmit_cnt - SERIAL_XMIT_SIZE);
					info->xmit_cnt =
						info->xmit_cnt - (SERIAL_XMIT_SIZE - info->xmit_tail);
					info->xmit_tail = 0;
				}
			}
		}
	} else {
		/*printk("Skipping transmit\n"); */
	}

#if 0
	printk("Enabling stuff anyhow\n");
	tx_start(0);

	if (SCC_EOT(0, 0)) {
		printk("TX FIFO empty.\n");
		/* Send char */
		sc28l91_xmit_char(info->uart, info->xmit_buf[info->xmit_tail++]);
		info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE - 1);
		info->xmit_cnt--;
	}
#endif

	restore_flags(flags);
	return total;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	int ret;

	if (serial_paranoia_check(info, tty->device, "rs_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int rs_chars_in_buffer(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	unsigned long flags;
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_flush_buffer"))
		return;
	save_flags(flags);
	cli();
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
	restore_flags(flags);
	wake_up_interruptible(&tty->write_wait);
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		tty->ldisc.write_wakeup) (tty->ldisc.write_wakeup) (tty);
}

/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

#ifdef SERIAL_DEBUG_THROTTLE
	char buf[64];

	printk("throttle %s: %d....\n", _tty_name(tty, buf),
		   tty->ldisc.chars_in_buffer(tty));
#endif

	if (serial_paranoia_check(info, tty->device, "rs_throttle"))
		return;

	if (I_IXOFF(tty))
		info->x_char = STOP_CHAR(tty);

	/* Turn off RTS line (do this atomic) */
}

static void rs_unthrottle(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

#ifdef SERIAL_DEBUG_THROTTLE
	char buf[64];

	printk("unthrottle %s: %d....\n", _tty_name(tty, buf),
		   tty->ldisc.chars_in_buffer(tty));
#endif

	if (serial_paranoia_check(info, tty->device, "rs_unthrottle"))
		return;

	if (I_IXOFF(tty)) {
		if (info->x_char)
			info->x_char = 0;
		else
			info->x_char = START_CHAR(tty);
	}

	/* Assert RTS line (do this atomic) */
}

/*
 * ------------------------------------------------------------
 * rs_ioctl() and friends
 * ------------------------------------------------------------
 */

static int get_serial_info(struct sc28l91_serial *info, 
	struct serial_struct *retinfo)
{
	struct serial_struct tmp;

	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.irq = info->irq;
	tmp.port = info->port;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	tmp.custom_divisor = info->custom_divisor;
	memcpy_tofs(retinfo, &tmp, sizeof(*retinfo));
	return 0;
}

static int set_serial_info(struct sc28l91_serial *info, 
	struct serial_struct *new_info)
{
	struct serial_struct new_serial;
	struct sc28l91_serial old_info;
	int retval = 0;

	if (!new_info)
		return -EFAULT;
	memcpy_fromfs(&new_serial, new_info, sizeof(new_serial));
	old_info = *info;

	if (!suser()) {
		if ((new_serial.baud_base != info->baud_base) ||
			(new_serial.type != info->type) ||
			(new_serial.close_delay != info->close_delay) ||
			((new_serial.flags & ~S_USR_MASK) !=
			 (info->flags & ~S_USR_MASK))) return -EPERM;
		info->flags = ((info->flags & ~S_USR_MASK) |
					   (new_serial.flags & S_USR_MASK));
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
	info->flags = ((info->flags & ~S_FLAGS) |
				   (new_serial.flags & S_FLAGS));
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
static int get_lsr_info(struct sc28l91_serial *info, unsigned int *value)
{
	unsigned char status;

	/* FIXME  -- This needs to be adapted to the philips UART */
#ifdef FIXME
	cli();
	status = info->uart->csr;
	status &= US_TXEMPTY;
	sti();
	put_user(status, value);
#endif	/* FIXME */
	return 0;
}

/*
 * This routine sends a break character out the serial port.
 */
static void send_break(struct sc28l91_serial *info, int duration)
{
	/* FIXME  -- This needs to be adapted to the philips UART */
#ifdef FIXME
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();
	info->uart->cr = US_STTBRK;
	if (!info->use_ints) {
		while (US_TXRDY != (info->uart->csr & US_TXRDY)) {
			;					// this takes max 2ms at 9600
		}
		info->uart->cr = US_STPBRK;
	}
	sti();
#endif	/* FIXME */
}

static int rs_ioctl(struct tty_struct *tty, struct file *file,
					unsigned int cmd, unsigned long arg)
{
	int error;
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	int retval;

	if (serial_paranoia_check(info, tty->device, "rs_ioctl"))
		return -ENODEV;

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
		(cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD) &&
		(cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
			return -EIO;
	}

	switch (cmd) {
	case TCSBRK:				/* SVID version: non-zero arg --> no break */
		retval = tty_check_change(tty);
		if (retval)
			return retval;
		tty_wait_until_sent(tty, 0);
		if (!arg)
			send_break(info, HZ / 4);	/* 1/4 second */
		return 0;
	case TCSBRKP:				/* support for POSIX tcsendbreak() */
		retval = tty_check_change(tty);
		if (retval)
			return retval;
		tty_wait_until_sent(tty, 0);
		send_break(info, arg ? arg * (HZ / 10) : HZ / 4);
		return 0;
	case TIOCGSOFTCAR:
		error = verify_area(VERIFY_WRITE, (void *) arg, sizeof(long));

		if (error)
			return error;
		put_fs_long(C_CLOCAL(tty) ? 1 : 0, (unsigned long *) arg);
		return 0;
	case TIOCSSOFTCAR:
		arg = get_fs_long((unsigned long *) arg);
		tty->termios->c_cflag = ((tty->termios->c_cflag & ~CLOCAL) | (arg ? CLOCAL : 0));
		return 0;
	case TIOCGSERIAL:
		error = verify_area(VERIFY_WRITE, (void *) arg, 
			sizeof(struct serial_struct));
		if (error)
			return error;
		return get_serial_info(info, (struct serial_struct *) arg);
	case TIOCSSERIAL:
		return set_serial_info(info, (struct serial_struct *) arg);
	case TIOCSERGETLSR:		/* Get line status register */
		error = verify_area(VERIFY_WRITE, (void *) arg, 
			sizeof(unsigned int));
		if (error)
			return error;
		else
			return get_lsr_info(info, (unsigned int *) arg);

	case TIOCSERGSTRUCT:
		error = verify_area(VERIFY_WRITE, (void *) arg, 
			sizeof(struct sc28l91_serial));
		if (error)
			return error;
		memcpy_tofs((struct sc28l91_serial *) arg, info, 
			sizeof(struct sc28l91_serial));
		return 0;

	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

static void rs_set_termios(struct tty_struct *tty, 
	struct termios *old_termios)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

	if (tty->termios->c_cflag == old_termios->c_cflag)
		return;

	change_speed(info);

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
 * S structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void rs_close(struct tty_struct *tty, struct file *filp)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;
	unsigned long flags;

	if (!info || serial_paranoia_check(info, tty->device, "rs_close"))
		return;

	save_flags(flags);
	cli();

	if (tty_hung_up_p(filp)) {
		restore_flags(flags);
		return;
	}
#ifdef SERIAL_DEBUG_OPEN
	printk("rs_close ttyS%d, count = %d\n", info->line, info->count);
#endif
	if ((tty->count == 1) && (info->count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  Info->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk("rs_close: bad serial port count; tty->count is 1, "
			   "info->count is %d\n", info->count);
		info->count = 1;
	}
	if (--info->count < 0) {
		printk("rs_close: bad serial port count for ttyS%d: %d\n",
			   info->line, info->count);
		info->count = 0;
	}
	if (info->count) {
		restore_flags(flags);
		return;
	}
	// closing port so disable interrupts
	tx_stop(info->uart);
	rx_stop(info->uart);
	set_ints_mode(0, info);

	info->flags |= S_CLOSING;
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if (info->flags & S_NORMAL_ACTIVE)
		info->normal_termios = *tty->termios;
	if (info->flags & S_CALLOUT_ACTIVE)
		info->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify 
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if (info->closing_wait != S_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, info->closing_wait);
	/*
	 * At this point we stop accepting input.  To do this, we
	 * disable the receive line status interrupts, and tell the
	 * interrupt driver to stop checking the data ready bit in the
	 * line status register.
	 */

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
			(tty->ldisc.close) (tty);
		tty->ldisc = ldiscs[N_TTY];
		tty->termios->c_line = N_TTY;
		if (tty->ldisc.open)
			(tty->ldisc.open) (tty);
	}
	if (info->blocked_open) {
		if (info->close_delay) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + info->close_delay;
			schedule();
		}
		wake_up_interruptible(&info->open_wait);
	}
	info->flags &= ~(S_NORMAL_ACTIVE | S_CALLOUT_ACTIVE | S_CLOSING);
	wake_up_interruptible(&info->close_wait);
	restore_flags(flags);
}

/*
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
static void rs_hangup(struct tty_struct *tty)
{
	struct sc28l91_serial *info = (struct sc28l91_serial *) tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_hangup"))
		return;

	rs_flush_buffer(tty);
	shutdown(info);
	info->event = 0;
	info->count = 0;
	info->flags &= ~(S_NORMAL_ACTIVE | S_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file *filp,
						   struct sc28l91_serial *info)
{
	unsigned long flags;
	struct wait_queue wait = { current, NULL };
	int retval;
	int do_clocal = 0;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (info->flags & S_CLOSING) {
		interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		if (info->flags & S_HUP_NOTIFY)
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
		if (info->flags & S_NORMAL_ACTIVE)
			return -EBUSY;
		if ((info->flags & S_CALLOUT_ACTIVE) &&
			(info->flags & S_SESSION_LOCKOUT) &&
			(info->session != current->session)) return -EBUSY;
		if ((info->flags & S_CALLOUT_ACTIVE) &&
			(info->flags & S_PGRP_LOCKOUT) &&
			(info->pgrp != current->pgrp)) return -EBUSY;
		info->flags |= S_CALLOUT_ACTIVE;
		return 0;
	}

	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) || (tty->flags & (1 << TTY_IO_ERROR))) {
		if (info->flags & S_CALLOUT_ACTIVE)
			return -EBUSY;
		info->flags |= S_NORMAL_ACTIVE;
		return 0;
	}

	if (info->flags & S_CALLOUT_ACTIVE) {
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
	printk("block_til_ready before block: ttyS%d, count = %d\n",
		   info->line, info->count);
#endif
	info->count--;
	info->blocked_open++;
	while (1) {
		save_flags(flags);
		cli();
		if (!(info->flags & S_CALLOUT_ACTIVE))
			sc28l91_rtsdtr(info, 1);
		restore_flags(flags);
		current->state = TASK_INTERRUPTIBLE;
		if (tty_hung_up_p(filp) || !(info->flags & S_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
			if (info->flags & S_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;
#else
			retval = -EAGAIN;
#endif
			break;
		}
		if (!(info->flags & S_CALLOUT_ACTIVE) &&
			!(info->flags & S_CLOSING) && do_clocal)
			break;
		if (current->signal & ~current->blocked) {
			retval = -ERESTARTSYS;
			break;
		}
#ifdef SERIAL_DEBUG_OPEN
		printk("block_til_ready blocking: ttyS%d, count = %d\n",
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
	printk("block_til_ready after blocking: ttyS%d, count = %d\n",
		   info->line, info->count);
#endif
	if (retval)
		return retval;
	info->flags |= S_NORMAL_ACTIVE;
	if (!info->use_ints) {
		serialpoll.data = (void *) info;
		queue_task(&serialpoll, &tq_timer);
	}
	return 0;
}

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its S structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
static int rs_open(struct tty_struct *tty, struct file *filp)
{
	struct sc28l91_serial *info;
	int retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;

	// check if line is sane
	if (line < 0 || line >= US_NB)
		return -ENODEV;

	info = &sc28l91_info[0];
#if 0
	/* Is the kgdb running over this line? */
	if (info->kgdb_channel)
		return -ENODEV;
#endif
	if (serial_paranoia_check(info, tty->device, "rs_open"))
		return -ENODEV;
#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open %s%d, count = %d\n", tty->driver.name, info->line,
		   info->count);
#endif
	info->count++;
	tty->driver_data = info;
	info->tty = tty;

	/*
	 * Start up serial port
	 */
	retval = startup(info);
	if (retval)
		return retval;

	retval = block_til_ready(tty, filp, info);
	if (retval) {
#ifdef SERIAL_DEBUG_OPEN
		printk("rs_open returning after block_til_ready with %d\n",
			   retval);
#endif
		return retval;
	}

	if ((info->count == 1) && (info->flags & S_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = info->normal_termios;
		else
			*tty->termios = info->callout_termios;
		change_speed(info);
	}

	info->session = current->session;
	info->pgrp = current->pgrp;

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open ttyS%d successful...\n", info->line);
#endif
	set_ints_mode(1, info);
	return 0;
}

extern void register_console(void (*proc) (const char *));

#if 0

static inline void rs_cons_check(struct sc28l91_serial *ss, int channel)
{
	int i, o, io;
	static consout_registered = 0;
	static msg_printed = 0;

	i = o = io = 0;

	/* Is this one of the serial console lines? */
	if ((sc28l91_cons_chanout != channel) && (sc28l91_cons_chanin != channel))
		return;
	sc28l91_conschan = ss->sc28l91_channel;
	sc28l91_consinfo = ss;

	/* Register the console output putchar, if necessary */
	if ((sc28l91_cons_chanout == channel)) {
		o = 1;
		/* double whee.. */
		if (!consout_registered) {
			register_console(sc28l91_console_print);
			consout_registered = 1;
		}
	}

	/* If this is console input, we handle the break received
	 * status interrupt on this line to mean prom_halt().
	 */
	if (sc28l91_cons_chanin == channel) {
		ss->break_abort = 1;
		i = 1;
	}
	if (o && i)
		io = 1;
	if (ss->baud != 9600)
		panic("Console baud rate weirdness");

	/* Set flag variable for this port so that it cannot be
	 * opened for other uses by accident.
	 */
	ss->is_cons = 1;

	if (io) {
		if (!msg_printed) {
			printk("zs%d: console I/O\n", ((channel >> 1) & 1));
			msg_printed = 1;
		}
	} else {
		printk("zs%d: console %s\n", ((channel >> 1) & 1),
			   (i == 1 ? "input" : (o == 1 ? "output" : "WEIRD")));
	}
}
#endif

static struct irqaction irq_usarta =
	{ rs_interrupta, 0, 0, "uart", NULL, NULL };
//static struct irqaction irq_usartb =
//	{ rs_interruptb, 0, 0, "usartb", NULL, NULL };

extern int setup_arm_irq(int, struct irqaction *);

static void interrupts_init(void)
{
	setup_arm_irq(IRQ_IRQ2, &irq_usarta);
}

static void show_serial_version(void)
{
	printk("Philips SC28L91 UART driver version 0.95\n");
}

/* rs_init inits the driver */
int rs_sc28l91_init(void)
{
	int flags, i;
	struct sc28l91_serial *info;

	/* Setup base handler, and timer table. */
	init_bh(SERIAL_BH, do_serial_bh);
	timer_table[RS_TIMER].fn = rs_timer;
	timer_table[RS_TIMER].expires = 0;

	show_serial_version();

	/* Initialize the tty_driver structure */

	memset(&serial_driver, 0, sizeof(struct tty_driver));

	serial_driver.magic = TTY_DRIVER_MAGIC;
	serial_driver.name = "ttyS";
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64;
	serial_driver.num = US_NB;
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

	serial_driver.open = rs_open;
	serial_driver.close = rs_close;
	serial_driver.write = rs_write;
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
	serial_driver.set_ldisc = rs_set_ldisc;

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

	save_flags(flags);
	cli();
	for (i = 0; i < US_NB; i++) {
		info = &sc28l91_info[i];
		info->magic = SERIAL_MAGIC;
		info->uart = uarts[i];
		uart_init(info);
		info->tty = 0;
		info->irqmask = AIC_IRQ2;
		info->irq = IRQ_IRQ2;
		
		info->port = (i) ? 1 : 2;
		info->line = i;
#ifdef CONFIG_CONSOLE_ON_SC28L91
		info->is_cons = 1;
#else
		info->is_cons = 0;
#endif	
		set_ints_mode(0, info);
		info->custom_divisor = 16;
		info->close_delay = 50;
		info->closing_wait = 3000;
		info->cts_state = 1;
		info->x_char = 0;
		info->event = 0;
		info->count = 0;
		info->blocked_open = 0;
		info->tqueue.routine = do_softint;
		info->tqueue.data = info;
		info->tqueue_hangup.routine = do_serial_hangup;
		info->tqueue_hangup.data = info;
		info->callout_termios = callout_driver.init_termios;
		info->normal_termios = serial_driver.init_termios;
		info->open_wait = 0;
		info->close_wait = 0;
		info->rx_buf = rx_buf1; //info->uart[RxFIFO];

		printk("%s%d at 0x%p (irq = %d)", serial_driver.name, info->line,
			   info->uart, info->irq);
		printk(" is a Philips SC28L9x UART\n");
	}
	interrupts_init();
	
#ifdef CONFIG_ARCH_ATMEL
	// disable interrupt
	*(volatile unsigned int *) AIC_IDCR = AIC_IRQ2;
	
	// set irq2 pin as peripheral
	*(volatile unsigned int *) PIO_DISABLE_REGISTER = (1 << 11);
	
	// clear interrupt
	*(volatile unsigned int *) AIC_ICCR = AIC_IRQ2;

	// enable interrupt
	*(volatile unsigned int *) AIC_IECR = AIC_IRQ2;
#endif

	
	restore_flags(flags);
	// hack to do polling
	serialpoll.routine = serpoll;
	serialpoll.data = 0;

	return 0;
}

#if 0
/*
 * register_serial and unregister_serial allows for serial ports to be
 * configured at run-time, to support PCMCIA modems.
 */
/* SPARC: Unused at this time, just here to make things link. */
static int register_serial(struct serial_struct *req)
{
	return -1;
}

static void unregister_serial(int line)
{
	return;
}

static void dbg_putc(int ch)
{
	static char tmp[2];
#define US_TPR  (0x38) /* Transmit Pointer Register */
#define US_TCR  (0x3C) /* Transmit Counter Register */

	tmp[0] = ch;

	outl_t((unsigned long) tmp, (CONFIG_SC28L91_UART_BASE + US_TPR) );
	outl_t(1, (CONFIG_SC28L91_UART_BASE + US_TCR) );

	while (inl_t((CONFIG_SC28L91_UART_BASE + US_TCR) )) {
	}
}

static void dbg_print(const char *str)
{
	const char *p;

	for (p = str; *p; p++) {
		if (*p == '\n') {
			dbg_putc('\r');
		}
		dbg_putc(*p);
	}
}

static void dbg_printk(const char *fmt, ...)
{
	char tmp[256];
	va_list args;

	va_start(args, fmt);
	vsprintf(tmp, fmt, args);
	va_end(args);
	dbg_print(tmp);
}

static void rs_sc28l91_print(const char *str)
{
	dbg_printk(str);
}

static void dump_a(unsigned long a, unsigned int s)
{
	unsigned long q;

	for (q = 0; q < s; q++) {
		if (q % 16 == 0) {
			dbg_printk("%08X: ", q + a);
		}
		if (q % 16 == 7) {
			dbg_printk("%02X-", *(unsigned char *) (q + a));
		} else {
			dbg_printk("%02X ", *(unsigned char *) (q + a));
		}
		if (q % 16 == 15) {
			dbg_printk(" :\n");
		}
	}
	if (q % 16) {
		dbg_printk(" :\n");
	}
}
#endif
