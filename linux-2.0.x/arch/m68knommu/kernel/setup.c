/*
 *  linux/arch/m68knommu/kernel/setup.c
 *
 *  Copyright (C) 2000       Lineo Inc. (www.lineo.com)
 *  Copyleft  ()) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1999-2002  Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 1998,1999  D. Jeff Dionne <jeff@lineo.ca>
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
#include <linux/ctype.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/machdep.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

#ifdef CONFIG_M68332
#include <asm/MC68332.h>
#endif

extern void register_console(void (*proc)(const char *));
/*  conswitchp               = &fb_con;*/
#ifdef CONFIG_CONSOLE
extern struct consw *conswitchp;
#ifdef CONFIG_FRAMEBUFFER
extern struct consw fb_con;
#endif
#endif

#if defined (CONFIG_M68360)
#include <asm/quicc_cpm.h>
extern void m360_cpm_reset(void);

#endif

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;

#define COMMAND_LINE_LENGTH 512
#define CONSOLE_DEVICE_LENGTH 32

char command_line[COMMAND_LINE_LENGTH];
char saved_command_line[COMMAND_LINE_LENGTH];
char console_device[CONSOLE_DEVICE_LENGTH];

char console_default[] = "CONSOLE=/dev/ttyS0";

/* setup some dummy routines */
static void dummy_waitbut(void)
{
}

void (*mach_sched_init) (void (*handler)(int, void *, struct pt_regs *)) = NULL;
void (*mach_tick)( void ) = NULL;
/* machine dependent keyboard functions */
int (*mach_keyb_init) (void) = NULL;
int (*mach_kbdrate) (struct kbd_repeat *) = NULL;
void (*mach_kbd_leds) (unsigned int) = NULL;
/* machine dependent irq functions */
void (*mach_init_IRQ) (void) = NULL;
void (*(*mach_default_handler)[]) (int, void *, struct pt_regs *) = NULL;
int (*mach_request_irq) (unsigned int, void (*)(int, void *, struct pt_regs *),
                         unsigned long, const char *, void *);
int (*mach_free_irq) (unsigned int, void *);
void (*mach_enable_irq) (unsigned int) = NULL;
void (*mach_disable_irq) (unsigned int) = NULL;
int (*mach_get_irq_list) (char *) = NULL;
int (*mach_process_int) (int, struct pt_regs *) = NULL;
void (*mach_trap_init) (void);
/* machine dependent timer functions */
unsigned long (*mach_gettimeoffset) (void) = NULL;
void (*mach_gettod) (int*, int*, int*, int*, int*, int*) = NULL;
int (*mach_hwclk) (int, struct hwclk_time*) = NULL;
int (*mach_set_clock_mmss) (unsigned long) = NULL;
void (*mach_mksound)( unsigned int count, unsigned int ticks ) = NULL;
void (*mach_reset)( void ) = NULL;
void (*waitbut)(void) = dummy_waitbut;
void (*mach_debug_init)(void) = NULL;

/* Other prototypes */
#ifdef CONFIG_DEV_FLASH
void flash_probe(void);
#endif

#ifdef CONFIG_M68000
	#define CPU "MC68000"
#endif
#ifdef CONFIG_M68328
	#define CPU "MC68328"
#endif
#ifdef CONFIG_M68EZ328
	#define CPU "MC68EZ328"
#endif
#ifdef CONFIG_M68332
	#define CPU "MC68332"
#endif
#ifdef CONFIG_M68360
	#define CPU "MC68360"
#endif
#ifdef CONFIG_M68376
	#define CPU "MC68376"
#endif
#if defined(CONFIG_M5206)
	#define	CPU "COLDFIRE(m5206)"
#endif
#if defined(CONFIG_M5206e)
	#define	CPU "COLDFIRE(m5206e)"
#endif
#if defined(CONFIG_M5249)
	#define	CPU "COLDFIRE(m5249)"
#endif
#if defined(CONFIG_M5272)
	#define	CPU "COLDFIRE(m5272)"
