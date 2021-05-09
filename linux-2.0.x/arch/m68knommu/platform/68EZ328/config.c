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
#ifdef CONFIG_UCSIMM
#include "ucsimm/bootstd.h"
#endif
#if defined(CONFIG_PILOT) && defined(CONFIG_PILOT_INCROMFS)
#include "PalmV/romfs.h"
#endif
#include <asm/MC68EZ328.h>

extern void register_console(void (*proc)(const char *));

void BSP_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{
  /* Restart mode, Enable int, 32KHz, Enable timer */
  TCTL = TCTL_OM | TCTL_IRQEN | TCTL_CLKSOURCE_32KHZ | TCTL_TEN;
  /* Set prescaler (Divide 32KHz by 32)*/
  TPRER = 31;
  /* Set compare register  32Khz / 32 / 10 = 100 */
  TCMP = 10;                                                              

  request_irq(IRQ_MACHSPEC | 1, timer_routine, IRQ_FLG_LOCK, "timer", NULL);
}

void BSP_tick(void)
{
  	/* Reset Timer1 */
	TSTAT &= 0;
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

void BSP_reset (void)
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

unsigned char *cs8900a_hwaddr;
static int errno;

#ifdef CONFIG_UCSIMM
_bsc0(char *, getserialnum)
_bsc1(unsigned char *, gethwaddr, int, a)
_bsc1(char *, getbenv, char *, a)
#endif

void config_BSP(char *command, int len)
{
  unsigned char *p;
#ifdef CONFIG_68328_SERIAL
  extern void console_print_68328(const char * b);
  register_console(console_print_68328);
#endif  

  printk("\n68EZ328 DragonBallEZ support (C) 1999 Rt-Control, Inc\n");

#ifdef CONFIG_UCSIMM
  printk("uCsimm serial string [%s]\n",getserialnum());
  p = cs8900a_hwaddr = gethwaddr(0);
  printk("uCsimm hwaddr %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
         p[0],
         p[1],
         p[2],
         p[3],
         p[4],
         p[5]);

  p = getbenv("APPEND");
  if (p) strcpy(p,command);
  else command[0] = 0;
#endif

#ifdef CONFIG_CWEZ328
  strcpy(command, "console=ttyS0");
#endif

  mach_sched_init      = BSP_sched_init;
  mach_tick            = BSP_tick;
  mach_gettimeoffset   = BSP_gettimeoffset;
  mach_hwclk           = NULL;
  mach_mksound         = NULL;
  mach_reset           = BSP_reset;
  mach_debug_init      = NULL;
#if defined(CONFIG_DS1743)
  {
	  extern int ds1743_set_clock_mmss(unsigned long);
	  extern void ds1743_gettod(int *, int *, int *, int *, int *, int *);
	  mach_set_clock_mmss = ds1743_set_clock_mmss;
	  mach_gettod = ds1743_gettod;
  }
#endif

  config_M68EZ328_irq();
}
