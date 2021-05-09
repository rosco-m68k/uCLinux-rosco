/*
 *  linux/arch/$(ARCH)/platform/$(PLATFORM)/config.c
 *
 *  Copyright (c) 2000 Michael Leslie <mleslie@lineo.com>
 *  Copyright (C) 1993 Hamish Macdonald
 *  Copyright (C) 1999 D. Jeff Dionne <jeff@uclinux.org>
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

#include <asm/m68360.h>
#include<asm/quicc_cpm.h>

extern QUICC *pquicc;
//extern unsigned long int system_clock;


extern void register_console(void (*proc)(const char *));

extern void config_quicc_irq(void);

int quicc_pit_div_counter;
void (*standard_timer_routine)(int, void *, struct pt_regs*);
void quicc_timer_ISR(int irq, void *dummy, struct pt_regs *regs);


void quicc_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{
    unsigned short pitr;
    unsigned short count;
    unsigned short picr = 0;
    picr = picr | (QUICC_SIM_PIT_INT_LVL << 0x08);

    pquicc->sim_picr= picr | QUICC_SIM_PIT_INT_VEC;
    pitr = pquicc->sim_pitr;
    pitr = pitr & QUICC_PITR_SWP;   // Isolate the SWT prescale bit as to not
                                    // change it
    pquicc->sim_pitr = pitr;    // Stop the PIT.
    //count = QUICC_PIT_SPCLK/(4*HZ);
    count = (OSCILLATOR/128)/(4*HZ);
    if(count == 0)
        count = 1;
    if(count > QUICC_PIT_MAX_COUNT)
    {
        count = count/QUICC_PIT_DIVIDE;
        quicc_pit_div_counter=0;
        standard_timer_routine = timer_routine;
        request_irq(QUICC_SIM_PIT_INT_VEC | IRQ_MACHSPEC, quicc_timer_ISR,
                IRQ_FLG_LOCK, "timer", NULL);
    }
    else
    {
        request_irq(QUICC_SIM_PIT_INT_VEC | IRQ_MACHSPEC, timer_routine,
                IRQ_FLG_LOCK, "timer", NULL);
    }
    pitr = pitr | count;
    pquicc->sim_pitr = pitr;
}
// Use this routine if the pit needs to be divided in software
void quicc_timer_ISR(int irq, void *dummy, struct pt_regs *regs)
{
    ++quicc_pit_div_counter;
    quicc_pit_div_counter &= (QUICC_PIT_DIVIDE-1);
    if(quicc_pit_div_counter == 0)
        standard_timer_routine(irq, dummy, regs);
}


void quicc_tick(void)
{
#if defined(CONFIG_M68360_SIM_WDT)
#else
    //Kick the watchdog timer so it does not reset the system.
    //Note: if interrupts are stopped for longer than 4 seconds, the system
    // will reset.
        quicc_kick_wdt();
#endif
}

unsigned long quicc_gettimeoffset (void)
{
  return 0;
}

int quicc_hwclk(int op, struct hwclk_time *t)
{
  if (!op) {
    /* read */
  } else {
    /* write */
  }
  return 0;
}

int quicc_set_clock_mmss (unsigned long nowtime)
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

void quicc_reset (void)
{
  cli();
  panic("Quicc Reset.\n");
  while(1);
}

unsigned char *scc1_hwaddr;
//static int errno;

void config_BSP(char *command, int len)
{
  printk("\n68360 QUICC support (C) 2000 Lineo Inc.\n");
#if defined(CONFIG_SED_SIOS)
  printk("SIOS support (C) 2002 SED Systems, a division of Calian Ltd.\n");
#endif
  scc1_hwaddr = "\00\01\02\03\04\05";

  mach_sched_init      = quicc_sched_init;
  mach_tick            = quicc_tick;
  mach_gettimeoffset   = quicc_gettimeoffset;
  mach_hwclk           = NULL;
  mach_mksound         = NULL;
  mach_reset           = quicc_reset;
  mach_debug_init      = NULL;
#if defined(CONFIG_DS1743)
  {
	  extern int ds1743_set_clock_mmss(unsigned long);
	  extern void ds1743_gettod(int *, int *, int *, int *, int *, int *);
	  mach_set_clock_mmss = ds1743_set_clock_mmss;
	  mach_gettod = ds1743_gettod;
  }
#endif

  config_quicc_irq();
}
