/*
 * linux/arch/$(ARCH)/platform/$(PLATFORM)/ints.c
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
#include <asm/machdep.h>
#include <asm/setup.h>

#include <asm/SH7615.h>

/* Posh 2 All IRQs are considered as equal
 * there is no difference b/w external and 
 * and On chip interrupts
 */
#define INTERNAL_IRQS (41)

/* assembler routines */
/* added SH2 specific handler for 
 * interrupts and exception
 */

/* Exceptions  Nmi, User break, Debug Intr*/
asmlinkage void cpubuserr(void);
asmlinkage void dmabuserr(void);
asmlinkage void genilleginst(void);
asmlinkage void sltilleginst(void);

asmlinkage void nmi(void);
asmlinkage void ubrk(void);
asmlinkage void hudi(void);

asmlinkage void system_call(void);
asmlinkage void trap33(void);
asmlinkage void trap34(void);
asmlinkage void trap35(void);
asmlinkage void trap36(void);
asmlinkage void trap37(void);
asmlinkage void trap38(void);
asmlinkage void trap39(void);
asmlinkage void trap40(void);
asmlinkage void trap41(void);
asmlinkage void trap42(void);
asmlinkage void trap43(void);
asmlinkage void trap44(void);
asmlinkage void trap45(void);
asmlinkage void trap46(void);
asmlinkage void trap47(void);
asmlinkage void trap48(void);
asmlinkage void trap49(void);
asmlinkage void trap50(void);
asmlinkage void trap51(void);
asmlinkage void trap52(void);
asmlinkage void trap53(void);
asmlinkage void trap54(void);
asmlinkage void trap55(void);
asmlinkage void trap56(void);
asmlinkage void trap57(void);
asmlinkage void trap58(void);
asmlinkage void trap59(void);
asmlinkage void trap60(void);
asmlinkage void trap61(void);
asmlinkage void trap62(void);
asmlinkage void trap63(void);
asmlinkage void bad_interrupt(void);

/* External IRQs */
asmlinkage void inthandler0(void);
asmlinkage void inthandler1(void);
asmlinkage void inthandler2(void);
asmlinkage void inthandler3(void);

/* On chip Interrupt Handlers */
asmlinkage void intdmac0(void);
asmlinkage void intdmac1(void);
asmlinkage void intwdt(void);
asmlinkage void intrefcmi(void);
asmlinkage void intscieri(void);
asmlinkage void intscirxi(void);
asmlinkage void intscitxi(void);
asmlinkage void intscitei(void);
asmlinkage void intfrtici(void);
asmlinkage void timerhandler(void);
asmlinkage void intfrtovi(void);
asmlinkage void intedmac(void);
asmlinkage void intsci2eri(void);
asmlinkage void intsci2rxi(void);
asmlinkage void intsci2txi(void);
asmlinkage void intsci2tei(void);




extern void *_ramvec[];

/* irq node variables for the 32 (potential) on chip sources */
static irq_node_t *int_irq_list[INTERNAL_IRQS];

static int int_irq_count[INTERNAL_IRQS];
static short int_irq_ablecount[INTERNAL_IRQS];

static void int_badint(int irq, void *dev_id, struct pt_regs *fp)
{
	num_spurious += 1;
}

/*
 * This function should be called during kernel startup to initialize
 * the amiga IRQ handling routines.
 */


