/* bf535_serial.c: Serial driver for BlackFin DSP internal USRTs.
 * Copyright(c) 2003	Metrowerks	<mwaddel@metrowerks.com>
 * Copyright(c)	2001	Tony Z. Kou	<tonyko@arcturusnetworks.com>
 * Copyright(c)	2001-2002 Arcturus Networks Inc. <www.arcturusnetworks.com>
 * 
 * Based on code from 68328 version serial driver imlpementation which was:
 * Copyright (C) 1995       David S. Miller    <davem@caip.rutgers.edu>
 * Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 * Copyright (C) 1998, 1999 D. Jeff Dionne     <jeff@uclinux.org>
 * Copyright (C) 1999       Vladimir Gurevich  <vgurevic@cisco.com> 
 */
 
#include <linux/module.h>
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
#include <linux/console.h>
#include <linux/reboot.h>
#include <linux/keyboard.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/delay.h>
#include <asm/uaccess.h>

#include "bf535_serial.h"
#define USE_INTS
#define SERIAL_PARANOIA_CHECK

#define SYNC_ALL	__asm__ __volatile__ ("ssync;\n")
#define ACCESS_LATCH(idx)	UART_LCR(idx) |= UART_LCR_DLAB;
#define ACCESS_PORT_IER(idx)	UART_LCR(idx) &= (~UART_LCR_DLAB);
#ifndef CONFIG_SERIAL_CONSOLE_PORT
#define	CONFIG_SERIAL_CONSOLE_PORT 1 /* default UART1 as serial console */
#endif

#if 0
struct tty_struct bf535_ttys;
struct bf535_serial *bf535_consinfo = 0;
#endif

/*
 *	Setup for console. Argument comes from the boot command line.
 */

#define	CONSOLE_BAUD_RATE	57600
#define	DEFAULT_CBAUD		B57600 /* valid only upon test */

static int bf535_console_initted = 0;
static int bf535_console_baud    = CONSOLE_BAUD_RATE;
static int bf535_console_cbaud   = DEFAULT_CBAUD;
/* static int bf535_console_port = -1;		*/

DECLARE_TASK_QUEUE(tq_serial);

#ifdef CONFIG_CONSOLE
extern wait_queue_head_t keypress_wait;
#endif

/*
 *	Driver data structures.
 */
struct tty_driver bf535_serial_driver, bf535_callout_driver;
static int bf535_serial_refcount;

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2
  
/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

#undef SERIAL_DEBUG_INTR
#undef SERIAL_DEBUG_OPEN
#undef SERIAL_DEBUG_FLOW

#define _INLINE_ inline

static struct bf535_serial bf535_soft[] = {
/*    magic    hub2     irq   flags           */
  {	0, 	0, 	IRQ_UART0, 	0 }, /* ttyS0 */
  {	0, 	1, 	IRQ_UART1, 	0 }, /* ttyS1 */
};

#define NR_PORTS (sizeof(bf535_soft) / sizeof(struct bf535_serial))

static struct tty_struct *bf535_serial_table[NR_PORTS];
static struct termios *bf535_serial_termios[NR_PORTS];
static struct termios *bf535_serial_termios_locked[NR_PORTS];

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif


/*
 * This is used to figure out the divisor speeds and the timeouts
 */

#ifndef CONFIG_PUB
static int baud_table[] = {
	0, 114, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 0 };

struct { unsigned short dl_high, dl_low;
	} hw_baud_table[] = {
	{0xff, 0xff}, /* approximately 0 */
	{0xff, 0xff}, /* 114 */
	{0x61, 0xa8}, /* 300 */
	{0x18, 0x6a}, /* 1200 */
	{0xc, 0x35},  /* 2400 */
	{0x6, 0x1a},  /* 4800 */
	{0x3, 0x0d},  /* 9600 */
	{0x1, 0x86},  /* 19200 */
	{0x0, 0xc3},  /* 38400 */
	{0x0, 0x82},  /* 57600 */
	{0x0, 0x41},  /* 115200 */
		      /* rate = SCLK / (16 * DL) - SCLK = 120MHz
   			 DL = (dl_high:dl_low) */
};	
#else
static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 0 };
struct { unsigned short dl_high, dl_low;
	} hw_baud_table[] = {
 	{0xff, 0xff}, /* approximately 0 */
	{0xff, 0xff}, /* 50 */
	{0xf4, 0x24}, /* 75 */
	{0xa6, 0x75}, /* 110 */
	{0x88, 0xa5}, /* 134 */
	{0x7a, 0x12}, /* 150 */
	{0x5b, 0x8d}, /* 200 */
	{0x3d, 0x09}, /* 300 */
	{0x1e, 0x84}, /* 600 */
	{0xf, 0x42}, /* 1200 */
	{0xa, 0x2c}, /* 1800 */
	{0x7, 0xa1}, /* 2400 */
	{0x3, 0xd0}, /* 4800 */
	{0x1, 0xe8}, /* 9600 */
	{0x0, 0xf4}, /* 19200 */
	{0x0, 0x7a}, /* 38400 */
	{0x0, 0x54}, /* 57600 */
	{0x0, 0x28}, /* 115200 */
};		     /* rate = SCLK / (16 * DL)   
	   		DL = (dl_high:dl_low) and above.
	   		Valid under case: SCLK = 75MHz */