#endif
#if defined(CONFIG_M5307)
	#define	CPU "COLDFIRE(m5307)"
#endif
#if defined(CONFIG_M5407)
	#define	CPU "COLDFIRE(m5407)"
#endif
#ifndef CPU
	#define	CPU "UNKOWN"
#endif


/*
 *	Setup the console. There may be a console defined in
 *	the command line arguments, so we look there first. If the
 *	command line arguments don't exist then we default to the
 *	first serial port. If we are going to use a console then
 *	setup its device name and the UART for it.
 */

void setup_console(void)
{
#if defined(CONFIG_COLDFIRE) || defined(CONFIG_M68360)
	char *sp, *cp;
	int i;
#ifdef CONFIG_COLDFIRE
	extern void rs_console_init(void);
	extern void rs_console_print(const char *b);
	extern int rs_console_setup(char *arg);
#endif

	/* Quickly check if any command line arguments for CONSOLE. */
	for (sp = NULL, i = 0; i < sizeof(command_line) - 8; i++) {
		if (command_line[i] == 0)
			break;
		if (command_line[i] == 'C' || command_line[i] == 'c') {
			if (strncmp(&command_line[i], "CONSOLE=", 8) == 0 ||
					strncmp(&command_line[i], "console=", 8) == 0) {
				sp = &command_line[i + 8];
				/* break; dont break so we find the last console= entry */
			}
		}
	}

	/* If no CONSOLE defined then default one */
	if (sp == NULL) {
		/* make sure init gets a copy, tack it on the end */
		if (sizeof(command_line) - strlen(command_line) >
				strlen(console_default) + 2) {
			strcat(command_line, " ");
			strcat(command_line, console_default);
		}
		sp = console_default + 8;
	}

	cp = sp; /* save a copy */

	/* If console specified then copy it into name buffer */
	for (i = 0; i < sizeof(console_device) - 1; i++) {
		if (*sp == '\0' || isspace(*sp) || *sp == ',')
			break;
		console_device[i] = *sp++;
	}
	console_device[i] = 0;

	/* If a serial console then init it */
#ifdef CONFIG_COLDFIRE
	if (rs_console_setup(cp)) {
		rs_console_init();
		register_console(rs_console_print);
	}
#endif
#ifdef CONFIG_M68360
	if (rs_quicc_console_setup(cp))
	{
		console_print_function_t print_function = rs_quicc_console_init();

		if (print_function)
			register_console(print_function);
		else
			while(1);
	}
#endif
#endif /* CONFIG_COLDFIRE || M68360 */
#if defined(CONFIG_68332_SERIAL)
	extern void console_print_68332(const char *b);
		register_console(console_print_68332);
#endif
}

#if defined( CONFIG_TELOS) || defined( CONFIG_UCSIMM ) || (defined( CONFIG_PILOT ) && defined( CONFIG_M68328 ))
#define CAT_ROMARRAY
#endif

void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _stext, _etext;
#ifdef DEBUG
	extern int _sdata;
	extern int _sbss, _ebss;
#endif
	extern int _edata, _end;
	extern int _ramstart, _ramend;

#ifdef CAT_ROMARRAY
	extern int __data_rom_start;
	extern int __data_start;
	int *romarray = (int *)((int) &__data_rom_start +
				      (int)&_edata - (int)&__data_start);
#endif

#if defined(CONFIG_CHR_DEV_FLASH) || defined(CONFIG_BLK_DEV_FLASH)
	/* we need to initialize the Flashrom device here since we might
	 * do things with flash early on in the boot
	 */
	flash_probe();
#endif

#ifdef CONFIG_COLDFIRE
	memory_start = _ramstart;
	memory_end = _ramend - 4096; /* <- stack area */
#else
 #if defined(CONFIG_PILOT) && defined(CONFIG_M68EZ328) && \
 		!defined(CONFIG_PILOT_INCROMFS)
	memory_start = _ramstart;
 #else
	memory_start = &_end;
 #endif
	memory_end = &_ramend - 4096; /* <- stack area */
