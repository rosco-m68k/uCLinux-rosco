/*
 * linux/arch/i960/kernel/setup.c
 *
 * Copyright (C) 1999 Keith Adams <kma@cse.ogi.edu>,
 *                    Erik Walthinsen <omega@cse.ogi.edu>
 *
 * Copied/hacked from code by:
 *
 * Copyright (C) 1998  D. Jeff Dionne <jeff@.lineo.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>
 * Copyright (C) 2000 Lineo, Inc.  (www.lineo.com) 
 *
 * linux/arch/m68k/kernel/setup.c
 *
 * Copyright (C) 1995  Hamish Macdonald
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
#include <asm/machdep.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

#ifdef CONFIG_IQ80303
#warning Find a better way to to this!
#undef DEBUG
#undef KERNEL_COMMANDLINE "root=/dev/nfs nfsroot=192.168.0.7:/usr/local/export/hurricaneii nfsaddrs=192.168.0.22:192.168.0.7:192.168.0.76:255.255.255.0:hurricaneii:eth0:none"
#endif

extern int end;
/* FIXME this needs to be looked into */
unsigned long availmem = 1024*1024;

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;
char command_line[512];
char saved_command_line[512];

/* setup some dummy routines */
static void dummy_waitbut(void)
{
}

/* machine dependent keyboard functions */
int (*mach_keyb_init) (void);
int (*mach_kbdrate) (struct kbd_repeat *) = NULL;
void (*mach_kbd_leds) (unsigned int) = NULL;
/* machine dependent irq functions */
void (*mach_init_IRQ) (void);
void (*(*mach_default_handler)[]) (int, void *, struct pt_regs *) = NULL;
int (*mach_request_irq) (unsigned int, void (*)(int, void *, struct pt_regs *),
                         unsigned long, const char *, void *);
int (*mach_free_irq) (unsigned int, void *);
void (*mach_enable_irq) (unsigned int) = NULL;
void (*mach_disable_irq) (unsigned int) = NULL;
int (*mach_get_irq_list) (char *) = NULL;
void (*mach_process_int) (int, struct pt_regs *) = NULL;
/* machine dependent timer functions */
unsigned long (*mach_gettimeoffset) (void);
void (*mach_gettod) (int*, int*, int*, int*, int*, int*);
int (*mach_hwclk) (int, struct hwclk_time*) = NULL;
int (*mach_set_clock_mmss) (unsigned long) = NULL;
void (*mach_mksound)( unsigned int count, unsigned int ticks );
void (*mach_reset)( void );
void (*waitbut)(void) = dummy_waitbut;
void (*mach_debug_init)(void);

extern void register_console(void (*proc)(const char *));

#define MASK_256K 0xfffc0000

#ifdef CONFIG_I960VH
#	define CPU "i960Vh"
#endif
#ifdef CONFIG_I960RX
#	define CPU "i960Rx"
#endif
#ifdef CONFIG_I960HX
#	define CPU "i960Hx"
#endif
#ifdef CONFIG_I960CX
#	define CPU "i960Cx"
#endif
#ifdef CONFIG_I960JX
#	define CPU "i960Jx"
#endif
#ifdef CONFIG_I960JF
#	define CPU "i960JF"
#endif
#ifdef CONFIG_I960RM
#	define CPU "i960RM"
#endif
#ifdef CONFIG_I960RN
#	define CPU "i960RN"
#endif
#ifdef CONFIG_80303
#	define CPU "Intel 80303"
#endif



/* set up for i960 */
void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern	int stackbase, _etext, _edata, _end;
	extern void mon960_console_init(void);

#ifdef CONFIG_MON960_CONSOLE
	extern void console_print_mon960(const char* msg);
	
	register_console(console_print_mon960);
	mon960_console_init();
#endif

	printk("\r\n\nuClinux " CPU "\n");
	printk("i960 port (C) 1999 Keith Adams, Erik Walthinsen, Oregon Graduate Institute\n");
#ifdef CONFIG_SX6
	printk(" modified for Promise Technology SX6 board (C) 2002 Fred Finster fredf@aol.com  \n");
#endif
#ifdef CONFIG_80303
    printk("80303 Support by Martin Proulx. (C) 2002 Okiok Data Ltd.\n");
#endif
	printk("Flat model support (C) 1998 Kenneth Albanowski, D. Jeff Dionne, TSHG Ltd.\n");

	memory_start = PAGE_ALIGN(((unsigned long)&_end));

	init_task.mm->start_code = 0;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;

	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);

#ifdef KERNEL_COMMANDLINE
    strcpy(command_line, KERNEL_COMMANDLINE);
	strcpy(saved_command_line, command_line);
    *cmdline_p = command_line;
#else
	command_line[512-1] = '\0';
	
	if (memcmp(command_line, "Arg!", 4))
		command_line[4] = '\0';
	
	memset(command_line, 0, 4);

	strcpy(saved_command_line, command_line+4);
	*cmdline_p = command_line+4;
#endif /* KERNEL_COMMANDLINE */
	
#ifdef DEBUG
	if (strlen(*cmdline_p)) 
		printk("Command line: '%s'\n", *cmdline_p);
#endif

	*memory_start_p = memory_start;
	*memory_end_p = (unsigned long)&stackbase;
}

/* set up for i960 */
int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq, clockfactor;

    cpu = CPU;
    mmu = "none";
    fpu = "none";
    clockfactor = 16;

    clockfreq = loops_per_sec*clockfactor;

    return(sprintf(buffer, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/1000000,(clockfreq/100000)%10,
		   loops_per_sec/500000,(loops_per_sec/5000)%100,
		   loops_per_sec));

}

void arch_gettod(int *year, int *mon, int *day, int *hour,
		 int *min, int *sec)
{
	*year = *mon = *day = *hour = *min = *sec = 0;
}
