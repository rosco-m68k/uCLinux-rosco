/*
 *  linux/arch/shnommu/setup.c
 *
 *  File modified on 2108/2002 by Binoj.G.S
 *  
 *
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
char console_device[16];

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
/*void (*mach_init_IRQ) (void) = NULL;
void (*(*mach_default_handler)[]) (int, void *, struct pt_regs *) = NULL;
int (*mach_request_irq) (unsigned int, void (*)(int, void *, struct pt_regs *),
                         unsigned long, const char *, void *);
int (*mach_free_irq) (unsigned int, void *);
void (*mach_enable_irq) (unsigned int) = NULL;
void (*mach_disable_irq) (unsigned int) = NULL;
int (*mach_get_irq_list) (char *) = NULL;
int (*mach_process_int) (int, struct pt_regs *) = NULL;*/
void (*mach_trap_init) (void) =NULL;
/* machine dependent timer functions */
unsigned long (*mach_gettimeoffset) (void) = NULL;
void (*mach_gettod) (int*, int*, int*, int*, int*, int*) = NULL;
int (*mach_hwclk) (int, struct hwclk_time*) = NULL;
int (*mach_set_clock_mmss) (unsigned long) = NULL;
void (*mach_mksound)( unsigned int count, unsigned int ticks ) = NULL;
void (*mach_reset)( void ) = NULL;
void (*waitbut)(void) = dummy_waitbut;
void (*mach_debug_init)(void) = NULL;


#ifdef CONFIG_SH7615
	#define CPU "SH7615"
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
#ifdef CONFIG_SH7615

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
#endif
      }	
 void setup_arch(char **cmdline_p,
		                 unsigned long * memory_start_p, unsigned long * memory_end_p)
  {
	         extern int _stext, _etext;
	         extern int _sdata, _edata;
	         extern int _sbss, _ebss, _end;
	         extern int _ramstart, _ramend;

		 memory_start = &_end + 512;    /*not confident reffered h8 port*/
	         memory_end = &_ramend - 4096; /* <- stack area */
		 config_BSP(&command_line[0], sizeof(command_line));
		 setup_console();/*whether it is required*/

        printk("\r\n uClinux for SH7615H\n");
        printk("\r\n Modified for Hirakawa Hewtech by Ushustech\n");
			
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
	else printk("No command line passed\n");
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

}

int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq, clockfactor;

    cpu ="SH7615";
    mmu = "none";
    fpu = "none";

}

void arch_gettod(int *year, int *mon, int *day, int *hour,
		 int *min, int *sec)
{
	if (mach_gettod)
		mach_gettod(year, mon, day, hour, min, sec);
	else
		*year = *mon = *day = *hour = *min = *sec = 0;
}