#endif

#define BAUD_TABLE_SIZE (sizeof(baud_table)/sizeof(baud_table[0]))

/*
 * tmp_buf is used as a temporary buffer by serial_write.  We need to
 * lock it in case the memcpy_fromfs blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
static unsigned char tmp_buf[SERIAL_XMIT_SIZE]; /* This is cheating */
DECLARE_MUTEX(tmp_buf_sem);

/* Forward declarations.... */
static void bf535_change_speed(struct bf535_serial *info);
static void bf535_set_baud( void );

/****************************************************************************/
static inline int serial_paranoia_check(struct bf535_serial *info,
					dev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%d,%d) in %s\n";
	static const char *badinfo =
		"Warning: null bf535_serial for (%d, %d) in %s\n";

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
static inline void bf535_rtsdtr(struct bf535_serial *info, int set)
{
	unsigned long           flags = 0;
#ifdef SERIAL_DEBUG_OPEN
        printk("%s(%d): bf535_rtsdtr(info=%x,set=%d)\n",
                __FILE__, __LINE__, info, set);
#endif

        save_flags(flags); cli(); 
	if (set) {
		/* set the RTS/CTS line */
	} else {
		/* clear it */
	}
        restore_flags(flags);
	return;
}

/* Utility routines */
static inline int get_baud(struct bf535_serial *info)
{
        unsigned long result = 115200;
        unsigned short int baud_lo, baud_hi;
        int i, idx = info->hub2;
 
        ACCESS_LATCH(idx) /* change access to divisor latch */
        baud_lo = UART_DLL(idx);
        baud_hi = UART_DLH(idx);
 
        for (i=0; i<BAUD_TABLE_SIZE ; i++){
                if ( baud_lo == hw_baud_table[i].dl_low && baud_hi == hw_baud_table[i].dl_high)
                {    result = baud_table[i];
                     break;
                }
        }
 
        return result; 
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
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
	unsigned long flags = 0;
	unsigned int idx = (unsigned int) info->hub2;

	if (serial_paranoia_check(info, tty->device, "rs_stop"))
		return;
	
	save_flags(flags); cli();
	   ACCESS_PORT_IER(idx) /* Change access to IER & data port */
	   UART_IER(idx) = 0;
	restore_flags(flags);
}

extern void cache_delay();

static void local_put_char(int hub2, char ch)
{
	int flags = 0;

	if (hub2 < 0 || hub2 > 1) {
		printk("Error: Invalid serial port requested.\n");
		return; /* validate port op(UART 0|1) */
	}

	save_flags(flags); cli();

	while (!(UART_LSR(hub2) & UART_LSR_THRE)) {
		SYNC_ALL;
	}

	ACCESS_PORT_IER(hub2)
	UART_THR(hub2) = ch;
	SYNC_ALL;

	while (UART_LSR(hub2) & UART_LSR_THRE) {
		udelay(5);
	}

	restore_flags(flags);
}

static void rs_put_char(struct tty_struct *tty, char ch)
{
	rs_write(tty, 1, &ch, 1);
}

static void rs_start(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
	unsigned long flags = 0;
	unsigned int idx = (unsigned int) info->hub2;
	
	if (serial_paranoia_check(info, tty->device, "rs_start"))
		return;
	
	save_flags(flags); cli();
	ACCESS_PORT_IER(idx)	/* Change access to IER & data port */
	if (info->xmit_cnt && info->xmit_buf && !(UART_IER(idx) & UART_IER_ETBEI))
		UART_IER(idx) |= UART_IER_ETBEI;
	 
	restore_flags(flags);
}

/* Drop into either the boot monitor or kgdb upon receiving a break
 * from keyboard/console input.
 */
static void batten_down_hatches(void)
{
	/* Drop into the debugger */
}