void SH7615_init_trap(void)
{

	

	_ramvec[4]  = genilleginst; 
	_ramvec[6]  = sltilleginst;
	_ramvec[9]  = cpubuserr;
	_ramvec[10] = dmabuserr;
	_ramvec[11] = 0;
	_ramvec[12] = 0;
	_ramvec[13] = 0;

	_ramvec[32] = system_call;
	_ramvec[33] = trap33;
	_ramvec[34] = trap34;
	_ramvec[35] = trap35;
	
	_ramvec[36] = trap36;
	_ramvec[37] = trap37;
	_ramvec[38] = trap38;
	_ramvec[39] = trap39;
	_ramvec[40] = trap40;

	_ramvec[41] = trap41;
	_ramvec[42] = trap42;
	_ramvec[43] = trap43;
	_ramvec[44] = trap44;
	_ramvec[45] = trap45;
	_ramvec[46] = trap46;
	_ramvec[47] = trap47;
	_ramvec[48] = trap48;
	_ramvec[49] = trap49;
         
        
	_ramvec[50] = trap50;
	_ramvec[51] = trap51;
	_ramvec[52] = trap52;
	_ramvec[53] = trap53;
	_ramvec[54] = trap54;
	_ramvec[55] = trap55;
	_ramvec[56] = trap56;
	_ramvec[57] = trap57;
	_ramvec[58] = trap58;


	_ramvec[59] = trap59;
	_ramvec[60] = trap60;
	_ramvec[61] = trap61;
	_ramvec[62] = trap62;
	_ramvec[63] = trap63;

	_ramvec[64] = inthandler0;
	_ramvec[65] = inthandler1;
	_ramvec[66] = inthandler2;
	_ramvec[67] = inthandler3;
        
	_ramvec[68] = intdmac0;
	_ramvec[69] = intdmac1;
	_ramvec[70] = intwdt;
	_ramvec[71] = intrefcmi;
	_ramvec[72] = intscieri;
	_ramvec[73] = intscirxi;
	_ramvec[74] = intscitxi;
	_ramvec[75] = intscitei;
	_ramvec[76] = intfrtici;
	_ramvec[77] = timerhandler; /* posh2 check this out*/


	_ramvec[78] = intfrtovi;
	_ramvec[79] = intedmac;
	_ramvec[80] = intsci2eri;
	_ramvec[81] = intsci2rxi;
	_ramvec[82] = intsci2txi;
	_ramvec[83] = intsci2tei;
}



/*
void sh7615_insert_irq(irq_node_t **list, irq_node_t *node)
{
	unsigned long flags;
	irq_node_t *cur;

	if (!node->dev_id)
		printk("%s: Warning: dev_id of %s is zero\n",
		       __FUNCTION__, node->devname);

	save_flags(flags);
	cli();

	cur = *list;

	while (cur) {
		list = &cur->next;
		cur = cur->next;
	}

	node->next = cur;
	*list = node;

	restore_flags(flags);
}

void sh7615_delete_irq(irq_node_t **list, void *dev_id)
{
	unsigned long flags;
	irq_node_t *node;

	save_flags(flags);
	cli();

	for (node = *list; node; list = &node->next, node = *list) {
		if (node->dev_id == dev_id) {
			*list = node->next;
*/
			/* Mark it as free. */
/*			node->handler = NULL;
			restore_flags(flags);
			return;
		}
	}
	restore_flags(flags);
	printk ("%s: tried to remove invalid irq\n", __FUNCTION__);
}

int sh7615_request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
                         unsigned long flags, const char *devname, void *dev_id)
{
	if (irq >= INTERNAL_IRQS) {
		printk ("%s: Unknown IRQ %d from %s\n", __FUNCTION__, irq, devname);
		return -ENXIO;
	}

	if (!int_irq_list[irq]) {
		int_irq_list[irq] = new_irq_node();
		int_irq_list[irq]->flags   = IRQ_FLG_STD;
	}

	if (!(int_irq_list[irq]->flags & IRQ_FLG_STD)) {
		if (int_irq_list[irq]->flags & IRQ_FLG_LOCK) {
			printk("%s: IRQ %d from %s is not replaceable\n",
			       __FUNCTION__, irq, int_irq_list[irq]->devname);
			return -EBUSY;
		}
		if (flags & IRQ_FLG_REPLACE) {
			printk("%s: %s can't replace IRQ %d from %s\n",
			       __FUNCTION__, devname, irq, int_irq_list[irq]->devname);
			return -EBUSY;
		}
	}
	int_irq_list[irq]->handler = handler;
	int_irq_list[irq]->flags   = flags;
	int_irq_list[irq]->dev_id  = dev_id;
	int_irq_list[irq]->devname = devname;
*/
	/* enable in the IMR */
