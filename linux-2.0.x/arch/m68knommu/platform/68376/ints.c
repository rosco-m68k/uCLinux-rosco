/*
 * linux/arch/$(ARCH)/platform/$(PLATFORM)/ints.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright 1996 Roman Zippel
 * Copyright 1999 D. Jeff Dionne <jeff@rt-control.com>
 * Copyright 2002 Mecel AB <www.mecel.se>
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

#include <asm/MC68332.h>

#define INTERNAL_IRQS (256)

/* assembler routines */
asmlinkage void system_call(void);
asmlinkage void buserr(void);
asmlinkage void trap(void);
asmlinkage void bad_interrupt(void);
asmlinkage void inthandler(void);
asmlinkage void timerhandler(void);

/* irq node variables for the 256 (potential) on chip sources */
static irq_node_t *int_irq_list[INTERNAL_IRQS];

static int int_irq_count[INTERNAL_IRQS];
static short int_irq_ablecount[INTERNAL_IRQS], int_irq_forbidden[INTERNAL_IRQS];

static void int_badint(int irq, void *dev_id, struct pt_regs *fp)
{
	num_spurious += 1;
}

/*
 * This assembly functions is used to access the %sr for masking interrupts.
 * The functin takes the irq and shifts it up to the high byte where the
 * where the irq mask resides. See %sr in the manual.
 */

void M68376_mask_IRQ (int irq) 
{  
  cli();
  __asm__ __volatile__ ("move.w %/sr,%/d0\n\t"
  						"or.w %0,%/d0\n\t"
  						"move.w	%/d0,%/sr\n\t"
  						: : "g" (~(irq<<8)) );
}

void M68376_enable_specific_IRQ (int irq) 
{
  cli();
  __asm__ __volatile__ ("move.w %/sr,%/d0\n\t"
  						"and.w %0,%/d0\n\t"
  						"move.w	%/d0,%/sr\n\t"
  						: : "g" ( irq<<8) );
} 

/*
 * This function should be called during kernel startup to initialize
 * the amiga IRQ handling routines.
 */

/*
 * The %vbr is setup in the head.s file and maps vbr to the _ramvec area.
 * The status register %sr is the one responsible for masking interrupts,
 * it's only accessable through assembly instructons
 */

void M68376_init_IRQ(void)
{
	int i;

	/* initialize handlers */
	for (i = 0; i < INTERNAL_IRQS; i++) {
		int_irq_list[i] = NULL;
		int_irq_forbidden[i] = NULL;
		int_irq_ablecount[i] = 0;
		int_irq_count[i] = 0;
	}

	/* Reserve the interrupts used by the CPU32, can not be allocated for 
	 * external or module interrupt. All fault handling on this level is 
	 * taken care of in entry.S, if they are taken care of at all. 
	 * For further description refer to the 68336/376 manual. 
 	 */

	int_irq_forbidden[0]  = 1;	   /* Initial SP */
	int_irq_forbidden[1]  = 1;	   /* Initial PC */
	int_irq_forbidden[2]  = 1;	   /* Bus error */
	int_irq_forbidden[3]  = 1;	   /* Adress error */
	int_irq_forbidden[4]  = 1;	   /* Illegal instruction */
	int_irq_forbidden[5]  = 1;	   /* Zero division */
	int_irq_forbidden[6]  = 1;	   /* CHK, CHK2 instructions */
	int_irq_forbidden[7]  = 1;	   /* TRAPcc, TRAPV instructions */
	int_irq_forbidden[8]  = 1;	   /* Privelege violation */
	int_irq_forbidden[9]  = 1;	   /* Trace */
	int_irq_forbidden[10] = 1;	   /* Line 1010 emulator */
	int_irq_forbidden[11] = 1;	   /* Line 1111 emulator*/
	int_irq_forbidden[12] = 1;	   /* Hardware breakpoint */
	int_irq_forbidden[13] = 1;	   /* (Reserved, coprocessor protocol violation) */
	int_irq_forbidden[14] = 1;	   /* Format errer and uninitialized interrupt */
	int_irq_forbidden[15] = 1;	   /* Format errer and uninitialized interrupt */
	int_irq_forbidden[24] = 1;	   /* Spurious interrupt */
	int_irq_forbidden[32] = 1;	   /* sys_call */


	/* turn off all interrupts */	 //Should be writing to %sr...
	M68376_mask_IRQ (7);  
}


