/* 68302serial.c: Serial port driver for 68302 microcontroller
 *
 * Based on:
 *
 * drivers/char/68328serial.c
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
#if 0
#include <asm/kdebug.h>
#endif

#include "68302serial.h"

#define USE_INTS	1
//#undef USE_INTS
#undef USE_ONLY_RX_INTS

static int m68k_use_ints=0;
#define SET_INTS_MODE(x)	m68k_use_ints=x

static struct m68k_serial m68k_soft;

struct tty_struct m68k_ttys;
/** struct tty_struct *m68k_constty; **/

/* Console hooks... */
/*static int m68k_cons_chanout = 0;
static int m68k_cons_chanin = 0;*/

struct m68k_serial *m68k_consinfo = 0;

#if 0
static unsigned char kgdb_regs[16] = {
	0, 0, 0,                     /* write 0, 1, 2 */
	(Rx8 | RxENABLE),            /* write 3 */
	(X16CLK | SB1 | PAR_EVEN),   /* write 4 */
	(Tx8 | TxENAB),              /* write 5 */
	0, 0, 0,                     /* write 6, 7, 8 */
	(NV),                        /* write 9 */
	(NRZ),                       /* write 10 */
	(TCBR | RCBR),               /* write 11 */
	0, 0,                        /* BRG time constant, write 12 + 13 */
	(BRSRC | BRENABL),           /* write 14 */
	(DCDIE)                      /* write 15 */
};
#endif

extern u_long clock_frequency;
#define M68K_CLOCK clock_frequency
#if 0
#ifdef CONFIG_CLOCK_20MHz
#define M68K_CLOCK (20000000) /* FIXME: 16MHz is likely wrong */
#else
#define M68K_CLOCK (25000000) /* FIXME: 16MHz is likely wrong */
#endif
#endif

DECLARE_TASK_QUEUE(tq_serial);

struct tq_struct serialpoll;

struct tty_driver serial_driver, callout_driver;
static int serial_refcount;

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

static void change_speed(struct m68k_serial *info);

static struct tty_struct *serial_table[2];
static struct termios *serial_termios[2];
static struct termios *serial_termios_locked[2];

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#define CONSOLE_SC

static char prompt0,prompt1,prompt2;
static char	rx_buffer[128];		// a little buffer for console SCC
static void scc0_xmit_char(char ch);
static void scc0_xmit_string(char *p, int len);
static void scc0_start_rx(char bd, char *p_start, ushort size, char last);
static void scc_wait_EOT(ushort scc, ushort bd);
static void scc68302_init(void);
static void scc68302_speed(unsigned baud, unsigned cflag);
static int	scc_cmd(ushort scc, ushort opcode);

#define SCC_EOT(X,Y)	(ushort)(!(SCC_TXBD_1W(X,Y) & 0x8000))

static ushort sccmask[]={IxR_SCC1, IxR_SCC2, IxR_SCC3};

#define scc_irq_enable(X)	IMR |= sccmask[X];
#define scc_irq_disable(X)	IMR	&= ~sccmask[X];
#define scc_clear_ev(X,Y)	SCCE(X) |= Y;
#define scc_clear_isr(X)	ISR |= sccmask[X];

#define scc_mask(X)		SCCM(X)=0
// unmask CTS, TX, RX
#define scc_unmask(X)	SCCM(X)=0x83	

void tx_enable(ushort scc);
void rx_enable(ushort scc);
void tx_disable(ushort scc);
void rx_disable(ushort scc);
void tx_stop(ushort scc);
void tx_start(ushort scc);
void rx_stop(ushort scc);
void rx_start(ushort scc);


void _INLINE_ tx_enable(ushort scc){
	SCM(scc) |= SCCM_ENT;
	if(m68k_use_ints)	
		SCCM(scc) |= SCCE_TX;
}
void _INLINE_ rx_enable(ushort scc){
	SCM(scc) |= SCCM_ENR;
	if(m68k_use_ints)	
		SCCM(scc) |= SCCE_RX;
}
void _INLINE_ tx_disable(ushort scc){
	SCM(scc) &= ~SCCM_ENT;
	SCCM(scc) &= ~SCCE_TX;
}
void _INLINE_ rx_disable(ushort scc){
	SCM(scc) &= ~SCCM_ENR;
	SCCM(scc) &= ~SCCE_RX;
}

