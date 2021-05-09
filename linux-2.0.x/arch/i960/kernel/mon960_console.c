/*
 *   FILE: mon960_console.c
 * AUTHOR: kma
 *  DESCR: serial console for TI's 16552 serial controller
 */

#ident "$Id: mon960_console.c,v 1.2 2002/06/18 00:31:50 gerg Exp $"

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/major.h>
#include <linux/interrupt.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <asm/board/serial.h>

static int console_refcount;
static struct tty_struct *console_table[1];
static struct termios	*console_termios[1];
static struct termios 	*console_termios_locked[1];
static struct tty_driver console_driver;

/*
 * The interrupt for the 16552; should only happen for reads
 */
void do_16552_intr(void)
{
	unsigned char		buf[16];
	int			ii;
	struct tty_struct	*tty = console_table[0];
	unsigned long		lsr;

	/* Drain the fifo into the tty */
	for (ii=0; (lsr = *SER16552_LSR) & LSR_RECEIVE_READY; ii++) {
		/* If there was a break, enter the debugger */
#ifdef CONFIG_MON960
		char	garbage;
		if (lsr & LSR_BREAK) {
			extern void system_break(void);
			system_break();
			garbage = *SER16552_RBR;
			ii--;
		}
		else
#endif
			buf[ii] = *SER16552_RBR;
	}
	

	if (ii)
		tty->ldisc.receive_buf(tty, buf, 0, ii);
}

#if 0
static void do_16552_bh(void)
{
	printk("in 16552 bottom half!\n");
	printk("umm... yeah.\n");
}
#endif

static inline void putc_16552(char c, int blocking)
{
	unsigned long	flags;

	save_flags(flags);
	cli();
top:
	while (!(*SER16552_LSR & LSR_SEND_READY)) {
		udelay(1000);
	}
	
	*SER16552_THR = c;
	
	if (c == '\n') {
		c = '\r';
		goto top;
	}
	restore_flags(flags);
}

void console_print_mon960(const char* msg)
{
	long flags;
	char c;
	
	save_flags(flags);
	cli();
	while (c = *msg++)
		putc_16552(c, 0);

	restore_flags(flags);
}

/*
 * XXX: make this interrupt driven if it's coming from the user
 */
static int con_write(struct tty_struct* tty, int from_user,
		     const unsigned char* buf, int count)
{
	int ii = count;
	
	while (ii--)
		putc_16552(*buf++, from_user);
	return count;
}

static int con_chars_in_buffer(struct tty_struct *tty)
{
	return 0;		/* we're not buffering */
}

static int con_writeroom(struct tty_struct *tty)
{
	return	8192;		/* could return anything */
}

static void con_putchar(struct tty_struct* tty, unsigned char c)
{
	putc_16552(c, 0);
}

static int con_open(struct tty_struct* tty, struct file* filp)
{
	return 0;
}

/*
 * register our tty driver
 */

void mon960_console_init(void)
{
	memset(&console_driver, 0, sizeof(console_driver));
	console_driver.magic = TTY_DRIVER_MAGIC;
	console_driver.name = "tty";
	console_driver.name_base = 1;
	console_driver.major = TTY_MAJOR;
	console_driver.minor_start = 1;
	console_driver.num = 1;
	console_driver.type = TTY_DRIVER_TYPE_SERIAL;
	console_driver.init_termios = tty_std_termios;
	console_driver.init_termios.c_lflag |= ISIG | ICANON | ECHO 
		 | ECHOE | ECHOK ;
	console_driver.init_termios.c_iflag |= ICRNL;
	console_driver.init_termios.c_cflag = CS8 | CREAD | CLOCAL;
	console_driver.flags = TTY_DRIVER_REAL_RAW;
	console_driver.refcount = &console_refcount;
	
	console_driver.table = console_table;
	console_driver.termios = console_termios;
	console_driver.termios_locked = console_termios_locked;
	
	console_driver.open = con_open;
	console_driver.write = con_write;
	console_driver.put_char = con_putchar;
	console_driver.write_room = con_writeroom;
	console_driver.chars_in_buffer = con_chars_in_buffer;

	if (tty_register_driver(&console_driver)) {
		printk("Couldn't register console driver\n");
	}
	
	/*
	 * Enable receive interrupts
	 */
	*SER16552_IER = IER_RCV_ENABLE;

}
