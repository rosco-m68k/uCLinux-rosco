/* 
 * lc7981.c: Driver for 7981 based graphical display
 *
 * Copyright (C) 2003  Gerold Boehler <gboehler@mail.austria.at>
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>

#define	DATA	(*(volatile unsigned char *)0x400000)
#define INST	(*(volatile unsigned char *)0x400001)
#define CHARS	(18+2)
#define LINES	16

static short cpos = 0x012c;	// start of cursor
static short dpos = 0x0000;	// start of display

static void init_lc7981(void);
static void putbus(char, char);

static int initialized = 0;

static void shift_window(void) {
	int i;

	for ( i = 0; i < CHARS; i++) 
		putbus(0x0c, 0);

	putbus(0x0a, (char)cpos);
	putbus(0x0b, (char)(cpos >> 8));

	dpos += CHARS;
	putbus(0x08, (char)(dpos));		// display address low
	putbus(0x09, (char)(dpos >> 8));	// display address high
}

static void putbus(char instr, char data) {
	INST = instr;
	DATA = data;
}

static int lc7981_write(struct inode *inode, struct file *file,
			const char *buffer, int count) {
	int i;

	for(i = 0; i < count && *buffer; i++) {
		switch( *buffer ) {

			case '\r': continue;

			case '\n': {
				int diff = CHARS - cpos % CHARS ;
				buffer++;
				if (diff == CHARS) continue;	// no double newlines

				cpos += diff;
				putbus(0x0a, (char)cpos);
				putbus(0x0b, (char)(cpos >> 8));

				shift_window();
				break;

			default: 
				cpos++;
				putbus(0x0c, *buffer++);

				if ( !(cpos % CHARS) ) 
					shift_window();
			}
		}
	}

	return count;
}

static struct file_operations lc7981_fops = {
	NULL,
	NULL,
	lc7981_write,
	NULL
};

int lc7981_init(void) {

	if(!initialized) init_lc7981();

	if ( register_chrdev(10, "/dev/lcd", &lc7981_fops) < 0) {
		printk("error registering lcd driver");
		return -1;
	}

	return 0;
}

void lc7981_print(const char *c) {

	if (!initialized) init_lc7981();
	
	lc7981_write(NULL, NULL, c, strlen(c));

}

static void init_lc7981(void) {
	int i;

	initialized++;

	/* chip select at 0x400000, 4kb */
	*(volatile unsigned short *)0xfffa60 = 0x4000;
	*(volatile unsigned short *)0xfffa62 = 0x7c30;

	putbus(0x00, 0x30);			// character mode
	putbus(0x01, 0x75 );			// character pitch v = 8, h = 6
	putbus(0x02, CHARS-2);			// characters per line = 18 (must be even)
	putbus(0x03, 0x7f);			// 128 dots
	putbus(0x04, 0x00);			// cursor position
	putbus(0x08, (char)(dpos));		// display start address low
	putbus(0x09, (char)(dpos >> 8));	// display start address high
	putbus(0x0a, 0x00);			// cursor address low
	putbus(0x0b, 0x00);			// cursor address high

	for(i = 0; i < 2048; i++)
		putbus(0x0c, 0);

	putbus(0x0a, (char)cpos);
	putbus(0x0b, (char)(cpos >> 8));
}