void _INLINE_ tx_stop(ushort scc){
	scc_cmd(scc, CR_OP_STOP_TX);
	tx_disable(scc);
}
void _INLINE_ tx_start(ushort scc){
	scc_cmd(scc, CR_OP_REST_TX);
	tx_enable(scc);
}
void _INLINE_ rx_stop(ushort scc){
	rx_disable(scc);
}
void _INLINE_ rx_start(ushort scc){
	scc_cmd(scc, CR_OP_HUNT_MODE);
	rx_enable(scc);
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
static unsigned char tmp_buf[4096]; /* This is cheating */
static struct semaphore tmp_buf_sem = MUTEX;

static inline int serial_paranoia_check(struct m68k_serial *info,
					dev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%d, %d) in %s\n";
	static const char *badinfo =
		"Warning: null m68k_serial for (%d, %d) in %s\n";

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
static inline void m68k_rtsdtr(struct m68k_serial *ss, int set)
{
	if(set) {
		/* set the RTS/CTS line */
	} else {
		/* clear it */
	}
	return;
}

static inline void kgdb_chaninit(struct m68k_serial *ss, int intson, int bps)
{
#if 0
	int brg;

	if(intson) {
		kgdb_regs[R1] = INT_ALL_Rx;
		kgdb_regs[R9] |= MIE;
	} else {
		kgdb_regs[R1] = 0;
		kgdb_regs[R9] &= ~MIE;
	}
	brg = BPS_TO_BRG(bps, ZS_CLOCK/16);
	kgdb_regs[R12] = (brg & 255);
	kgdb_regs[R13] = ((brg >> 8) & 255);
	load_zsregs(ss->m68k_channel, kgdb_regs);
#endif
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
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_stop"))
		return;
	
	save_flags(flags); cli();
	tx_stop(0);
	rx_stop(0);
	restore_flags(flags);
}

static void rs_put_char(char ch)
{
        int flags, loops = 0;

        save_flags(flags); cli();
		scc_wait_EOT(0,0);
		scc0_xmit_char(ch);
		scc_wait_EOT(0,0);
        restore_flags(flags);
}


static void rs_start(struct tty_struct *tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
	unsigned long flags;
	
	if (serial_paranoia_check(info, tty->device, "rs_start"))
		return;
	
	save_flags(flags); cli();
	tx_start(0);
	scc0_start_rx(0,rx_buffer, 3,1);
	rx_start(0);
	restore_flags(flags);
}

/* Drop into either the boot monitor or kadb upon receiving a break
 * from keyboard/console input.
 */
static void batten_down_hatches(void)
{
	/* If we are doing kadb, we call the debugger
	 * else we just drop into the boot monitor.
	 * Note that we must flush the user windows
	 * first before giving up control.
	 */
#if 0
	if((((unsigned long)linux_dbvec)>=DEBUG_FIRSTVADDR) &&
	   (((unsigned long)linux_dbvec)<=DEBUG_LASTVADDR))
		sp_enter_debugger();
	else
		panic("m68k_serial: batten_down_hatches");
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
 * and look at the resulting assemble code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct m68k_serial *info,
				    int event)
{
	info->event |= 1 << event;
	queue_task_irq_off(&info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}

extern void breakpoint(void);  /* For the KGDB frame character */

static _INLINE_ void receive_chars(struct m68k_serial *info, struct pt_regs *regs, unsigned short status)
{
	unsigned char ch;
	int count,stat;
#if 1
	// hack to receive chars by polling from anywhere
	struct m68k_serial * info1 = &m68k_soft;
	struct tty_struct *tty = info1->tty;
	if (!(info->flags & S_INITIALIZED))
		return;
#else
	struct tty_struct *tty = info->tty;
#endif	
	count = SCC_RXBD_2W(0,0);
	stat = SCC_RXBD_1W(0,0);
	// hack to receive chars by polling only BD fields
	if (stat & 0x8000 || !count){
		return;
	}
	ch = rx_buffer[0];
	if(info->is_cons) {
		if(status & SCCE_BRK) { /* whee, break received */
			batten_down_hatches();
			/*rs_recv_clear(info->m68k_channel);*/
			return;
		} else if (ch == 0x10) { /* ^P */
			show_state();
			show_free_areas();
			show_buffers();
			show_net_buffers();
			return;
		} else if (ch == 0x12) { /* ^R */
			hard_reset_now();
			return;
		}
		/* It is a 'keyboard interrupt' ;-) */
		wake_up(&keypress_wait);
	}
	/* Look for kgdb 'stop' character, consult the gdb documentation
	 * for remote target debugging and arch/sparc/kernel/sparc-stub.c
	 * to see how all this works.
	 */
	/*if((info->kgdb_channel) && (ch =='\003')) {
		breakpoint();
		goto clear_and_exit;
	}*/

	if(!tty)
		goto clear_and_exit;

	if (tty->flip.count >= TTY_FLIPBUF_SIZE)
		queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
	tty->flip.count++;
	if(stat & RX_BD_PR)
		*tty->flip.flag_buf_ptr++ = TTY_PARITY;
	else if(stat & RX_BD_OV)
		*tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
	else if(stat & RX_BD_FR)
		*tty->flip.flag_buf_ptr++ = TTY_FRAME;
	else
		*tty->flip.flag_buf_ptr++ = 0; /* XXX */
	*tty->flip.char_buf_ptr++ = ch;

	queue_task_irq_off(&tty->flip.tqueue, &tq_timer);

clear_and_exit:
	scc0_start_rx(0,rx_buffer,1,1);
	return;
}

static _INLINE_ void transmit_chars(struct m68k_serial *info)
{
	if (info->x_char) {
		/* Send next char */
		scc0_xmit_char(info->x_char);
		info->x_char = 0;
		goto clear_and_return;
	}

	if((info->xmit_cnt <= 0) || info->tty->stopped) {
		/* That's peculiar... */
//		tx_stop(0);
		goto clear_and_return;
	}

	/* Send char */
	scc0_xmit_char(info->xmit_buf[info->xmit_tail++]);
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	info->xmit_cnt--;

	if (info->xmit_cnt < WAKEUP_CHARS)
		rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);

	if(info->xmit_cnt <= 0) {
//		tx_stop(0);
		goto clear_and_return;
	}

clear_and_return:
	/* Clear interrupt (should be auto)*/
	return;
}

static _INLINE_ void status_handle(struct m68k_serial *info, unsigned short status)
{
#if 0
	if(status & DCD) {
		if((info->tty->termios->c_cflag & CRTSCTS) &&
		   ((info->curregs[3] & AUTO_ENAB)==0)) {
			info->curregs[3] |= AUTO_ENAB;
			info->pendregs[3] |= AUTO_ENAB;
			write_zsreg(info->m68k_channel, 3, info->curregs[3]);
		}
	} else {
		if((info->curregs[3] & AUTO_ENAB)) {
			info->curregs[3] &= ~AUTO_ENAB;
			info->pendregs[3] &= ~AUTO_ENAB;
			write_zsreg(info->m68k_channel, 3, info->curregs[3]);
		}
	}
#endif
	/* Whee, if this is console input and this is a
	 * 'break asserted' status change interrupt, call
	 * the boot prom.
	 */
	if((status & SCCE_BRK) && info->break_abort)
		batten_down_hatches();

	/* XXX Whee, put in a buffer somewhere, the status information
	 * XXX whee whee whee... Where does the information go...
	 */
	return;
}

/*
 * This is the serial driver's generic interrupt routine
 */
void rs_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct m68k_serial * info = &m68k_soft;
	ushort status;
	status = SCCE(0);
	
	if(m68k_use_ints){
		if (status & SCCE_TX) {
			scc_clear_ev(0,SCCE_TX);
			transmit_chars(info);
		}
		if (status & SCCE_RX){
			scc_clear_ev(0,SCCE_RX);
			receive_chars(info, regs, status);
		}
	}else{
		receive_chars(info, regs, status);
	}
	if (status & (SCCE_IDLE | SCCE_BRK | SCCE_CTS | SCCE_CD | SCCE_CCR | SCCE_BSY)) status_handle(info, status);
	scc_clear_isr(0);
	
	if(!m68k_use_ints)	
		queue_task_irq_off(&serialpoll, &tq_timer);
	return;
}
static void serpoll(void *data){
	rs_interrupt(0, NULL, NULL);
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
	struct m68k_serial	*info = (struct m68k_serial *) private_;
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
	struct m68k_serial	*info = (struct m68k_serial *) private_;
	struct tty_struct	*tty;
	
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
static void scc68302_init(void){
	SIMODE = 0;
	SCC_MRBLR(0) = 3;
	SCC_MAX_IDL(0)=0xF;
	SCC_RFCR(0)= 0;
	SCC_BRKCR(0)= 1;
	SCC_PAREC(0)=0;
	SCC_FRMEC(0)=0;
	SCC_NOSEC(0)=0;
	SCC_BRKEC(0)=0;
	SCC_UADDR1(0)=0;
	SCC_UADDR2(0)=0;
	SCC_CARACT(0)=0x8000;
	SCCE(0)=0xFF;
	SCCM(0)=0;				/* all it disabled */
	SCC_TXBD_1W(0,0) = 0x2000;		/* block emission */
}

static void scc68302_speed(unsigned baud, unsigned cflag){
	unsigned format;

	if ((20000000 == M68K_CLOCK))
	{
		SCON(0) = 2*(ushort)((M68K_CLOCK/16)/baud-1);
	}
	else if ((25000000 == M68K_CLOCK))
	{
	if (baud > 115200)
		baud = 115200;
	if (baud != 115200 && baud != 9600)
		baud = 57600;
	switch(baud){
		case 115200:
			SCON(0)=0x1A;	/* 115200 */
			break;
		case 57600:
			SCON(0)=0x34;	/* 57600 */
			break;
		case 9600:
			SCON(0)=0x144; /* 9600 */
			break;
		default:
			SCON(0)=0x142;
			break;
	}
	}	

	/* set parity */
	SCM(0)=0x171;			/* no parity normal uart 8 bits 1 stop bit */

	if (cflag != 0xffff){
		format = SCM(0)& 0x3F;	/* dont change the common settings */
		if ((cflag & CSIZE) == CS8)
			format |= SCM_CL;

		if (cflag & CSTOPB)
			format |= SCM_SL;

		if (cflag & PARENB)
			format |= SCM_PEN;

		if (cflag & PARODD)
			format |= SCM_ODD;

		SCM(0) = format;
	}
	tx_start(0);
	scc0_start_rx(0,rx_buffer, 3,1);
	rx_start(0);
}
static int	scc_cmd(ushort scc, ushort opcode){
	SCR = scc<<1 | opcode | CR_FLG;
	while(SCR & CR_FLG){;}
}
static void scc_wait_EOT(ushort scc, ushort bd){
	volatile ushort status;
#ifdef STBUG
	return;
#endif
	if(!(SCM(scc)&SCCM_ENT)){
		return;
		tx_start(scc);
	}
	status = SCC_TXBD_1W(scc,bd);
	while(status & 0x8000) {
		status = SCC_TXBD_1W(scc,bd);
	}
}
static int startup(struct m68k_serial * info)
{
	unsigned long flags;
	
	if (info->flags & S_INITIALIZED)
		return 0;

	if (!info->xmit_buf) {
		info->xmit_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!info->xmit_buf)
			return -ENOMEM;
	}
	save_flags(flags); cli();
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

	scc68302_init();
	SET_INTS_MODE(1);
	change_speed(info);
//	scc0_start_rx(0,rx_buffer, 3,1);
//	rx_start(0);
//	scc_irq_enable(0);

	info->flags |= S_INITIALIZED;
	restore_flags(flags);
	return 0;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct m68k_serial * info)
{
	unsigned long	flags;

	scc_irq_disable(0);
	tx_stop(0); /* All off! */
	rx_stop(0); /* All off! */
	if (!(info->flags & S_INITIALIZED))
		return;

#ifdef SERIAL_DEBUG_OPEN
	printk("Shutting down serial port %d (irq %d)....\n", info->line,
	       info->irq);
#endif
	
	save_flags(flags); cli(); /* Disable interrupts */
	
	if (info->xmit_buf) {
		free_page((unsigned long) info->xmit_buf);
		info->xmit_buf = 0;
	}

	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);
	
	info->flags &= ~S_INITIALIZED;
	restore_flags(flags);
}

