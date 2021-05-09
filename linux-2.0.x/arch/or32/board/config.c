/*
 *  linux/arch/$(ARCH)/platform/$(PLATFORM)/config.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Based on m68knommu/platform/xx/config.c
 */

#include <stdarg.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/console.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/irq.h>
#include <asm/machdep.h>

extern void register_console(void (*proc)(const char *));

/* Tick timer period */
unsigned long tick_period = SYS_TICK_PER;

void BSP_sched_init(void (*timer_routine)(int, void *, struct pt_regs *))
{
	/* Set counter period, enable timer and interrupt */
	mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (SYS_TICK_PER & SPR_TTMR_PERIOD));
}

void BSP_tick(void)
{
	mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (SYS_TICK_PER & SPR_TTMR_PERIOD));
}

unsigned long BSP_gettimeoffset (void)
{
	return 0;
}

void BSP_gettod (int *yearp, int *monp, int *dayp,
		   int *hourp, int *minp, int *secp)
{
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
}

void config_BSP(char *command, int len)
{
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
	
	
