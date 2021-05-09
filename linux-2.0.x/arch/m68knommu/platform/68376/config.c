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
#include <asm/MC68332.h>

extern void register_console(void (*proc)(const char *));

#ifdef CONFIG_68332_SERIAL
  extern void console_print_68332(const char * b);
#endif

void BSP_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{
  /* Halt the CTM4 unit while setup */
  BIUMCR = BIUMCR_HALT;
  /* Setup interrupt the CTM4 module */
  BIUMCR = BIUMCR_FREEZE | BIUMCR_INT_VECT_80 | BIUMCR_IARB_2;
  /* Setup the Counter Prescaler Module, responsible for the clock, sets PCLK6 to Fsys/96=250 kHz */ 
  CPSM = CPSM_PRERUN | CPSM_DIV23_3 | CPSM_PSEL_0;   
  /* Setup the MCSM2 timer, interrupt period 100Hz  */
  MCSM2SIC = MCSM2SIC_IRQ_1 | MCSM2SIC_ARB3_0 | MSCM2SIC_BUSDRIVE_0 | MCSM2SIC_PRESCALE_6;
  MCSM2CNT = 65536-2500;

  request_irq(IRQ_MACHSPEC | 130, timer_routine, IRQ_FLG_LOCK, "timer", NULL);
  
  /* Start the CTM4 module */ 
  BIUMCR = BIUMCR & ~BIUMCR_HALT;
}

void BSP_tick(void)
{
  	/* Reset Interrupt flag on Timer1 */
	MCSM2SIC = MCSM2SIC & ~MCSM2SIC_INT_FLAG;
	/*TSTAT &= 0;*/
}

unsigned long BSP_gettimeoffset (void)
{
  return 0;
}

int BSP_hwclk(int op, struct hwclk_time *t)
{
  if (!op) {
    /* read */
  } else {
    /* write */
  }
  return 0;
}

int BSP_set_clock_mmss (unsigned long nowtime)
{
#if 0
  short real_seconds = nowtime % 60, real_minutes = (nowtime / 60) % 60;

  tod->second1 = real_seconds / 10;
  tod->second2 = real_seconds % 10;
  tod->minute1 = real_minutes / 10;
  tod->minute2 = real_minutes % 10;
#endif
  return 0;
}

void BSP_reset (void)  /*  Functionality unknown...  But prolly broken for '332/376*/
{
  cli();
  asm volatile ("
    moveal #0x10c00000, %a0;
    moveb #0, 0xFFFFF300;
    moveal 0(%a0), %sp;
    moveal 4(%a0), %a0;
    jmp (%a0);
    ");
}

void config_BSP(char *command, int len)
{
#ifdef CONFIG_68332_SERIAL
  register_console(console_print_68332);
#endif  

#if defined( CONFIG_M68376 )
	printk("M68376 port by Mecel AB. <www.mecel.se>\n");
	#if defined( CONFIG_FR1000 )
		printk("FR1000 support by Mecel AB <www.mecel.se>\n");
	#endif
#endif

  mach_sched_init      = BSP_sched_init;
  mach_tick            = BSP_tick;
  mach_gettimeoffset   = BSP_gettimeoffset;
  mach_hwclk           = NULL;
  mach_mksound         = NULL;
  mach_reset           = BSP_reset;
  mach_debug_init      = NULL;
  mach_trap_init       = NULL;
#if defined(CONFIG_DS1743)
  {
	  extern int ds1743_set_clock_mmss(unsigned long);
	  extern void ds1743_gettod(int *, int *, int *, int *, int *, int *);
	  mach_set_clock_mmss = ds1743_set_clock_mmss;
	  mach_gettod = ds1743_gettod;
  }
#endif

  config_M68376_irq(); 
}