static _INLINE_ void status_handle(struct bf535_serial *info, unsigned short status)
{
	/* If this is console input and this is a
	 * 'break asserted' status change interrupt
	 * see if we can drop into the debugger
	 */
	if((status & UART_LSR_BI) && info->break_abort)
		batten_down_hatches();
	return;
}

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct bf535_serial *info,
				    int event)
{
	info->event |= 1 << event;
	queue_task(&info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}

static _INLINE_ void receive_chars(struct bf535_serial *info, struct pt_regs *regs, unsigned short rx)
{
	struct tty_struct *tty = info->tty;
	unsigned char ch;
	int idx = info->hub2;

	/*
	 * This do { } while() loop will get ALL chars out of Rx FIFO 
         */
	do {
		ch = (unsigned char) rx;
	
		if(info->is_cons) {
			if (UART_LSR(idx) & UART_LSR_BI){ /* break received */ 
				status_handle(info, UART_LSR(idx));
				return;
			} else if (ch == 0x10) { /* ^P */
				show_state();
				show_free_areas();
				show_buffers();
/*				show_net_buffers(); */
				return;
			} else if (ch == 0x12) { /* ^R */
				machine_restart(NULL);
				return;
			}
			/* It is a 'keyboard interrupt' ;-) */
#ifdef CONFIG_CONSOLE
			wake_up(&keypress_wait);
#endif			
		}

		if(!tty){
			printk("no tty\n");
			goto clear_and_exit;
		}
		
		/*
		 * Make sure that we do not overflow the buffer
		 */
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			queue_task(&tty->flip.tqueue, &tq_timer);
			return;
		}

		if(UART_LSR(idx) & UART_LSR_PE) {
			*tty->flip.flag_buf_ptr++ = TTY_PARITY;
			status_handle(info, UART_LSR(idx));
		} else if(UART_LSR(idx) & UART_LSR_OE) {
			*tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
			status_handle(info, UART_LSR(idx));
		} else if(UART_LSR(idx) & UART_LSR_FE) {
			*tty->flip.flag_buf_ptr++ = TTY_FRAME;
			status_handle(info, UART_LSR(idx));
		} else {
			*tty->flip.flag_buf_ptr++ = 0; /* XXX */
		}

		tty->flip.count++;
                *tty->flip.char_buf_ptr++ = ch;

		ACCESS_PORT_IER(idx) /* change access to port data */
		rx = UART_RBR(idx);
	} while(UART_LSR(idx) & UART_LSR_DR);

	queue_task(&tty->flip.tqueue, &tq_timer);

clear_and_exit:
	return;
}

static _INLINE_ void transmit_chars(struct bf535_serial *info)
{
	int idx = info->hub2;

	if (info->x_char) { /* Send next char */
		local_put_char(idx, info->x_char);
		info->x_char = 0;
		goto clear_and_return;
	}

	if((info->xmit_cnt <= 0) || info->tty->stopped) { /* TX ints off */
		ACCESS_PORT_IER(idx) /* Change access to IER & data port */
		UART_IER(idx) &= ~UART_IER_ETBEI;
		goto clear_and_return;
	}

	/* Send char */
	local_put_char(idx,info->xmit_buf[info->xmit_tail++]);
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	info->xmit_cnt--;

	if (info->xmit_cnt < WAKEUP_CHARS)
		rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);

	if(info->xmit_cnt <= 0) { /* All done for now... TX ints off */
		ACCESS_PORT_IER(idx) /* Change access to IER & data port */
		UART_IER(idx) &= ~UART_IER_ETBEI;
		goto clear_and_return;
	}

clear_and_return:
	/* Clear interrupt (should be auto)*/
	return;
}

#define UART_INVOKED	3
/*
 * This is the serial driver's generic interrupt routine
 * Note: Generally it should be attached to general interrupt 10, responsile 
 *       for UART0&1 RCV and XMT interrupt, to make sure the invoked interrupt
 *       source, look into bit 10-13 of SIC_ISR(peripheral interrupt status reg.
 *       Finally, to see what can be done about request_irq(......)
 */
