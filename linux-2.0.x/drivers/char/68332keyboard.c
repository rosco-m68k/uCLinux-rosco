/*
 * linux/drivers/char/68332keyboard.c
 * quite ugly but no time at the moment...
 *
 * Copyright Gerold Boehler <gboehler@mail.austria.at>
 */

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/keyboard.h>
#include <linux/fs.h>

#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/machdep.h>

#include <asm/io.h>
#include <asm/system.h>

#include <asm/MC68332.h>

#define CLOCK	(PORTF & 0x40)
#define DATA	(PORTF & 0x20)
#define BUFLEN	32

static void kbd_int(int, void*, struct pt_regs*);
int kbd_init(void);

static char prev = 0;

static char buf[BUFLEN];
static int in = 0, out = 0;

static int is_empty(void) {
	return in == out;
}

static int is_full(void) {
	return (in+1) % BUFLEN == out;
}

static char get_ascii(unsigned char code) {
	switch(code) {
		case 0x16: return '1';
		case 0x1e: return '2';
		case 0x26: return '3'; 
		case 0x25: return '4';
		case 0x2e: return '5';
		case 0x36: return '6';
		case 0x3d: return '7';
		case 0x3e: return '8';
		case 0x46: return '9';
		case 0x45: return '0';
		
		case 0x15: return 'q';
		case 0x1d: return 'w';
		case 0x24: return 'e';
		case 0x2d: return 'r';
		case 0x2c: return 't';
		case 0x35: return 'z';
		case 0x3c: return 'u';
		case 0x43: return 'i';
		case 0x44: return 'o';
		case 0x4d: return 'p';
		case 0x1c: return 'a';
		case 0x1b: return 's';
		case 0x23: return 'd';
		case 0x2b: return 'f';
		case 0x34: return 'g';
		case 0x33: return 'h';
		case 0x3b: return 'j';
		case 0x42: return 'k';
		case 0x4b: return 'l';
		case 0x1a: return 'y';
		case 0x22: return 'x';
		case 0x21: return 'c';
		case 0x2a: return 'v';
		case 0x32: return 'b';
		case 0x31: return 'n';
		case 0x3a: return 'm';
		case 0x29: return ' ';

		case 0x5a: return '\n';

		default: return 0;
		
	}
}

static char get_ASCII(unsigned char code) {
	char ascii = get_ascii(code);
	if (ascii >= '0' && ascii <= '9') {
		switch(ascii) {
			case '0': return '=';
			case '1': return '!';
			case '2': return '"';
			case '3': return '§';
			case '4': return '$';
			case '5': return '%';
			case '6': return '&';
			case '7': return '/';
			case '8': return '(';
			case '9': return ')';
		}
	}
	else if (ascii >= 'a' && ascii <= 'z' || ascii >= 'A' && ascii <= 'Z')
		return ascii -32;

	else
		return ascii;

}

static void keyboard_start() {
	PORTF &= ~0x20;
	DDRF &= ~0x20;
	PFPAR |= 0x20;
}

static void keyboard_stop() {
	DDRF |= 0x20;
	PFPAR &= ~0x20;
	PORTF |= 0x20;
}

static inline void busywait(void) {
	while(!CLOCK);
	while(CLOCK);
}

static int kbd_read(struct inode *inode, struct file *file, char *buffer, int count) {
	int cnt = count;
	if (is_empty()) { wait_for_keypress(); }
	while(cnt && !is_empty()) {
		*buffer++ = buf[out];
		out = (out+1) % BUFLEN;
		cnt--;
	}

	return count - cnt;
}

static char read_code(void) {
	unsigned char scancode = 0;	
	int i;

	for(i = 0; i <= 8; i++) {
		busywait();
		scancode |= ((DATA >> 5) & 0x01 ) << i;
	}

	busywait();

	return scancode;

}

static void kbd_int(int irq, void *dummy, struct pt_regs *regs) {
	unsigned long flags;
	unsigned char scancode = 0;
	int i;

	static int upper = 0;

	cli();
	save_flags(flags);

	scancode = read_code();

	keyboard_stop();

	if (scancode == 0xf0 || scancode == 0xe0) {
		scancode = read_code();

	/* only capslock style at the moment */
	} else if ( scancode == 0x12 || scancode == 0x59) {
		upper = upper ? 0 : 1;
	}

	if(get_ascii(scancode) &&  !is_full()) {
		buf[in] = upper ? get_ASCII(scancode) : get_ascii(scancode);
		in = (in+1) % BUFLEN;
		wake_up(&keypress_wait);
	}

	keyboard_start();
	restore_flags(flags);
}

static struct file_operations kbd_fops = {
	NULL,
	kbd_read
};

/*
 * External int 29 is used for startbit, portf=0x40 is used for clock
 */

int kbd_init(void) {

	if (request_irq(IRQ_MACHSPEC | 29, kbd_int, IRQ_FLG_STD, "keyboard", NULL)) {
		panic("Unable to attach keyboard interrupt!\n");
		return -1;
	}

	if (register_chrdev(11, "kbd", &kbd_fops))
		panic("Couldn't register keyboard!\n");

	/* enable external interrupts */
	DDRE &= ~0x4;
	PEPAR |= 0x4;

	keyboard_start();
#if 0
	/* setup clock port */
	DDRF &= ~0x20;
	PFPAR |= 0x20;
#endif
	/* setup data port */
	DDRF &= ~0x40;
	PFPAR &= ~0x40;

	return 0;
}


