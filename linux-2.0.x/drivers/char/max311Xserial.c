/* max311Xserial.c: Serial port driver for MAX311X UART
 *
 * Copyright (C) 1995       David S. Miller    <davem@caip.rutgers.edu>
 * Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 * Copyright (C) 1998, 1999 D. Jeff Dionne     <jeff@rt-control.com>
 * Copyright (C) 1999       Vladimir Gurevich  <vgurevic@cisco.com>
 * Copyright (C) 1999       David Beckemeyer   <david@bdt.com>
 *
 */
 
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

#include "max311Xserial.h"

/* Warning: This doesn't work yet!
 * There is some race with the TX interrupt behavior of the
 * MAX3111 that I haven't been able to correct.  Polling TX
 * works okay for my needs (little or no output anyway).
 * Define to use interrupts (after you debug it!)
 */
#undef USE_INTS

#define NR_PORTS 2

static struct max311x_serial max311x_soft_table[NR_PORTS];

static struct tty_driver max_serial_driver;
static int serial_refcount;

/* serial subtype definitions */
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2
  
/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

/* Debugging... DEBUG_INTR is bad to use when one of the zs
 * lines is your console ;(
 */
#undef MAX311X_DEBUG_INTR
#undef MAX311X_DEBUG_OPEN
#undef MAX311X_DEBUG_FLOW
#undef MAX311X_DEBUG_XMIT
#undef MAX311X_DEBUG_CONF
#undef MAX311X_DEBUG_TXRX
#undef MAX311X_DEBUG_CNTR
#undef MAX311X_DEBUG_IOCT
#define MAX311X_DEBUG_STARTUP
#define RSMAX_ISR_PASS_LIMIT 256

#define _INLINE_ inline

static void change_speed(struct max311x_serial *info);

static struct tty_struct *serial_table[NR_PORTS];
static struct termios *serial_termios[NR_PORTS];
static struct termios *serial_termios_locked[NR_PORTS];

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
static unsigned char tmp_buf[SERIAL_XMIT_SIZE]; /* This is cheating */
static struct semaphore tmp_buf_sem = MUTEX;

static inline int serial_paranoia_check(struct max311x_serial *info,
					dev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%d, %d) in %s\n";
	static const char *badinfo =
		"Warning: null max311x_serial for (%d, %d) in %s\n";

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

/*
 * This is used to figure out the divisor speeds and the timeouts
 */
static max_baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 0 };

static div_table[] = {
	/* divisor index table for 1.8432Mhz crystal */
        -1,     // 0
        -1,     // 50
        -1,     // 75
        -1,     // 110
        -1,     // 134
        -1,     // 150
        -1,     // 200
        15,     // 300
        14,     // 600
        13,     // 1200
        6,      // 1800
        12,     // 2400
        11,     // 4800
        10,     // 9600
        9,      // 19200
        8,      // 38400
        1,      // 57600
        0,      // 115200
        -1     // 0
};

/* Sets or clears DTR/RTS on the requested line */
static inline void max311x_rtsdtr(struct max311x_serial *ss, int set)
{
	if (set) {
		/* set the RTS/CTS line */
	} else {
		/* clear it */
	}
	return;
}

/* 
 * The main SPI master cycle
 *
 * Must be called with Interrupts disabled!
 */
static unsigned short spi_cycle(int cs_bit, unsigned short wrdata)
{
        SPIMCONT |= SPIMCONT_ENABLE;            // Enable SPI
        SPIMDATA = wrdata;                      // load data to be xmitted
        PDDATA &= ~cs_bit;                      // select MAX3111
        SPIMCONT |= SPIMCONT_XCH;               // trigger exchage
        while (!(SPIMCONT & SPIMCONT_IRQ));     // Poll IRQ
        SPIMCONT &= ~SPIMCONT_IRQ;              // Clear IRQ bit
        PDDATA |= cs_bit;                       // deselect MAX3111
        return(SPIMDATA);
}

/*
 * ------------------------------------------------------------
 * rsmax_stop() and rsmax_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 * ------------------------------------------------------------
 */
static void rsmax_stop(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rsmax_stop"))
		return;
	
	save_flags(flags); cli();
	info->config &= ~MAX3111_WC_TM;
	spi_cycle(info->cs_line, info->config);
	restore_flags(flags);
#ifdef MAX311X_DEBUG_CONF
	printk("rsmax_stop config %x\n", info->config);
#endif
}


