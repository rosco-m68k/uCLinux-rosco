/*
 *  linux/arch/m68knommu/platform/68332/config.c
 *
 *  Copyright (C) 1993 Hamish Macdonald
 *  Copyright (C) 1999 D. Jeff Dionne
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Cloned from linux/arch/m68knommu/platform/68360/config.c
 * Gerold Boehler <gboehler@mail.austria.at>
 *
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


#include <asm/machdep.h>

void config_mwi_irq(void);

void BSP_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{

	PITR = 0x0000;		// clear pit
	PICR = 0x0440;		// setup timer for int 64
	request_irq(IRQ_MACHSPEC | 0x40, timer_routine, IRQ_FLG_LOCK, "timer", NULL);
	PITR = 0x0052;		// 10 ms interrupt
}

void BSP_tick(void)
{
#ifdef CONFIG_MWI
	static char blinkencnt = 0;
	switch(blinkencnt++) {
		case 0: PORTF = 0xf7; break;
		case 25: PORTF = 0xfb; break;
		case 50: PORTF = 0xfd; break;
		case 75: PORTF = 0xfe; break;
		case 100: blinkencnt = 0;
	}
#endif
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

}

void config_BSP(char *command, int len)
{
	mach_sched_init      = BSP_sched_init;
	mach_tick            = BSP_tick;
	mach_gettimeoffset   = BSP_gettimeoffset;
	mach_hwclk           = NULL;
	mach_mksound         = NULL;
	mach_reset           = BSP_reset;
	mach_debug_init      = NULL;
	mach_trap_init       = NULL;

	config_mwi_irq();
}