void rs_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct bf535_serial *info; // = &bf535_soft[CONFIG_SERIAL_CONSOLE_PORT];
	unsigned short iir; /* Interrupt Identification Register */
	unsigned short rx, idx;
	unsigned int sic_status = SIC_ISR;

	for (idx = 0; idx < NR_PORTS; idx++){
	   if (sic_status & (UART_INVOKED << 2*(idx + 5))){ 
	   	/* test bit 10-11 and 12-13 */
		iir = UART_IIR(idx);
		info = &bf535_soft[idx];

		if (!(iir & UART_IIR_NOINT)){
		   switch (iir & UART_IIR_STATUS){
		   case UART_IIR_LSR:
				printk("Line status changed for serial port %d.\n", idx);
				break;
		   case UART_IIR_RBR:
		   		/* Change access to IER & data port */
				ACCESS_PORT_IER(idx) 
				if (UART_LSR(idx) & UART_LSR_DR){
				   rx = UART_RBR(idx);
				   receive_chars(info, regs, rx);
				}
				break;
		   case UART_IIR_THR:
		   		/* Change access to IER & data port */
				ACCESS_PORT_IER(idx) 
				if (UART_LSR(idx) & UART_LSR_THRE){
				    transmit_chars(info);
//				    do{
//					transmit_chars(info);
//				    }while(info->xmit_cnt > 0);
				}
				break;
		   case UART_IIR_MSR:
				printk("Modem status changed for serial port.\n");
			
	   	   }
		}
	   }
	}	
	return;
}

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
	struct bf535_serial	*info = (struct bf535_serial *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	if (test_and_clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) {
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
	struct bf535_serial	*info = (struct bf535_serial *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	tty_hangup(tty);
}


static int startup(struct bf535_serial * info)
{
	unsigned long flags = 0;
	int idx	= info->hub2;
	
	if (info->flags & S_INITIALIZED)
		return 0;

	if (!info->xmit_buf) {
		info->xmit_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!info->xmit_buf)
			return -ENOMEM;
	}

	local_irq_save(flags);

	/*
	 * Clear the FIFO buffers and disable them
	 * (they will be reenabled in bf535_change_speed())
	 */

	info->xmit_fifo_size = 1;
	ACCESS_PORT_IER(idx) /* Change access to IER & data port */
	UART_IER(idx) = UART_IER_ERBFI | UART_IER_ETBEI | UART_IER_ELSI | UART_IER_EDDSI;
	(void)UART_RBR(idx);

	/*
	 * Finally, enable sequencing and interrupts
	 */
	UART_IER(idx) = UART_IER_ERBFI | UART_IER_ELSI | UART_IER_EDDSI;
	bf535_rtsdtr(info, 1);

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * and set the speed of the serial port
	 */

	bf535_change_speed(info);

	info->flags |= S_INITIALIZED;
	local_irq_restore(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct bf535_serial * info)
{
	unsigned long	flags = 0;
	int idx = info->hub2;

        if (!(info->flags & S_INITIALIZED))
                return; 

#ifdef SERIAL_DEBUG_OPEN
        printk("Shutting down serial port %d (irq %d)....\n", info->hub2,
               info->irq);
#endif
      
	save_flags(flags); cli(); /* Disable interrupts */

	UART_LCR(idx) = 0;
	ACCESS_PORT_IER(idx) /* Change access to IER & data port */
	UART_IER(idx) = 0;
	
        if (!info->tty || (info->tty->termios->c_cflag & HUPCL))
                bf535_rtsdtr(info, 0);
	
	if (info->xmit_buf) {
		free_page((unsigned long) info->xmit_buf);
		info->xmit_buf = 0;
	}

	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);

	info->flags &= ~S_INITIALIZED;
	restore_flags(flags);
}

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void bf535_change_speed(struct bf535_serial *info)
{
	unsigned short uart_lcr;
	unsigned cflag, flags = 0;
	int	i, idx = info->hub2;

	if (!info->tty || !info->tty->termios)
		return;
	cflag = info->tty->termios->c_cflag;

	local_irq_save(flags);
	ACCESS_PORT_IER(idx) /* Change access to IER & data port */
//	UART_IER(idx) = 0;  /* disable all interrupts for UART0 */

	i = cflag & CBAUD;
        if (i & CBAUDEX) {
                i = (i & ~CBAUDEX) + B38400;
        }

	info->baud = baud_table[i];
	ACCESS_LATCH(idx)	/* change to access of divisor latch */
	UART_DLL(idx) = hw_baud_table[i].dl_low;
	UART_DLH(idx) = hw_baud_table[i].dl_high;

	UART_LCR(idx) &= UART_LCR_DLAB; /* clear all except DLAB bit */
	uart_lcr = UART_LCR(idx);

	switch (cflag & CSIZE) {
	case CS6:	uart_lcr |= UART_LCR_WLS6; break;
	case CS7:	uart_lcr |= UART_LCR_WLS7; break;
	case CS8:	uart_lcr |= UART_LCR_WLS8; break;
	default: 	break;
	}
		
	if (cflag & CSTOPB)
		uart_lcr |= UART_LCR_STB;

	if (cflag & PARENB){
		uart_lcr |= UART_LCR_PEN;
		if (cflag & PARODD)
		uart_lcr &= ~(UART_LCR_EPS | UART_LCR_SP);
	}

        if (cflag & CRTSCTS) {
		uart_lcr |= UART_LCR_SB;
        }
		UART_LCR(idx) = uart_lcr;

        restore_flags(flags);	
	return;
}

static void rs_set_ldisc(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_set_ldisc"))
		return;

	info->is_cons = (tty->termios->c_line == N_TTY);
	
	printk("ttyS%d console mode %s\n", info->line, info->is_cons ? "on" : "off");
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
	unsigned long flags = 0;
	int idx = info->hub2;

	if (serial_paranoia_check(info, tty->device, "rs_flush_chars"))
		return;
#ifndef USE_INTS
	for(;;) {
#endif
		if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped || 
	   	   !info->xmit_buf)
			return;

		save_flags(flags); cli();

		ACCESS_PORT_IER(idx) /* Change access to IER & data port */
#ifdef USE_INTS
		UART_IER(idx) |= UART_IER_ETBEI;
		if (UART_LSR(idx) & UART_LSR_TEMT) {
#else
		UART_IER(idx) = 0; /* disable all interrupts for UART0  */
		if (1) {
#endif
			/* Send char */
			local_put_char(idx,info->xmit_buf[info->xmit_tail++]);
			info->xmit_tail = info->xmit_tail&(SERIAL_XMIT_SIZE-1);
			info->xmit_cnt--;
		}

#ifndef USE_INTS
		while (!(UART_LSR(idx) & UART_LSR_TEMT)) 
			SYNC_ALL;
	}
#endif
	restore_flags(flags);
}

static int rs_write(struct tty_struct * tty, int from_user,
		    const unsigned char *buf, int count)
{
	int	c, total = 0;
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
	unsigned long flags = 0;
	int	idx = info->hub2;

	if (serial_paranoia_check(info, tty->device, "rs_write"))
		return 0;

	if (!tty || !info->xmit_buf)
		return 0;

	while (1) {
		c = MIN(count, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
				   SERIAL_XMIT_SIZE - info->xmit_head));
		if (c <= 0)
			break;

		save_flags(flags); cli();		
		if (from_user) {
			down(&tmp_buf_sem);
			copy_from_user(tmp_buf, buf, c);
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

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped) {
		/* Enable transmitter */
		save_flags(flags); cli();		
#ifdef USE_INTS
		ACCESS_PORT_IER(idx) /* Change access to IER & data port */
		UART_IER(idx) |= UART_IER_ETBEI;
#else
		while(info->xmit_cnt) {
#endif
		while (!(UART_LSR(idx) & UART_LSR_TEMT))
			SYNC_ALL;

		if (UART_LSR(idx) & UART_LSR_TEMT) {
			local_put_char(idx,info->xmit_buf[info->xmit_tail++]);
			info->xmit_tail = info->xmit_tail&(SERIAL_XMIT_SIZE-1);
			info->xmit_cnt--;
		}

#ifndef USE_INTS
		}
#endif
		restore_flags(flags);
	}
	return total;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
	int	ret;
				
	if (serial_paranoia_check(info, tty->device, "rs_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int rs_chars_in_buffer(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rs_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rs_flush_buffer"))
		return;
	cli();
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
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
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_throttle"))
		return;
	
	if (I_IXOFF(tty))
		info->x_char = STOP_CHAR(tty);

	/* Turn off RTS line (do this atomic) */
}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;

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

static int get_serial_info(struct bf535_serial * info,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
  
	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.hub2 = info->hub2;
	tmp.irq = info->irq;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	tmp.custom_divisor = info->custom_divisor;
	copy_to_user(retinfo,&tmp,sizeof(*retinfo));
	return 0;
}

static int set_serial_info(struct bf535_serial * info,
			   struct serial_struct * new_info)
{
	struct serial_struct new_serial;
	struct bf535_serial old_info;
	int    retval = 0;

	if (!new_info)
		return -EFAULT;
	copy_from_user(&new_serial,new_info,sizeof(new_serial));
	old_info = *info;

	if (!suser()) {
		if ((new_serial.baud_base != info->baud_base) ||
		    (new_serial.type != info->type) ||
		    (new_serial.close_delay != info->close_delay) ||
		    ((new_serial.flags & ~S_USR_MASK) !=
		     (info->flags & ~S_USR_MASK)))
			return -EPERM;
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
static int get_lsr_info(struct bf535_serial * info, unsigned int *value)
{
	unsigned char status;

	cli();

	status = 0; /* no CTS status pin in BlackFin Serial */
	sti();
	put_user(status,value);
	return 0;
}

/*
 * This routine sends a break character out the serial port.
 */
static void send_break(	struct bf535_serial * info, int duration)
{
        unsigned long flags = 0;
	int	 idx = info->hub2;

        current->state = TASK_INTERRUPTIBLE;
        save_flags(flags);
        cli();
	UART_LCR(idx) |= UART_LCR_SB;
        schedule_timeout(duration);
	UART_LCR(idx) &= ~UART_LCR_SB;
        restore_flags(flags);
}

static int rs_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct bf535_serial * info = (struct bf535_serial *)tty->driver_data;
	int retval;

	if (serial_paranoia_check(info, tty->device, "rs_ioctl"))
		return -ENODEV;

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
	    (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
		case TCSBRK:	/* SVID version: non-zero arg --> no break */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (!arg)
				send_break(info, HZ/4);	/* 1/4 second */
			return 0;
		case TCSBRKP:	/* support for POSIX tcsendbreak() */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			send_break(info, arg ? arg*(HZ/10) : HZ/4);
			return 0;
		case TIOCGSOFTCAR:
			error = verify_area(VERIFY_WRITE, (void *) arg,sizeof(long));
			if (error)
				return error;
			put_user(C_CLOCAL(tty) ? 1 : 0,
				    (unsigned long *) arg);
			return 0;
		case TIOCSSOFTCAR:
			get_user(arg, (unsigned long *) arg);
			tty->termios->c_cflag =
				((tty->termios->c_cflag & ~CLOCAL) |
				 (arg ? CLOCAL : 0));
			return 0;
		case TIOCGSERIAL:
			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct serial_struct));
			if (error)
				return error;
			return get_serial_info(info,
					       (struct serial_struct *) arg);
		case TIOCSSERIAL:
			return set_serial_info(info,
					       (struct serial_struct *) arg);
		case TIOCSERGETLSR: /* Get line status register */
			error = verify_area(VERIFY_WRITE, (void *) arg,
				sizeof(unsigned int));
			if (error)
				return error;
			else
			    return get_lsr_info(info, (unsigned int *) arg);

		case TIOCSERGSTRUCT:
			error = verify_area(VERIFY_WRITE, (void *) arg,
						sizeof(struct bf535_serial));
			if (error)
				return error;
			copy_to_user((struct bf535_serial *) arg,
				    info, sizeof(struct bf535_serial));
			return 0;
			
		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

static void rs_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct bf535_serial *info = (struct bf535_serial *)tty->driver_data;

	if (tty->termios->c_cflag == old_termios->c_cflag)
		return;

	bf535_change_speed(info);

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
static void rs_close(struct tty_struct *tty, struct file * filp)
{
	struct bf535_serial * info = (struct bf535_serial *)tty->driver_data;
	unsigned long flags = 0;
	int	 idx = info->hub2;

	if (!info || serial_paranoia_check(info, tty->device, "rs_close"))
		return;
	
	save_flags(flags); cli();
	
	if (tty_hung_up_p(filp)) {
		restore_flags(flags);
		return;
	}
	
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

	ACCESS_PORT_IER(idx) /* Change access to IER & data port */
	UART_IER(idx) = 0;

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
			schedule_timeout(info->close_delay);
		}
		wake_up_interruptible(&info->open_wait);
	}
	info->flags &= ~(S_NORMAL_ACTIVE|S_CALLOUT_ACTIVE|
			 S_CLOSING);
	wake_up_interruptible(&info->close_wait);
	restore_flags(flags);
}