static void rsmax_start(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
	unsigned long flags;
	
	if (serial_paranoia_check(info, tty->device, "rsmax_start"))
		return;
	
#ifdef USE_INTS
	save_flags(flags); cli();
	if (info->xmit_cnt && info->xmit_buf &&
            !(spi_cycle(info->cs_line, 0x4000) & MAX3111_WC_T)) {
		info->config |= MAX3111_WC_TM;
		spi_cycle(info->cs_line, info->config);
#ifdef MAX311X_DEBUG_CONF
		printk("rsmax_start config %x\n", info->config);
#endif
	}
	restore_flags(flags);
#endif
}


/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rsmax_sched_event(struct max311x_serial *info,
				    int event)
{
	info->event |= 1 << event;
	queue_task_irq_off(&info->tqueue, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
}

static _INLINE_ void receive_chars(struct max311x_serial *info, unsigned short rx)
{
	struct tty_struct *tty = info->tty;
	unsigned char ch;

	/*
	 * This while() loop will get ALL chars out of Rx FIFO 
         */
	while (rx & MAX3111_RD_R) {

		ch = (rx & 0x00ff);
	
		if(!tty)
			goto clear_and_exit;
		
		/*
		 * Make sure that we do not overflow the buffer
		 */
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
			return;
		}

		if(rx & MAX3111_RD_FE) {
			*tty->flip.flag_buf_ptr++ = TTY_FRAME;
		} else {
			*tty->flip.flag_buf_ptr++ = 0; /* XXX */
		}
                *tty->flip.char_buf_ptr++ = ch;
		tty->flip.count++;

		rx = spi_cycle(info->cs_line, 0);
	}

	queue_task_irq_off(&tty->flip.tqueue, &tq_timer);

clear_and_exit:
	return;
}

static _INLINE_ void transmit_chars(struct max311x_serial *info)
{
	unsigned short rx;

	if (info->x_char) {
		/* Send next char */
		rx = spi_cycle(info->cs_line, 0x8000 | info->x_char);
		if (rx & MAX3111_RD_R) {
#ifdef MAX311X_DEBUG_TXRX
			printk("transmit_chars tx R %x\n", rx);
#endif
			receive_chars(info, rx);
		}
		info->x_char = 0;
		goto clear_and_return;
	}

	if((info->xmit_cnt <= 0) || info->tty->stopped) {
		/* That's peculiar... TX ints off */
		info->config &= ~MAX3111_WC_TM;
		spi_cycle(info->cs_line, info->config);
#ifdef MAX311X_DEBUG_CONF
		printk("transmit_chars config %x\n", info->config);
#endif
		goto clear_and_return;
	}

	/* Send char */
#ifdef MAX311X_DEBUG_XMIT
	printk("MAX tx_chars %x\n", info->xmit_buf[info->xmit_tail]);
#endif
	rx = spi_cycle(info->cs_line, 0x8000 | info->xmit_buf[info->xmit_tail++]);
	if (rx & MAX3111_RD_R) {
#ifdef MAX311X_DEBUG_TXRX
		printk("transmit_chars tx R %x\n", rx);
#endif
		receive_chars(info, rx);
	}
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	info->xmit_cnt--;

	if (info->xmit_cnt < WAKEUP_CHARS)
		rsmax_sched_event(info, RSMAX_EVENT_WRITE_WAKEUP);

	if(info->xmit_cnt <= 0) {
		/* All done for now... TX ints off */
		info->config &= ~MAX3111_WC_TM;
		spi_cycle(info->cs_line, info->config);
#ifdef MAX311X_DEBUG_CONF
		printk("transmit_chars config %x\n", info->config);
#endif
		goto clear_and_return;
	}

clear_and_return:
	/* Clear interrupt (should be auto)*/
	return;
}

/*
 * This is the serial driver's generic interrupt routine
 */
void rsmax0_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct max311x_serial * info = &max311x_soft_table[0];
	unsigned short rx;

	rx = spi_cycle(info->cs_line, 0);
#ifdef USE_INTS
	if (rx & MAX3111_RD_R) receive_chars(info, rx);
	if (rx & MAX3111_RD_T)   transmit_chars(info);
#else
	if (rx & MAX3111_RD_R) receive_chars(info, rx);
#ifdef MAX311X_DEBUG_INTR
	else printk("spurrious rsmax0_interrupt rx=%x\n", rx);
#endif
#endif
	return;
}

