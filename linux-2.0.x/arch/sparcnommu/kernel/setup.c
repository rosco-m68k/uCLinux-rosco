/*
 *  linux/arch/sparcnommu/kernel/setup.c
 *
 *  Copyleft  (C) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1999-2002  Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 1998,2000  D. Jeff Dionne <jeff@lineo.ca>
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

#include <asm/irq.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif


extern void register_console(void (*proc)(const char *));
extern void console_print_LEON(const char * b);

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;

char command_line[512];
char saved_command_line[512];
char console_device[16];

char console_default[] = "CONSOLE=/dev/ttyS0";

#define CPU "LEON/Sparc2.1"


/*
 *	Setup the console. There may be a console defined in
 *	the command line arguments, so we look there first. If the
 *	command line arguments don't exist then we default to the
 *	first serial port. If we are going to use a console then
 *	setup its device name and the UART for it.
 */
void setup_console(void)
{
#ifdef CONFIG_COLDFIRE
/* This is a great idea.... Until we get support for it across all
   supported m68k platforms, we limit it to ColdFire... DJD */
	char *sp, *cp;
	int i;
	extern void rs_console_init(void);
	extern void rs_console_print(const char *b);
	extern int rs_console_setup(char *arg);

	/* Quickly check if any command line arguments for CONSOLE. */
	for (sp = NULL, i = 0; (i < sizeof(command_line)-8); i++) {
		if (command_line[i] == 0)
			break;
		if (command_line[i] == 'C') {
			if (!strncmp(&command_line[i], "CONSOLE=", 8)) {
				sp = &command_line[i + 8];
				break;
			}
		}
	}

	/* If no CONSOLE defined then default one */
	if (sp == NULL) {
		strcpy(command_line, console_default);
		sp = command_line + 8;
	}

	/* If console specified then copy it into name buffer */
	for (cp = sp, i = 0; (i < (sizeof(console_device) - 1)); i++) {
		if ((*sp == 0) || (*sp == ' ') || (*sp == ','))
			break;
		console_device[i] = *sp++;
	}
	console_device[i] = 0;

	/* If a serial console then init it */
	if (rs_console_setup(cp)) {
		rs_console_init();
		register_console(rs_console_print);
	}
#endif /* CONFIG_COLDFIRE */

#ifdef CONFIG_LEON_SERIAL
  register_console(console_print_LEON);
#endif

}

#if defined( CONFIG_LEON_2 )
#define CAT_ROMARRAY
#endif

void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _stext, _etext;
	extern int _sdata, _edata;
	extern int _sbss, _ebss, _end;
	extern int _ramstart, _ramend;

#ifdef CAT_ROMARRAY
	extern int __data_rom_start;
	extern int __data_start;
	int *romarray = (int *)((int) &__data_rom_start +
				      (int)&_edata - (int)&__data_start);
#endif

	memory_start = &_end;
	memory_end = &_ramend - 4096; /* <- stack area */

#if 0
	config_BSP(&command_line[0], sizeof(command_line));
#else
	command_line[0] = 0;
#endif
	setup_console();
	printk("\x0F\r\n\nuClinux/Sparc\n");
	printk("Flat model support (C) 1998-2000 Kenneth Albanowski, D. Jeff Dionne\n");
	printk("LEON-2.1 Sparc V8 support (C) 2000 D. Jeff Dionne, Lineo Inc.\n");
	printk("LEON-2.2/LEON-2.3 Sparc V8 support (C) 2001 The LEOX team <team@leox.org>.\n");
#ifdef DEBUG
	printk("KERNEL -> TEXT=0x%08x-0x%08x DATA=0x%08x-0x%08x "
		"BSS=0x%08x-0x%08x\n", (int) &_stext, (int) &_etext,
		(int) &_sdata, (int) &_edata,
		(int) &_sbss, (int) &_ebss);
	printk("KERNEL -> ROMFS=0x%06x-0x%06x MEM=0x%06x-0x%06x "
		"STACK=0x%06x-0x%06x\n",
#ifdef CAT_ROMARRAY
	       (int) romarray, ((int) romarray) + romarray[2],
#else
	       (int) &_ebss, (int) memory_start,
#endif
		(int) memory_start, (int) memory_end,
		(int) memory_end, (int) &_ramend);
#endif
	init_task.mm->start_code = (unsigned long) &_stext;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;

	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);

	/* Keep a copy of command line */
	*cmdline_p = &command_line[0];

	memcpy(saved_command_line, command_line, sizeof(saved_command_line));
	saved_command_line[sizeof(saved_command_line)-1] = 0;

#ifdef DEBUG
	if (strlen(*cmdline_p)) 
		printk("Command line: '%s'\n", *cmdline_p);
	else
		printk("No Command line passed\n");
#endif
	*memory_start_p = memory_start;
	*memory_end_p = memory_end;

#ifdef DEBUG
	printk("Done setup_arch\n");
#endif

}

int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq, clockfactor;

    cpu = CPU;
    mmu = "none";
    fpu = "none";
    clockfactor = 1;

    clockfreq = loops_per_sec*clockfactor;

    return(sprintf(buffer, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/100000,(clockfreq/100000)%10,
		   loops_per_sec/500000,(loops_per_sec/5000)%100,
		   loops_per_sec));

}

void arch_gettod(int *year, int *mon, int *day, int *hour,
		 int *min, int *sec)
{
		*year = *mon = *day = *hour = *min = *sec = 0;
}