/*	if (!int_irq_ablecount[irq])
		*(volatile unsigned long *)0xfffff304 &= ~(1<<irq);

	return 0;
}

void sh7615_free_irq(unsigned int irq, void *dev_id)
{
	if (irq >= INTERNAL_IRQS) {
		printk ("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_list[irq]->dev_id != dev_id)
		printk("%s: removing probably wrong IRQ %d from %s\n",
		       __FUNCTION__, irq, int_irq_list[irq]->devname);
	int_irq_list[irq]->handler = int_badint;
	int_irq_list[irq]->flags   = IRQ_FLG_STD;
	int_irq_list[irq]->dev_id  = NULL;
	int_irq_list[irq]->devname = NULL;

	*(volatile unsigned long *)0xfffff304 |= 1<<irq;
}

*/

/*
 * Enable/disable a particular machine specific interrupt source.
 * Note that this may affect other interrupts in case of a shared interrupt.
 * This function should only be called for a _very_ short time to change some
 * internal data, that may not be changed by the interrupt at the same time.
 * int_(enable|disable)_irq calls may also be nested.
 */


/*
void sh7615_enable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (--int_irq_ablecount[irq])
		return;
*/
	/* enable the interrupt */

/*	*(volatile unsigned long *)0xfffff304 &= ~(1<<irq);
}

void sh7615_disable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_ablecount[irq]++)
		return;


	posh2 - remvoe disable the interrupt 
	*(volatile unsigned long *)0xfffff304 |= 1<<irq;
}
*/

/* The 68k family did not have a good way to determine the source
 * of interrupts until later in the family.  The EC000 core does
 * not provide the vector number on the stack, we vector everything
 * into one vector and look in the blasted mask register...
 * This code is designed to be fast, almost constant time, not clean!
 */
/*
inline int sh7615_do_irq(int vec, struct pt_regs *fp)
{
	int irq;
	int mask;

	unsigned long pend = *(volatile unsigned long *)0xfffff30c;

	while (pend) {
		if (pend & 0x0000ffff) {
			if (pend & 0x000000ff) {
				if (pend & 0x0000000f) {
					mask = 0x00000001;
					irq = 0;
				} else {
					mask = 0x00000010;
					irq = 4;
				}
			} else {
				if (pend & 0x00000f00) {
					mask = 0x00000100;
					irq = 8;
				} else {
					mask = 0x00001000;
					irq = 12;
				}
			}
		} else {
			if (pend & 0x00ff0000) {
				if (pend & 0x000f0000) {
					mask = 0x00010000;
					irq = 16;
				} else {
					mask = 0x00100000;
					irq = 20;
				}
			} else {
				if (pend & 0x0f000000) {
					mask = 0x01000000;
					irq = 24;
				} else {
					mask = 0x10000000;
					irq = 28;
				}
			}
		}

		while (! (mask & pend)) {
			mask <<=1;
			irq++;
		}

		if (int_irq_list[irq] && int_irq_list[irq]->handler) {
			int_irq_list[irq]->handler(irq | IRQ_MACHSPEC, int_irq_list[irq]->dev_id, fp);
			int_irq_count[irq]++;
		} else {
			printk("unregistered interrupt %d!\nTurning it off in the IMR...\n", irq);
			*(volatile unsigned long *)0xfffff304 |= mask;
		}
		pend &= ~mask;
	}
	return 0;
}

int sh7615_get_irq_list(char *buf)
{
	int i, len = 0;
	irq_node_t *node;

	len += sprintf(buf+len, "Internal SH7615 interrupts\n");

	for (i = 0; i < INTERNAL_IRQS; i++) {
		if (!(node = int_irq_list[i]))
			continue;
		if (!(node->handler))
			continue;

		len += sprintf(buf+len, " %2d: %10u    %s\n", i,
		               int_irq_count[i], int_irq_list[i]->devname);
	}
	return len;
}

void config_sh7615_irq(void)
{
	mach_default_handler = NULL;
	mach_init_IRQ        = sh7615_init_IRQ;
	mach_request_irq     = sh7615_request_irq;
	mach_free_irq        = sh7615_free_irq;
	mach_enable_irq      = sh7615_enable_irq;
	mach_disable_irq     = sh7615_disable_irq;
	mach_get_irq_list    = sh7615_get_irq_list;
	mach_process_int     = sh7615_do_irq;
}

*/
