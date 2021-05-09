/*
 * linux/arch/h8300/kernel/ints.c
 *
 * Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 * Based on linux/arch/$(ARCH)/platform/$(PLATFORM)/ints.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright 1996 Roman Zippel
 * Copyright 1999 D. Jeff Dionne <jeff@rt-control.com>
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/io.h>
#include <asm/setup.h>

#define INTERNAL_IRQS (64)

/* assembler routines */
asmlinkage void system_call(void);
asmlinkage void interrupt7(void);
asmlinkage void interrupt12(void);
asmlinkage void interrupt13(void);
asmlinkage void interrupt14(void);
asmlinkage void interrupt15(void);
asmlinkage void interrupt16(void);
asmlinkage void interrupt17(void);
asmlinkage void interrupt20(void);
asmlinkage void interrupt21(void);
asmlinkage void interrupt22(void);
asmlinkage void interrupt23(void);
asmlinkage void interrupt24(void);
asmlinkage void interrupt25(void);
asmlinkage void interrupt26(void);
asmlinkage void interrupt28(void);
asmlinkage void interrupt29(void);
asmlinkage void interrupt30(void);
asmlinkage void interrupt32(void);
asmlinkage void interrupt33(void);
asmlinkage void interrupt34(void);
asmlinkage void interrupt36(void);
asmlinkage void interrupt37(void);
asmlinkage void interrupt38(void);
asmlinkage void interrupt39(void);
asmlinkage void interrupt40(void);
asmlinkage void interrupt41(void);
asmlinkage void interrupt42(void);
asmlinkage void interrupt43(void);
asmlinkage void interrupt44(void);
asmlinkage void interrupt45(void);
asmlinkage void interrupt46(void);
asmlinkage void interrupt47(void);
asmlinkage void interrupt52(void);
asmlinkage void interrupt53(void);
asmlinkage void interrupt54(void);
asmlinkage void interrupt55(void);
asmlinkage void interrupt56(void);
asmlinkage void interrupt57(void);
asmlinkage void interrupt58(void);
asmlinkage void interrupt59(void);
asmlinkage void interrupt60(void);
asmlinkage void interrupt61(void);
asmlinkage void interrupt62(void);
asmlinkage void interrupt63(void);
asmlinkage void bad_interrupt(void);

extern void *_ramvec[];

/* irq node variables for the 32 (potential) on chip sources */
static irq_node_t *int_irq_list[INTERNAL_IRQS];

static int int_irq_count[INTERNAL_IRQS];
static short int_irq_ablecount[INTERNAL_IRQS];

static void int_badint(int irq, void *dev_id, struct pt_regs *fp)
{
	num_spurious += 1;
}