struct {
	int divisor, prescale;
} hw_baud_table[18] = {
	{0,0}, /* 0 */
	{0,0}, /* 50 */
	{0,0}, /* 75 */
	{0,0}, /* 110 */
	{0,0}, /* 134 */
	{0,0}, /* 150 */
	{0,0}, /* 200 */
	{7,0x26}, /* 300 */
	{6,0x26}, /* 600 */
	{5,0x26}, /* 1200 */
	{0,0}, /* 1800 */
	{4,0x26}, /* 2400 */
	{3,0x26}, /* 4800 */
	{2,0x26}, /* 9600 */
	{1,0x26}, /* 19200 */
	{0,0x26}, /* 38400 */
	{1,0x38}, /* 57600 */
	{0,0x38}, /* 115200 */
};
/* rate = 1036800 / ((65 - prescale) * (1<<divider)) */

static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 0 };

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct m68k_serial *info)
{
	unsigned short port;
	unsigned cflag;
	int	i;

	if (!info->tty || !info->tty->termios)
		return;
	cflag = info->tty->termios->c_cflag;
	if (!(port = info->port))
		return;

	/* First disable the interrupts */
	scc_irq_disable(0);
	tx_stop(0);
	rx_stop(0);
	/* set the baudrate */
	i = cflag & CBAUD;

	info->baud = baud_table[i];
	scc68302_speed(info->baud, cflag);
	scc0_start_rx(0,rx_buffer, 3,1);
	if(m68k_use_ints){
		tx_start(0);
		rx_start(0);
		scc_irq_enable(0);
	}
	return;
}