/*
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void rs_hangup(struct tty_struct *tty)
{
	struct bf535_serial * info = (struct bf535_serial *)tty->driver_data;
	
	if (serial_paranoia_check(info, tty->device, "rs_hangup"))
		return;
	
	rs_flush_buffer(tty);
	shutdown(info);
	info->event = 0;
	info->count = 0;
	info->flags &= ~(S_NORMAL_ACTIVE|S_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file * filp,
			   struct bf535_serial *info)
{
	DECLARE_WAITQUEUE(wait, current);
	int		retval;
	int		do_clocal = 0;

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
		    (info->session != current->session))
		    return -EBUSY;
		if ((info->flags & S_CALLOUT_ACTIVE) &&
		    (info->flags & S_PGRP_LOCKOUT) &&
		    (info->pgrp != current->pgrp))
		    return -EBUSY;
		info->flags |= S_CALLOUT_ACTIVE;
		return 0;
	}
	
	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
	    (tty->flags & (1 << TTY_IO_ERROR))) {
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

	info->count--;
	info->blocked_open++;
	while (1) {
		cli();
		if (!(info->flags & S_CALLOUT_ACTIVE))
			bf535_rtsdtr(info, 1);
		sti();
		current->state = TASK_INTERRUPTIBLE;
		if (tty_hung_up_p(filp) ||
		    !(info->flags & S_INITIALIZED)) {
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
                if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			break;
		}
		schedule();
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&info->open_wait, &wait);
	if (!tty_hung_up_p(filp))
		info->count++;
	info->blocked_open--;

	if (retval)
		return retval;
	info->flags |= S_NORMAL_ACTIVE;
	return 0;
}	

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its S structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
int rs_open(struct tty_struct *tty, struct file * filp)
{
	struct bf535_serial	*info;
	int 			retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;
	
	if ((line < 0) || (line >= NR_PORTS)) 
		return -ENODEV;

	info = bf535_soft + line;

	if (serial_paranoia_check(info, tty->device, "rs_open"))
		return -ENODEV;
#ifdef SERIAL_DEBUG_OPEN
        printk("bf535_open %s%d, count = %d\n", tty->driver.name, info->line,
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
                printk("bf535_open returning after block_til_ready with %d\n",
                       retval);
#endif 
		return retval;
	}

	if ((info->count == 1) && (info->flags & S_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = info->normal_termios;
		else 
			*tty->termios = info->callout_termios;
		bf535_change_speed(info);
	}

	info->session = current->session;
	info->pgrp = current->pgrp;

	return 0;
}

/* Finally, routines used to initialize the serial driver. */

