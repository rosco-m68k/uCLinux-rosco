/*
 *  linux/arch/m68knommu/kernel/setup.c
 *
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

#include <asm/shglports.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/machdep.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

extern void register_console(void (*proc)(const char *));
/*  conswitchp               = &fb_con;*/
#ifdef CONFIG_CONSOLE
extern struct consw *conswitchp;
#ifdef CONFIG_FRAMEBUFFER
extern struct consw fb_con;
#endif
#endif
  
unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;
char command_line[512];
char saved_command_line[512];

/* setup some dummy routines */
static void dummy_waitbut(void)
{
}

void (*mach_sched_init) (void (*handler)(int, void *, struct pt_regs *));
/* machine dependent keyboard functions */
int (*mach_keyb_init) (void);
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
void (*mach_process_int) (int, struct pt_regs *) = NULL;
/* machine dependent timer functions */
unsigned long (*mach_gettimeoffset) (void);
void (*mach_gettod) (int*, int*, int*, int*, int*, int*);
int (*mach_hwclk) (int, struct hwclk_time*) = NULL;
int (*mach_set_clock_mmss) (unsigned long) = NULL;
void (*mach_mksound)( unsigned int count, unsigned int ticks );
void (*mach_reset)( void );
void (*waitbut)(void) = dummy_waitbut;
void (*mach_debug_init)(void) = NULL;
void (*mach_trap_init) (void) = NULL;
void (*mach_tick)( void ) = NULL;

extern void register_console(void (*proc)(const char *));

#define MASK_256K 0xfffc0000

#ifdef CONFIG_M68000
	#define CPU "68000"
#endif
#ifdef CONFIG_M68332
	#define CPU "68332"
#endif

void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _etext, _edata, _end, _ramend, _romvec, _prgram;
	
#ifdef CONFIG_68332_SERIAL
       extern void console_print_68332(const char * b);
	register_console(console_print_68332);
#endif

	printk("\x0F\r\n\nuClinux/MC" CPU "\n");
	printk("Flat model support (C) 1998,1999 Kenneth Albanowski, D. Jeff Dionne\n");
	memory_start = &_end;

	init_task.mm->start_code = 0;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;

	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);

	command_line[512-1] = '\0';
	
	if (memcmp(command_line, "Arg!", 4))
		command_line[4] = '\0';
	
	memset(command_line, 0, 4);

	strcpy(saved_command_line, command_line+4);
	*cmdline_p = command_line+4;
	
	
#ifdef DEBUG
	if (strlen(*cmdline_p)) 
		printk("Command line: '%s'\n", *cmdline_p);
#endif

	*memory_start_p = memory_start;
	*memory_end_p = memory_end = (unsigned long)&_ramend - 0x10000;
//	rom_length = (unsigned long)&_flashend - (unsigned long)&_romvec;

#ifdef CONFIG_CONSOLE
#ifdef CONFIG_FRAMEBUFFER
	conswitchp = &fb_con;
#else
	conswitchp = 0;
#endif
#endif
}

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