void scc0_start_rx(char bd, char *p_start, ushort size, char last){
#if 0	
	ulong scc_rx_bd_addr;
	ulong  scc_rx_bd_ctrl;
	scc_rx_bd_addr = SCC_BASE + bd * SCC_BD_SIZE * 2 + 4;
	scc_rx_bd_ctrl = SCC_BASE + bd * SCC_BD_SIZE * 2;

	*((ulong *)scc_rx_bd_addr) = ( ulong )p_start ;	/* adresse reception caratere dans RECEPT*/
	if (last){
		*((ushort *)scc_rx_bd_ctrl) = 0xE000; /* last buffer, start reception */
	}else{
		*((ushort *)scc_rx_bd_ctrl) = 0xC000; /* chained buffer, start reception */
	}
#else
	SCC_RXBD_1L(0,bd) = (ulong)p_start;
	if (last)
		SCC_RXBD_1W(0,bd) = 0xF000;  /* last buffer, start reception */
	else
		SCC_RXBD_1W(0,bd) = 0xD000;  /* chained buffer, start reception */
	
#endif	
}
static void scc0_xmit_char(char ch){
	prompt0 = ch;
	SCC_TXBD_2W(0,0) = (ushort)1; /* nbr de caracteres à emettre */
	SCC_TXBD_1L(0,0) = ( ulong )&prompt0; /* envoi contenu de recept */
	if(m68k_use_ints){	
		SCC_TXBD_1W(0,0)= 0xF200; /* start emission */
	}else{
		SCC_TXBD_1W(0,0)= 0xE200; /* start emission */
		scc_wait_EOT(0,0);
	}
}
static void scc0_xmit_string(char *p, int len){
	SCC_TXBD_2W(0,0) = (ushort)len;		/* nbr de caracteres à emettre */
	SCC_TXBD_1L(0,0) = ( ulong )p; /* envoi contenu de recept */
	if(m68k_use_ints){
		SCC_TXBD_1W(0,0)= 0xF200; /* start emission */
	}else{		
		SCC_TXBD_1W(0,0) = 0xE200;		/* start emission */
		scc_wait_EOT(0,0);
	}
}