static void show_serial_version(void)
{
	printk("BlackFin serial driver version 1.00\n");
}

/* rsbf535_init inits the driver *
static */ int __init rsbf535_init(void)
{
	int i, flags = 0;
	struct bf535_serial *info;
	
	/* Setup base handler, and timer table. */
	init_bh(SERIAL_BH, do_serial_bh);
	show_serial_version();

	/* Initialize the tty_driver structure */
	/* NOTE: Not all of this is exactly right for us. */
	
	memset(&bf535_serial_driver, 0, sizeof(struct tty_driver));
	bf535_serial_driver.magic = TTY_DRIVER_MAGIC;
	bf535_serial_driver.name = "ttyS";
	bf535_serial_driver.major = TTY_MAJOR;
	bf535_serial_driver.minor_start = 64; 
	bf535_serial_driver.num = NR_PORTS;
	bf535_serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	bf535_serial_driver.subtype = SERIAL_TYPE_NORMAL;
	bf535_serial_driver.init_termios = tty_std_termios;
	bf535_serial_driver.init_termios.c_cflag = 
			bf535_console_cbaud | CS8 | CREAD | HUPCL | CLOCAL;
	bf535_serial_driver.flags = TTY_DRIVER_REAL_RAW;
	bf535_serial_driver.refcount = &bf535_serial_refcount;
	bf535_serial_driver.table = bf535_serial_table;
	bf535_serial_driver.termios = bf535_serial_termios;
	bf535_serial_driver.termios_locked = bf535_serial_termios_locked;

	bf535_serial_driver.open = rs_open;
	bf535_serial_driver.close = rs_close;
	bf535_serial_driver.write = rs_write;
	bf535_serial_driver.flush_chars = rs_flush_chars;
	bf535_serial_driver.put_char = rs_put_char;
	bf535_serial_driver.write_room = rs_write_room;
	bf535_serial_driver.chars_in_buffer = rs_chars_in_buffer;
	bf535_serial_driver.flush_buffer = rs_flush_buffer;
	bf535_serial_driver.ioctl = rs_ioctl;
	bf535_serial_driver.throttle = rs_throttle;
	bf535_serial_driver.unthrottle = rs_unthrottle;
/*	bf535_serial_driver.send_xchar = ????	*/
	bf535_serial_driver.set_termios = rs_set_termios;
	bf535_serial_driver.stop = rs_stop;
	bf535_serial_driver.start = rs_start;
	bf535_serial_driver.hangup = rs_hangup;
	bf535_serial_driver.set_ldisc = rs_set_ldisc;
/*	bf535_serial_driver.wait_until_sent = ????	*/
/*	bf535_serial_driver.read_proc = ????	*/

	/*
	 * The callout device is just like normal device except for
	 * major number and the subtype code. ??? --tonyko
	 */
	bf535_callout_driver = bf535_serial_driver;
	bf535_callout_driver.name = "cua";
	bf535_callout_driver.major = TTYAUX_MAJOR;
	bf535_callout_driver.subtype = SERIAL_TYPE_CALLOUT;

	if (tty_register_driver(&bf535_serial_driver))
		panic("Couldn't register serial driver\n");
	if (tty_register_driver(&bf535_callout_driver))
		panic("Couldn't register callout driver\n");
	
	save_flags(flags); cli();

        /*
         *      Configure all the attached serial ports.
         */
	for (i = 0, info = bf535_soft; (i < NR_PORTS); i++, info++){
		info->magic = SERIAL_MAGIC;
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
		info->callout_termios =bf535_callout_driver.init_termios;
		info->normal_termios = bf535_serial_driver.init_termios;
		init_waitqueue_head(&info->open_wait);
		init_waitqueue_head(&info->close_wait);
		info->line = i;
		info->is_cons = 1; /* Means shortcuts work */
	
		switch(i){
			case 0:
				info->irq = IRQ_UART0;
				break;
			case 1:
				info->irq = IRQ_UART1;
				break;
		}
		
		printk("%s%d at irq = %d", \
			bf535_serial_driver.name, i, info->irq);
		printk(" is a builtin BlackFin UART\n");
	}
	restore_flags(flags);

	if (request_irq(IRQ_UART0, rs_interrupt, IRQ_FLG_STD, 
                        "BlkFin_UART0", NULL))
	    panic("Unable to attach BlackFin UART0 interrupt\n");
	if (request_irq(IRQ_UART1, rs_interrupt, IRQ_FLG_STD, 
                        "BlkFin_UART1", NULL))
	    panic("Unable to attach BlackFin UART1 interrupt\n");
	printk("Enabling Serial UART0/1 Interrupts\n");
	enable_irq(IRQ_UART0);
	enable_irq(IRQ_UART1);

	return 0;
}

