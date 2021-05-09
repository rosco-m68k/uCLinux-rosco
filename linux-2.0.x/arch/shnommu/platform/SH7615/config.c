/*
 *  linux/arch/$(ARCH)/platform/$(PLATFORM)/config.c
 *
 *  Copyright (C) 1993 Hamish Macdonald
 *  Copyright (C) 1999 D. Jeff Dionne
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <stdarg.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/console.h>

#include <asm/setup.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/irq.h>
#include <asm/machdep.h>

#include <asm/SH7615.h>
#define   IRQ_MACHSPEC   0 /*Posh2 used for compilation have to modify*/
extern void register_console(void (*proc)(const char *));
extern void SH7615_init_trap(void);

void BSP_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{
	/*
	 * posh2 - removing the source  lines- Need to add
	 * the exact source lines for posh2  -- from Binoj
	 */

  /*ENABLING TIMER INTEVAL */

 //WTCSR_CNT=CLOCK_SET|TIMER_ENABLE|OVERFLOW_CLEAR;
 //WTCSR_CNT=COUNT_VALUE ;/*1/(CLK/1024)*FF-11=TIMEINTERVAL*/
 /* Using FRT for more precision */
 
 request_irq(6, timer_routine, IRQ_FLG_LOCK, "timer", NULL);
 TCR  |=INIT_TCR;
 TOCR |=INIT_TOCR;
 FTCSR|=INIT_FTCSR;
 
 FRC_H = INIT_FRC_H;
 FRC_L = INIT_FRC_L;
 OCRA_H = INIT_OCRA_H;
 OCRA_L = INIT_OCRA_L;
 TIER |= INIT_TIER;
 
}

void BSP_tick(void)
{
	/*
	 * posh2 - removing the source  lines- Need to add
	 * the exact source lines for posh2 -- from Binoj
	 */
	

	/* WTCSR_CNT=TIMER_DISABLE;
	 WTCSR_CNT=COUNT_VALUE;*/

}

unsigned long BSP_gettimeoffset (void)
{
	/*
	 */
	return 0;
}

void BSP_gettod (int *yearp, int *monp, int *dayp,
		   int *hourp, int *minp, int *secp)
{
}

int BSP_hwclk(int op, struct hwclk_time *t)
{
	return 0;
}

int BSP_set_clock_mmss (unsigned long nowtime)
{
	return 0;
}

void BSP_reset (void)
{
	/*
	 * posh2 - removing the source  lines- Need to add
	 * the exact source lines for posh2 _nucleus 
	 */
}

unsigned char *cs8900a_hwaddr;
static int errno;

#ifdef CONFIG_UCSIMM
/*
 * posh2 - Commenting out these lines
 */
 
/*
_bsc0(char *, getserialnum)
_bsc1(unsigned char *, gethwaddr, int, a)
_bsc1(char *, getbenv, char *, a)
*/
#endif

void config_BSP(char *command, int len)
{
	/*
	 * posh2 - removing the source  lines- Need to add
	 * the exact source lines for posh2
	 */
	 char *buf = "Initialising the serial port"; 
	 extern void  console_print_SH7615(char * b);
	 extern	void  console_init_SH7615();

	 //strcpy( buf, (const char *) "Initializing Serial Port"); 
	 console_init_SH7615();
  	 register_console(console_print_SH7615);

	// printk("initializing serial port");
	// console_print_SH7615((char *)"Initializing serial port");
	 
	 mach_trap_init	      = SH7615_init_trap;
         mach_sched_init      = BSP_sched_init;
	 mach_tick            = BSP_tick;
    	 mach_gettimeoffset   = BSP_gettimeoffset;
      	 mach_gettod          = BSP_gettod;
         mach_hwclk           = NULL;
	 mach_set_clock_mmss  = NULL;
	 mach_mksound         = NULL;
	 mach_reset           = BSP_reset;
	 mach_debug_init      = NULL;
		 
}