#if 0
/* These are for receiving and sending characters under the kgdb
 * source level kernel debugger.
 */
void putDebugChar(char kgdb_char)
{
	struct sun_zschannel *chan = m68k_kgdbchan;

	while((chan->control & Tx_BUF_EMP)==0)
		udelay(5);

	chan->data = kgdb_char;
}

char getDebugChar(void)
{
	struct sun_zschannel *chan = m68k_kgdbchan;

	while((chan->control & Rx_CH_AV)==0)
		barrier();
	return chan->data;
}
#endif

/*
 * Fair output driver allows a process to speak.
 */
static void rs_fair_output(void)
{
	int left;		/* Output no more than that */
	unsigned long flags;
	struct m68k_serial *info = &m68k_soft;
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

		rs_put_char(c);

		save_flags(flags);  cli();
		left = MIN(info->xmit_cnt, left-1);
	}

	/* Last character is being transmitted now (hopefully). */
//	udelay(20);
	scc_wait_EOT(0,0);
	restore_flags(flags);
	return;
}

/*
 * m68k_console_print is registered for printk.
 */
void console_print_68302(const char *p)
{
	char c;
	struct m68k_serial *info;
	info = &m68k_soft;

	if (!(info->flags & S_INITIALIZED)){
		scc68302_init();
		scc68302_speed(9600,0xffff);
	}
	while((c=*(p++)) != 0) {
		if(c == '\n')
			rs_put_char('\r');
		rs_put_char(c);
	}

	/* Comment this if you want to have a strict interrupt-driven output */
	if (!m68k_use_ints)
		rs_fair_output();

	return;
}