/**************************************** 
 * This function is never called upon ? *
 ****************************************/
void M68376_insert_irq(irq_node_t **list, irq_node_t *node)
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


/**************************************** 
 * This function is never called upon ? *
 ****************************************/
void M68376_delete_irq(irq_node_t **list, void *dev_id)
{
	unsigned long flags;
	irq_node_t *node;

	save_flags(flags);
	cli();

	for (node = *list; node; list = &node->next, node = *list) {
		if (node->dev_id == dev_id) {
			*list = node->next;
			/* Mark it as free. */
			node->handler = NULL;
			restore_flags(flags);
			return;
		}
	}
	restore_flags(flags);
	printk ("%s: tried to remove invalid irq\n", __FUNCTION__);
}

int M68376_request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
                         unsigned long flags, const char *devname, void *dev_id)
{
	if (irq >= INTERNAL_IRQS) {
		printk ("%s: Unknown IRQ %d from %s\n", __FUNCTION__, irq, devname);
		return -ENXIO;
	}

	if (!int_irq_list[irq]) {
		if (int_irq_forbidden[irq]) {
        	printk("%s: IRQ %d is reserved for internal CPU interrupts\n",
			       __FUNCTION__, irq);
			return -EBUSY;
        }
        else {
	        int_irq_list[irq] = new_irq_node();
			int_irq_list[irq]->flags   = IRQ_FLG_STD;
        }
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
	
//	printk("DEBUG: IRQ request irq: %d, handler: %d, flags: %d, dev_id: %d, devname: %s\n", irq, handler, flags, dev_id, devname);
//  File needs to be cleaned up..

	
	return 0;
}

void M68376_free_irq(unsigned int irq, void *dev_id)
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

	//*(volatile unsigned long *)0xfffff304 |= 1<<irq;
}

/*
 * Enable/disable a particular machine specific interrupt source.
 * Note that this may affect other interrupts in case of a shared interrupt.
 * This function should only be called for a _very_ short time to change some
 * internal data, that may not be changed by the interrupt at the same time.
 * int_(enable|disable)_irq calls may also be nested.
 */

void M68376_enable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (--int_irq_ablecount[irq])
		return;

	/* enable the interrupt */
	//*(volatile unsigned long *)0xfffff304 &= ~(1<<irq);
}

void M68376_disable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_ablecount[irq]++)
		return;

	/* disable the interrupt */
	//*(volatile unsigned long *)0xfffff304 |= 1<<irq;
}

/* The 68k family did not have a good way to determine the source
 * of interrupts until later in the family.  The EC000 core does
 * not provide the vector number on the stack, we vector everything
 * into one vector and look in the blasted mask register...
 * This code is designed to be fast, almost constant time, not clean!
 */
inline int M68376_do_irq(int vec, struct pt_regs *fp)
{
	int irq;

	irq = vec;

	if (int_irq_list[irq] && int_irq_list[irq]->handler) {
			int_irq_list[irq]->handler(irq | IRQ_MACHSPEC, int_irq_list[irq]->dev_id, fp);
			int_irq_count[irq]++;
		} else {
            panic("No handler for interrupt vector %ld\n", vec);
	}

	return 0;
}

int M68376_get_irq_list(char *buf)
{
	int i, len = 0;
	irq_node_t *node;

	len += sprintf(buf+len, "Internal 68332 interrupts\n");

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

void config_M68376_irq(void)
{
	mach_default_handler = NULL;
	mach_init_IRQ        = M68376_init_IRQ;
	mach_request_irq     = M68376_request_irq;
	mach_free_irq        = M68376_free_irq;
	mach_enable_irq      = M68376_enable_irq;
	mach_disable_irq     = M68376_disable_irq;
	mach_get_irq_list    = M68376_get_irq_list;
	mach_process_int     = M68376_do_irq;
}