#endif /* CONFIG_COLDFIRE */

#ifdef CONFIG_MWI
	memory_start = _ramstart;
	memory_end = _ramend - 0x400;
#endif

#if defined (CONFIG_M68360)
	if (quicc_cpm_init())
		while(1);
#endif

	config_BSP(&command_line[0], sizeof(command_line));

	setup_console();
	printk("\x0F\r\n\nuClinux/" CPU "\n");

#if defined( CONFIG_M68360 )
	printk("uCquicc support by Lineo Inc. <mleslie@lineo.com>\n");
#endif

#ifdef CONFIG_COLDFIRE
	printk("COLDFIRE port done by Greg Ungerer, gerg@snapgear.com\n");
#ifdef CONFIG_M5307
	printk("Modified for M5307 by Dave Miller, dmiller@intellistor.com\n");
#endif
#ifdef CONFIG_ELITE
	printk("Modified for M5206eLITE by Rob Scott, rscott@mtrob.fdns.net\n");
#endif  
#ifdef CONFIG_TELOS
	printk("Modified for Omnia ToolVox by James D. Schettine, james@telos-systems.com\n");
#endif
#endif
	printk("Flat model support (C) 1998,1999 Kenneth Albanowski, D. Jeff Dionne\n");

#if defined( CONFIG_PILOT ) && defined( CONFIG_M68328 )
	printk("TRG SuperPilot FLASH card support <info@trgnet.com>\n");
#endif

#if defined( CONFIG_PILOT ) && defined( CONFIG_M68EZ328 )
	printk("PalmV support by Lineo Inc. <jeff@uClinux.com>\n");
#endif

#ifdef CONFIG_M68EZ328ADS
	printk("M68EZ328ADS board support (C) 1999 Vladimir Gurevich <vgurevic@cisco.com>\n");
#endif

#ifdef CONFIG_ALMA_ANS
	printk("Alma Electronics board support (C) 1999 Vladimir Gurevich <vgurevic@cisco.com>\n");
#endif

#ifdef CONFIG_CWEZ328
	printk("Cwlinux cwez328 board support (C) 2002 Andrew Ip <aip@cwlinux.com>\n");
#endif
#ifdef CONFIG_MWI
	printk("Mini Web Interface T10 Board Support (C) 2003 Gerold Boehler <gboehler@mail.austria.at>\n");
#endif


#ifdef DEBUG
	printk("KERNEL -> TEXT=0x%06x-0x%06x DATA=0x%06x-0x%06x "
		"BSS=0x%06x-0x%06x\n", (int) &_stext, (int) &_etext,
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
		(int) memory_end,
			((unsigned) &_ramend > (unsigned) &_sdata &&
				(unsigned) &_ramend < (unsigned) &_end) ?
				(unsigned) _ramend : (unsigned) &_ramend);
#endif

	init_task.mm->start_code = (unsigned long) &_stext;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;

#ifdef CONFIG_BLK_DEV_BLKMEM
	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);
#endif
#ifdef CONFIG_BLK_DEV_INITRD
	ROOT_DEV = MKDEV(RAMDISK_MAJOR,0);
#endif

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
	/*rom_length = (unsigned long)&_flashend - (unsigned long)&_romvec;*/
	
#ifdef CONFIG_CONSOLE
#ifdef CONFIG_FRAMEBUFFER
	conswitchp = &fb_con;
#else
	conswitchp = 0;
#endif
#endif

#ifdef DEBUG
	printk("Done setup_arch\n");
#endif

}

int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq;

    cpu = CPU;
    mmu = "none";
    fpu = "none";

#ifdef CONFIG_COLDFIRE
    clockfreq = loops_per_sec*3;
#else
    clockfreq = loops_per_sec*16;
#endif

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
	if (mach_gettod)
		mach_gettod(year, mon, day, hour, min, sec);
}