static void rs_set_ldisc(struct tty_struct *tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_set_ldisc"))
		return;

	info->is_cons = (tty->termios->c_line == N_TTY);
	
	printk("ttyS%d console mode %s\n", info->line, info->is_cons ? "on" : "off");
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_flush_chars"))
		return;
	if(!m68k_use_ints){	
		for(;;) {
			if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
				!info->xmit_buf)
				return;

			/* Enable transmitter */
			save_flags(flags); cli();
			tx_start(0);
		}
	}else{
		if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped ||
			!info->xmit_buf)
			return;

			/* Enable transmitter */
		save_flags(flags); cli();
		tx_start(0);
	}

	if(!m68k_use_ints)	
		scc_wait_EOT(0,0);
		/* Send char */
	scc0_xmit_char( info->xmit_buf[info->xmit_tail++]);
	info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
	info->xmit_cnt--;
	
	restore_flags(flags);
}

extern void console_printn(const char * b, int count);

static int rs_write(struct tty_struct * tty, int from_user,
		    const unsigned char *buf, int count)
{
	int	c, total = 0;
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_write"))
		return 0;

	if (!tty || !info->xmit_buf)
		return 0;
	
	/*buf = "123456";
	count = 6;
	
	printk("Writing '%s' to serial port\n", buf);*/

	/*printk("rs_write of %d bytes\n", count);*/


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

	if (info->xmit_cnt && !tty->stopped && !tty->hw_stopped ){
		/* Enable transmitter */
		
		cli();		
		/*printk("Enabling transmitter\n");*/

		if(!m68k_use_ints){	
			while(info->xmit_cnt) {
				scc_wait_EOT(0,0);
				/* Send char */
				scc0_xmit_char(info->xmit_buf[info->xmit_tail++]);
				scc_wait_EOT(0,0);
				info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
				info->xmit_cnt--;
			}
		}else{
			if (info->xmit_cnt){
				/* Send char */
				scc_wait_EOT(0,0);
				// xmit_string should be use instead
				scc0_xmit_char(info->xmit_buf[info->xmit_tail++]);
				info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
				info->xmit_cnt--;
			}
		}
		restore_flags(flags);
	} else {
		/*printk("Skipping transmit\n");*/
	}


#if 0
		printk("Enabling stuff anyhow\n");
		tx_start(0);

		if (SCC_EOT(0,0)) {
			printk("TX FIFO empty.\n");
			/* Send char */
			scc0_xmit_char(info->xmit_buf[info->xmit_tail++]);
			info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE-1);
			info->xmit_cnt--;
		}
#endif

	restore_flags(flags);
	return total;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
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
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_chars_in_buffer"))
		return 0;
	return info->xmit_cnt;
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
				
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
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	printk("throttle %s: %d....\n", _tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif

	if (serial_paranoia_check(info, tty->device, "rs_throttle"))
		return;
	
	if (I_IXOFF(tty))
		info->x_char = STOP_CHAR(tty);

	/* Turn off RTS line (do this atomic) */
}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
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

static int get_serial_info(struct m68k_serial * info,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
  
	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->line;
	tmp.port = info->port;
	tmp.irq = info->irq;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	tmp.custom_divisor = info->custom_divisor;
	memcpy_tofs(retinfo,&tmp,sizeof(*retinfo));
	return 0;
}