/*
 * This is the serial driver's generic interrupt routine
 */
void rsmax1_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct max311x_serial * info = &max311x_soft_table[1];
	unsigned short rx;

	rx = spi_cycle(info->cs_line, 0);
#ifdef USE_INTS
	if (rx & MAX3111_RD_R) receive_chars(info, rx);
	if (rx & MAX3111_RD_T)   transmit_chars(info);
#else
	if (rx & MAX3111_RD_R) receive_chars(info, rx);
#ifdef MAX311X_DEBUG_INTR
	else printk("spurrious rsmax1_interrupt rx=%x\n", rx);
#endif
#endif
	return;
}

/*
 * This routine is used to handle the "bottom half" processing for the
 * serial driver, known also the "software interrupt" processing.
 * This processing is done at the kernel interrupt level, after the
 * rsmax_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using rsmax_sched_event(), and they get done here.
 */

static void do_softint(void *private_)
{
	struct max311x_serial	*info = (struct max311x_serial *) private_;
	struct tty_struct	*tty;
	
	tty = info->tty;
	if (!tty)
		return;

	if (clear_bit(RSMAX_EVENT_WRITE_WAKEUP, &info->event)) {
		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		    tty->ldisc.write_wakeup)
			(tty->ldisc.write_wakeup)(tty);
		wake_up_interruptible(&tty->write_wait);
	}
}



static int startup(struct max311x_serial * info)
{
	unsigned long flags;
	
#ifdef MAX311X_DEBUG_STARTUP
  printk("MAX startup %s%d flags=%x cs_line=%x\n", 
    max_serial_driver.name, info->line, info->flags, info->cs_line);
#endif

	if (info->flags & S_INITIALIZED)
		return 0;

	if (!info->xmit_buf) {
		info->xmit_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!info->xmit_buf)
			return -ENOMEM;
	}

	save_flags(flags); cli();

	if (info->line == 0) {
	        // Config Port D
	        PDDATA |= MAX3111_CS0;          // deselect MAX3111
        	PDSEL |= 0x10;                  // Enable PD4
	        PDDIR |= 0x10;                  // Set PD4 to output
        	PDDIR &= 0xfe;                  // Set PD0/INT0 to input
	        PDPUEN &= 0xef;                 // Disable pull-up of PD4
        	PDPUEN |= 0x01;                 // Enable pull-up of INT0/PD0
	        PDIQEG &= 0xfe;                 // Edge trigger INT0
        	PDPOL |= 0x01;                  // IRQ/INT0/PD0 is active low
	}
	else {
	        // Config Port D
	        PDDATA |= MAX3111_CS1;          // deselect MAX3111
        	PDSEL |= 0x08;                  // Enable PD3
	        PDDIR |= 0x08;                  // Set PD3 to output
        	PDDIR &= 0xfd;                  // Set PD1/INT1 to input
	        PDPUEN &= 0xf7;                 // Disable pull-up of PD3
        	PDPUEN |= 0x02;                 // Enable pull-up of INT1/PD1
	        PDIQEG &= 0xfd;                 // Edge trigger INT1
        	PDPOL |= 0x02;                  // IRQ/INT0/PD1 is active low
	}

	// Config SPI
        PESEL &= 0xf8;                  // Select SPI outputs on port E
        SPIMCONT = 0x400f;              // Init SPI clock rate and bit length
                                        // PHA=0 POL=0 16-bit div by 16
        SPIMCONT |= SPIMCONT_ENABLE;            // Enable SPI

	info->xmit_fifo_size = 1;

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * and set the speed of the serial port
	 */

	change_speed(info);

	/*
	 * Finally, enable interrupts
	 */
	if (info->line == 0)
		PDIRQEN |= 0x01;
	else
		PDIRQEN |= 0x02;

	info->flags |= S_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct max311x_serial * info)
{
	unsigned long	flags;


	save_flags(flags); cli(); /* Disable interrupts */
	info->config &= ~(MAX3111_WC_TM | MAX3111_WC_RM);
	spi_cycle(info->cs_line, info->config);
#ifdef MAX311X_DEBUG_CONF
	printk("shutdown config %x\n", info->config);
#endif

	if (!(info->flags & S_INITIALIZED)) {
		restore_flags(flags);
		return;
	}
	
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
 * This routine is called to set the UART config registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct max311x_serial *info)
{
        unsigned max_config;
	unsigned cflag;
	int	i;
	unsigned long flags;

	if (!info->tty || !info->tty->termios)
		return;
	cflag = info->tty->termios->c_cflag;

	i = cflag & CBAUD;
        if (i & CBAUDEX) {
                i = (i & ~CBAUDEX) + B38400;
        }

	if (div_table[i] < 0) {
		printk("Cannot set baud %d on MAX3111\n", i);
		return;
	}

	info->baud = max_baud_table[i];
	max_config = (0xc000 | div_table[i]);
		
	if (cflag & CSTOPB)
		max_config |= MAX3111_WC_ST;

	/* we don't support parity and CS8 settings */
	/* we always operate no parity, 8-bit */

#ifdef USE_INTS
	max_config |= (MAX3111_WC_RM | MAX3111_WC_TM);
#else
	max_config |= MAX3111_WC_RM;
#endif

	save_flags(flags); cli(); /* Disable interrupts */
	spi_cycle(info->cs_line, info->config = max_config);
	restore_flags(flags);
#ifdef MAX311X_DEBUG_CONF
	printk("change_speed config %x\n", info->config);
#endif
	return;
}


