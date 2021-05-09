/*
 *  linux/arch/h8300/kernel/setup.c
 *
 *  Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 *  Based on linux/arch/m68knommu/kernel/setup.c
 *
 *  Copyleft  ()) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1999       Greg Ungerer (gerg@moreton.com.au)
 *  Copyright (C) 1998,1999  D. Jeff Dionne <jeff@rt-control.com>
 *  Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 *  Copyright (C) 1995       Hamish Macdonald
 */

/*
 * This file handles the architecture-dependent parts of system setup
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/major.h>

#include <asm/setup.h>
#include <asm/irq.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

extern int console_loglevel;
#ifdef CONFIG_ROMKERNEL
extern void *_vector,*_erom;
#endif

/*  conswitchp               = &fb_con;*/
#ifdef CONFIG_CONSOLE
extern struct consw *conswitchp;
#endif

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;

extern char _command_line[512];
char saved_command_line[512];
char console_tty[16]="/dev/ttyS0";

static void parse_commandline(char *cmdline)
{
	while(*cmdline) {
		if (((*cmdline & 0xdf) == 'C') &&
		    ((strncmp(cmdline,"CONSOLE=",8) == 0) ||
                     (strncmp(cmdline,"console=",8) == 0))) {
			int cnt;
			char *dev=console_tty;
			cmdline += 8;
			while (*cmdline == ' ')
				cmdline++ ;
			for (cnt = 0; cnt < sizeof(console_tty)-1; cnt++) {
				if (*cmdline == '\0') {
					break ;
				}
				*dev++ = *cmdline++ ;
			}
			if (cnt>0) 
				*dev = '\0';
			break ;
		}
	}
}

void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _stext, _etext;
	extern int _sdata, _edata;
	extern int _sbss, _ebss, _end;
	extern int _ramstart, _ramend;

	memory_start = &_end + 512;
	memory_end = &_ramend - 4096; /* <- stack area */

	printk("\x0F\r\n\nuClinux for H8/300H\n");
	printk("H8/300H Porting by Yoshinori Sato\n");
	printk("Flat model support (C) 1998,1999 Kenneth Albanowski, D. Jeff Dionne\n");
#ifdef DEBUG
	printk("KERNEL -> TEXT=0x%06x-0x%06x DATA=0x%06x-0x%06x "
		"BSS=0x%06x-0x%06x\n", (int) &_stext, (int) &_etext,
		(int) &_sdata, (int) &_edata,
		(int) &_sbss, (int) &_ebss);
	printk("KERNEL -> ROMFS=0x%06x-0x%06x MEM=0x%06x-0x%06x "
		"STACK=0x%06x-0x%06x\n",
	       (int) &_ebss, (int) memory_start,
		(int) memory_start, (int) memory_end,
		(int) memory_end, (int) &_ramend);
#endif
	init_task.mm->start_code = (unsigned long) &_stext;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;

	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);

	/* Keep a copy of command line */
	*cmdline_p = _command_line;

#ifdef DEBUG
	if (strlen(*cmdline_p)) 
		printk("Command line: '%s'\n", *cmdline_p);
	else
		printk("No Command line passed\n");
#endif
	parse_commandline(*cmdline_p);

	*memory_start_p = memory_start;

	*memory_end_p = memory_end;
#ifdef CONFIG_ROMKERNEL
	rom_length = (unsigned long)&_erom - (unsigned long)&_vector;
#endif	
#ifdef CONFIG_CONSOLE
	conswitchp = 0;
#endif
  
#ifdef DEBUG
	printk("Done setup_arch\n");
#endif

}

int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;

    cpu = "H8/300H";
    mmu = "none";
    fpu = "none";

    return(sprintf(buffer, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   loops_per_sec/500000,(loops_per_sec/5000)%100,
		   loops_per_sec));

}