/*
 * register_serial and unregister_serial allows for serial ports to be
 * configured at run-time, to support PCMCIA modems.
 */
/* NOTE : Unused at this time, just here to make things link. */
/*int register_serial(struct serial_struct *req)
{
	return -1;
}

void unregister_serial(int line)
{
	return;
}*/
	
module_init(rsbf535_init);
/* DAVIDM module_exit(rsbf535_fini); */


/* setting console baud rate: CONFIG_SERIAL_CONSOLE_PORT */
static void bf535_set_baud( void )
{
	int	i;

	/* Change access to IER & data port */
	ACCESS_PORT_IER(CONFIG_SERIAL_CONSOLE_PORT) 
	UART_IER(CONFIG_SERIAL_CONSOLE_PORT) &= ~UART_IER_ETBEI;

again:
	for (i = 0; i < sizeof(baud_table) / sizeof(baud_table[0]); i++)
		if (baud_table[i] == bf535_console_baud)
			break;
	if (i >= sizeof(baud_table) / sizeof(baud_table[0])) {
		bf535_console_baud = 9600;
		goto again;
	}

	ACCESS_LATCH(CONFIG_SERIAL_CONSOLE_PORT) /*Set to access divisor latch*/
	UART_DLL(CONFIG_SERIAL_CONSOLE_PORT) = hw_baud_table[i].dl_low;
	UART_DLH(CONFIG_SERIAL_CONSOLE_PORT) = hw_baud_table[i].dl_high;

	UART_LCR(CONFIG_SERIAL_CONSOLE_PORT) |= UART_LCR_WLS8;
	UART_LCR(CONFIG_SERIAL_CONSOLE_PORT) &= ~UART_LCR_PEN;

	/* Change access to IER & data port */
	ACCESS_PORT_IER(CONFIG_SERIAL_CONSOLE_PORT) 
        UART_IER(CONFIG_SERIAL_CONSOLE_PORT) |=(UART_IER_ETBEI | UART_IER_ELSI);
	bf535_console_initted = 1;
	return;
}