static int set_serial_info(struct m68k_serial * info,
			   struct serial_struct * new_info)
{
	struct serial_struct new_serial;
	struct m68k_serial old_info;
	int 			retval = 0;

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
static int get_lsr_info(struct m68k_serial * info, unsigned int *value)
{
	unsigned char status;

	cli();
	if(m68k_use_ints)	
		status = (SCC_EOT(0,0))? 1 : 0;
	else
		status = 0;
	sti();
	put_user(status,value);
	return 0;
}

/*
 * This routine sends a break character out the serial port.
 */
static void send_break(	struct m68k_serial * info, int duration)
{
	if (!info->port)
		return;
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();
	if(m68k_use_ints)	
		scc_cmd(0,CR_OP_STOP_TX);
	sti();
}

static int rs_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	int error;
	struct m68k_serial * info = (struct m68k_serial *)tty->driver_data;
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
						sizeof(struct m68k_serial));
			if (error)
				return error;
			memcpy_tofs((struct m68k_serial *) arg,
				    info, sizeof(struct m68k_serial));
			return 0;
			
		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

static void rs_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct m68k_serial *info = (struct m68k_serial *)tty->driver_data;

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
static void rs_close(struct tty_struct *tty, struct file * filp)
{
	struct m68k_serial * info = (struct m68k_serial *)tty->driver_data;
	unsigned long flags;

	if (!info || serial_paranoia_check(info, tty->device, "rs_close"))
		return;
	
	save_flags(flags); cli();
	
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
	// closing port so disable SCC interrupts
	SET_INTS_MODE(0);
	
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
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
void rs_hangup(struct tty_struct *tty)
{
	struct m68k_serial * info = (struct m68k_serial *)tty->driver_data;
	
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
			   struct m68k_serial *info)
{
	struct wait_queue wait = { current, NULL };
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
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready before block: ttyS%d, count = %d\n",
	       info->line, info->count);
#endif
	info->count--;
	info->blocked_open++;
	while (1) {
		cli();
		if (!(info->flags & S_CALLOUT_ACTIVE))
			m68k_rtsdtr(info, 1);
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
	if(!m68k_use_ints)	
		queue_task(&serialpoll, &tq_timer);
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
	struct m68k_serial	*info;
	int 			retval, line;

	line = MINOR(tty->device) - tty->driver.minor_start;
	
	if (line != 0) /* we have exactly one */
		return -ENODEV;

	info = &m68k_soft;
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
	return 0;
}

/* Finally, routines used to initialize the serial driver. */

static void show_serial_version(void)
{
	printk("MC68302 serial driver version 1.00\n");
}

extern void register_console(void (*proc)(const char *));
#if 0

static inline void
rs_cons_check(struct m68k_serial *ss, int channel)
{
	int i, o, io;
	static consout_registered = 0;
	static msg_printed = 0;

	i = o = io = 0;

	/* Is this one of the serial console lines? */
	if((m68k_cons_chanout != channel) &&
	   (m68k_cons_chanin != channel))
		return;
	m68k_conschan = ss->m68k_channel;
	m68k_consinfo = ss;

	/* Register the console output putchar, if necessary */
	if((m68k_cons_chanout == channel)) {
		o = 1;
		/* double whee.. */
		if(!consout_registered) {
			register_console(m68k_console_print);
			consout_registered = 1;
		}
	}

	/* If this is console input, we handle the break received
	 * status interrupt on this line to mean prom_halt().
	 */
	if(m68k_cons_chanin == channel) {
		ss->break_abort = 1;
		i = 1;
	}
	if(o && i)
		io = 1;
	if(ss->m68k_baud != 9600)
		panic("Console baud rate weirdness");

	/* Set flag variable for this port so that it cannot be
	 * opened for other uses by accident.
	 */
	ss->is_cons = 1;

	if(io) {
		if(!msg_printed) {
			printk("zs%d: console I/O\n", ((channel>>1)&1));
			msg_printed = 1;
		}
	} else {
		printk("zs%d: console %s\n", ((channel>>1)&1),
		       (i==1 ? "input" : (o==1 ? "output" : "WEIRD")));
	}
}
#endif

volatile int test_done;

/* rs_init inits the driver */
int rs68302_init(void)
{
	int flags;
	struct m68k_serial *info;
	m68k_use_ints=0;
	/* Setup base handler, and timer table. */
	init_bh(SERIAL_BH, do_serial_bh);
	timer_table[RS_TIMER].fn = rs_timer;
	timer_table[RS_TIMER].expires = 0;

	/* Initialize the tty_driver structure */
	
	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic = TTY_DRIVER_MAGIC;
	serial_driver.name = "ttyS";
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64;
	serial_driver.num = 1;
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
	
	save_flags(flags); cli();

	info = &m68k_soft;
	info->magic = SERIAL_MAGIC;
	info->port = 1;
	info->tty = 0;
	info->irq = 0;	/* SCC1 irq;*/
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
	info->line = 0;
	info->is_cons = 1; /* Means shortcuts work */
#if 0
	if (request_irq(scc_irq,
					rs_interrupt,
					(SA_INTERRUPT | SA_STATIC_ALLOC),
					"68302 SCC", NULL))
		panic("Unable to attach scc intr\n");
#endif
	
	restore_flags(flags);
// hack to do polling
	serialpoll.routine = serpoll;
	serialpoll.data = (void *)&m68k_soft;
	
	return 0;
}

/*
 * register_serial and unregister_serial allows for serial ports to be
 * configured at run-time, to support PCMCIA modems.
 */
/* SPARC: Unused at this time, just here to make things link. */
int register_serial(struct serial_struct *req)
{
	return -1;
}

void unregister_serial(int line)
{
	return;
}

#if 0
/* Hooks for running a serial console.  con_init() calls this if the
 * console is being run over one of the ttya/ttyb serial ports.
 * 'chip' should be zero, as chip 1 drives the mouse/keyboard.
 * 'channel' is decoded as 0=TTYA 1=TTYB, note that the channels
 * are addressed backwards, channel B is first, then channel A.
 */
void
rs_cons_hook(int chip, int out, int channel)
{
	if(chip)
		panic("rs_cons_hook called with chip not zero");
	if(!m68k_chips[chip]) {
		m68k_chips[chip] = get_zs(chip);
		/* Two channels per chip */
		m68k_channels[(chip*2)] = &m68k_chips[chip]->channelA;
		m68k_channels[(chip*2)+1] = &m68k_chips[chip]->channelB;
	}
	m68k_soft[channel].m68k_channel = m68k_channels[channel];
	m68k_soft[channel].change_needed = 0;
	m68k_soft[channel].clk_divisor = 16;
	m68k_soft[channel].m68k_baud = get_zsbaud(&m68k_soft[channel]);
	rs_cons_check(&m68k_soft[channel], channel);
	if(out)
		m68k_cons_chanout = ((chip * 2) + channel);
	else
		m68k_cons_chanin = ((chip * 2) + channel);

}

/* This is called at boot time to prime the kgdb serial debugging
 * serial line.  The 'tty_num' argument is 0 for /dev/ttya and 1
 * for /dev/ttyb which is determined in setup_arch() from the
 * boot command line flags.
 */
void
rs_kgdb_hook(int tty_num)
{
	int chip = 0;

	if(!m68k_chips[chip]) {
		m68k_chips[chip] = get_zs(chip);
		/* Two channels per chip */
		m68k_channels[(chip*2)] = &m68k_chips[chip]->channelA;
		m68k_channels[(chip*2)+1] = &m68k_chips[chip]->channelB;
	}
	m68k_soft[tty_num].m68k_channel = m68k_channels[tty_num];
	m68k_kgdbchan = m68k_soft[tty_num].m68k_channel;
	m68k_soft[tty_num].change_needed = 0;
	m68k_soft[tty_num].clk_divisor = 16;
	m68k_soft[tty_num].m68k_baud = get_zsbaud(&m68k_soft[tty_num]);
	m68k_soft[tty_num].kgdb_channel = 1;     /* This runs kgdb */
	m68k_soft[tty_num ^ 1].kgdb_channel = 0; /* This does not */
	/* Turn on transmitter/receiver at 8-bits/char */
	kgdb_chaninit(&m68k_soft[tty_num], 0, 9600);
	ZS_CLEARERR(m68k_kgdbchan);
	udelay(5);
	ZS_CLEARFIFO(m68k_kgdbchan);
}
#endif