static void rsmax_set_ldisc(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rsmax_set_ldisc"))
		return;

	printk("ttyM%d setting line discipline\n", info->line);
}

static void rsmax_flush_chars(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
	unsigned rc, rx;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rsmax_flush_chars"))
		return;
#ifndef USE_INTS
	for(;;) {
#endif
	if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
	    !info->xmit_buf)
		return;

	save_flags(flags); cli();

#ifdef USE_INTS
	/* Enable transmitter interrupts */
	info->config |= MAX3111_WC_TM;
	rc = spi_cycle(info->cs_line, info->config);
#ifdef MAX311X_DEBUG_CONF
	printk("rsmax_flush_chars config %x\n", info->config);
#endif
	if (rc & MAX3111_WC_T) {
#else
	while (!(spi_cycle(info->cs_line, 0x4000) & MAX3111_WC_T)) udelay(5);
	if (1) {
#endif
		/* Send char */
#ifdef MAX311X_DEBUG_XMIT
		printk("MAX flush %x\n", info->xmit_buf[info->xmit_tail]);
#endif
		rx = spi_cycle(info->cs_line, 0x8000 | info->xmit_buf[info->xmit_tail++]);
		if (rx & MAX3111_RD_R) {
#ifdef MAX311X_DEBUG_TXRX
			printk("rsmax_flush_chars tx R %x\n", rx);
#endif
			receive_chars(info, rx);
		}
		info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
		info->xmit_cnt--;
	}
	restore_flags(flags);

#ifndef USE_INTS
	}
#endif
}

static int rsmax_write(struct tty_struct * tty, int from_user,
		    const unsigned char *buf, int count)
{
	int	c, total = 0;
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
	unsigned short rx;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rsmax_write"))
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

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped) {
		/* Enable transmitter */
		cli();		

#ifdef USE_INTS
		info->config |= MAX3111_WC_TM;
#ifdef MAX311X_DEBUG_CONF
		printk("rsmax_write config %x\n", info->config);
#endif
		if (spi_cycle(info->cs_line, info->config) & MAX3111_WC_T) {
#else
		while(info->xmit_cnt) {
			while (!(spi_cycle(info->cs_line, 0x4000) & MAX3111_WC_T)) udelay(5);
		if (1) {
#endif
#ifdef MAX311X_DEBUG_XMIT
			printk("MAX write %x\n", info->xmit_buf[info->xmit_tail]);
#endif
			rx = spi_cycle(info->cs_line, 0x8000 | info->xmit_buf[info->xmit_tail++]);
			if (rx & MAX3111_RD_R) {
#ifdef MAX311X_DEBUG_TXRX
				printk("rsmax_write tx R %x\n", rx);
#endif
				receive_chars(info, rx);
			}
			
			info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
			info->xmit_cnt--;
		}

#ifndef USE_INTS
		}
#endif
		restore_flags(flags);
	}
	restore_flags(flags);
	return total;
}

static int rsmax_write_room(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
	int	ret;
				
	if (serial_paranoia_check(info, tty->device, "rsmax_write_room"))
		return 0;
	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if (ret < 0)
		ret = 0;
	return ret;
}