int bf535_console_setup(struct console *cp, char *arg)
{
	int	i, n = CONSOLE_BAUD_RATE;

	if (!cp)
		return(-1);

	if (arg)
		n = simple_strtoul(arg,NULL,0);

	for (i = 0; i < BAUD_TABLE_SIZE; i++)
		if (baud_table[i] == n)
			break;
	if (i < BAUD_TABLE_SIZE) {
		bf535_console_baud = n;
		bf535_console_cbaud = 0;
		if (i > 15) {
			bf535_console_cbaud |= CBAUDEX;
			i -= 15;
		}
		bf535_console_cbaud |= i;
	}

	bf535_set_baud(); /* make sure baud rate changes */
	return(0);
}

static kdev_t bf535_console_device(struct console *c)
{
	return MKDEV(TTY_MAJOR, 64 + c->index);
}

void bf535_console_write (struct console *co, const char *str,
			   unsigned int count)
{
    if (!bf535_console_initted)
	bf535_set_baud();
    while (count--) {
        if (*str == '\n')	/* if a LF, also do CR... */
           local_put_char( CONFIG_SERIAL_CONSOLE_PORT, '\r');
        local_put_char( CONFIG_SERIAL_CONSOLE_PORT, *str++ );
    }
}

static struct console bf535_driver = {
	name:		"ttyS",
	write:		bf535_console_write,
	read:		NULL,
	device:		bf535_console_device,
	unblank:	NULL,
	setup:		bf535_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		CONFIG_SERIAL_CONSOLE_PORT,
	cflag:		0,
	next:		NULL
};

void bf535_console_init(void)
{
	register_console(&bf535_driver);
}