static int rsmax_chars_in_buffer(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rsmax_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void rsmax_flush_buffer(struct tty_struct *tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rsmax_flush_buffer"))
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
 * rsmax_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rsmax_throttle(struct tty_struct * tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rsmax_throttle"))
		return;
	
	if (I_IXOFF(tty))
		info->x_char = STOP_CHAR(tty);

	/* Turn off RTS line (do this atomic) */
}

static void rsmax_unthrottle(struct tty_struct * tty)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rsmax_unthrottle"))
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
 * rsmax_ioctl() and friends
 * ------------------------------------------------------------
 */

static int get_serial_info(struct max311x_serial * info,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
  
	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	memcpy_tofs(retinfo,&tmp,sizeof(*retinfo));
	return 0;
}

static int set_serial_info(struct max311x_serial * info,
			   struct serial_struct * new_info)
{
	struct serial_struct new_serial;
	struct max311x_serial old_info;

	if (!new_info)
		return -EFAULT;
	memcpy_fromfs(&new_serial,new_info,sizeof(new_serial));
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
	return 0;
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
static int get_lsr_info(struct max311x_serial * info, unsigned int *value)
{
	unsigned char status;

	status = 0;
	put_user(status,value);
	return 0;
}

static int rsmax_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct max311x_serial * info = (struct max311x_serial *)tty->driver_data;
	int retval;

	if (serial_paranoia_check(info, tty->device, "rsmax_ioctl"))
		return -ENODEV;
#ifdef MAX311X_DEBUG_IOCT
	printk("rsmax_ioctl cmd %x\n", cmd);
#endif
	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
	    (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
		case TIOCGSOFTCAR:
			error = verify_area(VERIFY_WRITE, (void *) arg,sizeof(long));
			if (error)
				return error;
			put_fs_long(C_CLOCAL(tty) ? 1 : 0,
				    (unsigned long *) arg);
			return 0;
		case TIOCSSOFTCAR:
			arg = get_fs_long((unsigned long *) arg);
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
						sizeof(struct max311x_serial));
			if (error)
				return error;
			memcpy_tofs((struct max311x_serial *) arg,
				    info, sizeof(struct max311x_serial));
			return 0;
			
		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

static void rsmax_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct max311x_serial *info = (struct max311x_serial *)tty->driver_data;

	if (tty->termios->c_cflag == old_termios->c_cflag)
		return;

	change_speed(info);

	if ((old_termios->c_cflag & CRTSCTS) &&
	    !(tty->termios->c_cflag & CRTSCTS)) {
		tty->hw_stopped = 0;
		rsmax_start(tty);
	}
	
}

/*
 * ------------------------------------------------------------
 * rsmax_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * S structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void rsmax_close(struct tty_struct *tty, struct file * filp)
{
	struct max311x_serial * info = (struct max311x_serial *)tty->driver_data;
	unsigned long flags;

	if (!info || serial_paranoia_check(info, tty->device, "rsmax_close"))
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
		printk("rsmax_close: bad serial port count; tty->count is 1, "
		       "info->count is %d\n", info->count);
		if (info->count)
			info->count = 1;
	}
	if (--info->count < 0) {
		printk("rsmax_close: bad serial port count for ttyM%d: %d\n",
		       info->line, info->count);
		info->count = 0;
	}
#ifdef MAX311X_DEBUG_CNTR
	printk("rsmax_close count decremented to %d\n", info->count);
#endif
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
	info->flags &= ~(S_NORMAL_ACTIVE|S_CALLOUT_ACTIVE|
			 S_CLOSING);
	wake_up_interruptible(&info->close_wait);
	restore_flags(flags);
}

/*
 * max_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void max_hangup(struct tty_struct *tty)
{
	struct max311x_serial * info = (struct max311x_serial *)tty->driver_data;
	
	if (serial_paranoia_check(info, tty->device, "max_hangup"))
		return;
	
	rsmax_flush_buffer(tty);
	shutdown(info);
	info->event = 0;
#ifdef MAX311X_DEBUG_CNTR
	printk("max_hangup zeroing count, was %d\n", info->count);
#endif
	info->count = 0;
	info->flags &= ~(S_NORMAL_ACTIVE|S_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rsmax_open() and friends
 * ------------------------------------------------------------
 */

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its S structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
int rsmax_open(struct tty_struct *tty, struct file * filp)
{
	struct max311x_serial	*info;
	int 			retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;
	
	if (line < 0 || line >= NR_PORTS)
		return -ENODEV;

	info = &max311x_soft_table[line];

	if (serial_paranoia_check(info, tty->device, "rsmax_open"))
		return -ENODEV;

	info->count++;
#ifdef MAX311X_DEBUG_CNTR
	printk("rsmax_open count incremented to %d\n", info->count);
#endif
	tty->driver_data = info;
	info->tty = tty;

	/*
	 * Start up serial port
	 */
	if (info->count == 1) {
		retval = startup(info);
		if (retval)
			return retval;
		*tty->termios = info->normal_termios;
		change_speed(info);
		info->session = current->session;
		info->pgrp = current->pgrp;
	}
	return 0;
}

/* Finally, routines used to initialize the serial driver. */

static void show_max_version(void)
{
	printk("MAX311X serial driver version 0.20-R1\n");
}

extern void register_console(void (*proc)(const char *));

/* rsmax_init inits the driver */
int rsmax311x_init(void)
{
	int flags, i;
	struct max311x_serial *info;
	
	show_max_version();

	/* Initialize the tty_driver structure */
	/* SPARC: Not all of this is exactly right for us. */
	
	memset(&max_serial_driver, 0, sizeof(struct tty_driver));
	max_serial_driver.magic = TTY_DRIVER_MAGIC;
	max_serial_driver.name = "ttyM";
	max_serial_driver.major = MAX3111_MAJOR;
	max_serial_driver.minor_start = 64;
	max_serial_driver.num = NR_PORTS;
	max_serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	max_serial_driver.subtype = SERIAL_TYPE_NORMAL;
	max_serial_driver.init_termios = tty_std_termios;

	
        max_serial_driver.init_termios.c_cflag = 
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;

	max_serial_driver.flags = TTY_DRIVER_REAL_RAW;
	max_serial_driver.refcount = &serial_refcount;
	max_serial_driver.table = serial_table;
	max_serial_driver.termios = serial_termios;
	max_serial_driver.termios_locked = serial_termios_locked;

	max_serial_driver.open = rsmax_open;
	max_serial_driver.close = rsmax_close;
	max_serial_driver.write = rsmax_write;
	max_serial_driver.flush_chars = rsmax_flush_chars;
	max_serial_driver.write_room = rsmax_write_room;
	max_serial_driver.chars_in_buffer = rsmax_chars_in_buffer;
	max_serial_driver.flush_buffer = rsmax_flush_buffer;
	max_serial_driver.ioctl = rsmax_ioctl;
	max_serial_driver.throttle = rsmax_throttle;
	max_serial_driver.unthrottle = rsmax_unthrottle;
	max_serial_driver.set_termios = rsmax_set_termios;
	max_serial_driver.stop = rsmax_stop;
	max_serial_driver.start = rsmax_start;
	max_serial_driver.hangup = max_hangup;
	max_serial_driver.set_ldisc = rsmax_set_ldisc;

	if (tty_register_driver(&max_serial_driver))
		panic("Couldn't register max_serial_driver driver\n");
	
	save_flags(flags); cli();


	for (i = 0; i < NR_PORTS; i++) {
		info = &max311x_soft_table[i];
		info->magic = SERIAL_MAGIC;
		info->tty = 0;
		info->close_delay = 50;
		info->closing_wait = 3000;
		info->x_char = 0;
		info->event = 0;
		info->count = 0;
		info->blocked_open = 0;
		info->tqueue.routine = do_softint;
		info->tqueue.data = info;
		info->normal_termios = max_serial_driver.init_termios;
		info->open_wait = 0;
		info->close_wait = 0;
		info->line = i;

		if (i == 0) {
		        if (request_irq(IRQ_MACHSPEC | INT0_IRQ_NUM,
                	        rsmax0_interrupt,
                        	IRQ_FLG_STD,
	                        "MAX311X_UART0", NULL))
	        	        panic("Unable to attach INT0 MAX311X interrupt\n");
			info->cs_line = MAX3111_CS0;
		}
		else {
		        if (request_irq(IRQ_MACHSPEC | INT1_IRQ_NUM,
                	        rsmax1_interrupt,
                        	IRQ_FLG_STD,
	                        "MAX311X_UART1", NULL))
        		        panic("Unable to attach INT1 MAX311X interrupt\n");
			info->cs_line = MAX3111_CS1;
		}
		printk("%s%d MAX311X UART CS=0x%x using tx ",
		   max_serial_driver.name, info->line, info->cs_line);
#ifdef USE_INTS
			printk("interrupts\n");
#else
			printk("polling\n");
#endif
	}
	restore_flags(flags);
	return 0;
}

