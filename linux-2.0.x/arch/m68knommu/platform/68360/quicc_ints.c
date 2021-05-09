/*
 * linux/arch/$(ARCH)/platform/$(PLATFORM)/ints.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2000  Michael Leslie <mleslie@lineo.com>
 * Copyright (c) 1996 Roman Zippel
 * Copyright (c) 1999 D. Jeff Dionne <jeff@uclinux.org>
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

#include <asm/m68360.h>


// from quicc/commproc.c
extern QUICC *pquicc;
extern void cpm_interrupt_init(void);




// assembler routines
asmlinkage void system_call(void);
asmlinkage void buserr(void);
asmlinkage void trap(void);
asmlinkage void trap3(void);
asmlinkage void trap4(void);
asmlinkage void trap5(void);
asmlinkage void trap6(void);
asmlinkage void trap7(void);
asmlinkage void trap8(void);
asmlinkage void trap9(void);
asmlinkage void trap10(void);
asmlinkage void trap11(void);
asmlinkage void trap12(void);
asmlinkage void trap13(void);
asmlinkage void trap14(void);
asmlinkage void trap15(void);
asmlinkage void trap33(void);
asmlinkage void bad_interrupt(void);
asmlinkage void inthandler(void);

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
 * IRQ handling routines.
 */

void quicc_init_IRQ(void)
{
        int i;
       
        /* set up the vectors */
        _ramvec[2] = buserr;
        _ramvec[3] = trap3;
        _ramvec[4] = trap4;
        _ramvec[5] = trap5;
        _ramvec[6] = trap6;
        _ramvec[7] = trap7;
        _ramvec[8] = trap8;
        _ramvec[9] = trap9;
        _ramvec[10] = trap10;
        _ramvec[11] = trap11;
        _ramvec[12] = trap12;
        _ramvec[13] = trap13;
        _ramvec[14] = trap14;
        _ramvec[15] = trap15;
       
        //Autovectors
        _ramvec[25] = inthandler;
        _ramvec[26] = inthandler;
        _ramvec[27] = inthandler;
        _ramvec[28] = inthandler;
        _ramvec[29] = inthandler;
        _ramvec[30] = inthandler;
        _ramvec[31] = inthandler;
        _ramvec[32] = system_call;
        _ramvec[33] = trap33;

        cpm_interrupt_init();

        _ramvec[CPMVEC_ERROR]           = bad_interrupt; // Error
        _ramvec[CPMVEC_PIO_PC11]        = inthandler;   // pio - pc11
        _ramvec[CPMVEC_PIO_PC10]        = inthandler;   // pio - pc10
        _ramvec[CPMVEC_SMC2]            = inthandler;   // smc2/pip
        _ramvec[CPMVEC_SMC1]            = inthandler;   // smc1
        _ramvec[CPMVEC_SPI]             = inthandler;   // spi
        _ramvec[CPMVEC_PIO_PC9]         = inthandler;   // pio - pc9
        _ramvec[CPMVEC_TIMER4]          = inthandler;   // timer 4
        _ramvec[CPMVEC_RESERVED1]       = inthandler;   // reserved
        _ramvec[CPMVEC_PIO_PC8]         = inthandler;   // pio - pc8
        _ramvec[CPMVEC_PIO_PC7]         = inthandler;  // pio - pc7
        _ramvec[CPMVEC_PIO_PC6]         = inthandler;  // pio - pc6
        _ramvec[CPMVEC_TIMER3]          = inthandler;  // timer 3
        _ramvec[CPMVEC_RISCTIMER]       = inthandler;  // reserved
        _ramvec[CPMVEC_PIO_PC5]         = inthandler;  // pio - pc5
        _ramvec[CPMVEC_PIO_PC4]         = inthandler;  // pio - pc4
        _ramvec[CPMVEC_RESERVED2]       = inthandler;  // reserved
        _ramvec[CPMVEC_RISCTIMER]       = inthandler;  // timer table
        _ramvec[CPMVEC_TIMER2]          = inthandler;  // timer 2
        _ramvec[CPMVEC_RESERVED3]       = inthandler;  // reserved
        _ramvec[CPMVEC_IDMA2]           = inthandler;  // idma 2
        _ramvec[CPMVEC_IDMA1]           = inthandler;  // idma 1
        _ramvec[CPMVEC_SDMA_CB_ERR]     = inthandler;  // sdma channel bus error
        _ramvec[CPMVEC_PIO_PC3]         = inthandler;  // pio - pc3
        _ramvec[CPMVEC_PIO_PC2]         = inthandler;  // pio - pc2
        _ramvec[CPMVEC_TIMER1]          = inthandler;  // timer 1
        _ramvec[CPMVEC_PIO_PC1]         = inthandler;  // pio - pc1
        _ramvec[CPMVEC_SCC4]            = inthandler;  // scc 4
        _ramvec[CPMVEC_SCC3]            = inthandler;  // scc 3
        _ramvec[CPMVEC_SCC2]            = inthandler;  // scc 2
        _ramvec[CPMVEC_SCC1]            = inthandler;  // scc 1
        _ramvec[CPMVEC_PIO_PC0]         = inthandler;  // pio - pc0
        _ramvec[QUICC_SIM_PIT_INT_VEC]  = inthandler;  // pit

	/* initialize handlers */
        for (i = (QUICC_SIM_PIT_INT_VEC + 1); i < INTERNAL_IRQS; i = i + 1)
                _ramvec[i] = inthandler;
       
        for (i = 0; i < INTERNAL_IRQS; i++)
        {
                int_irq_list[i] = NULL;
                int_irq_ablecount[i] = 0;
                int_irq_count[i] = 0;
        }
}


void quicc_insert_irq(irq_node_t **list, irq_node_t *node)
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

void quicc_delete_irq(irq_node_t **list, void *dev_id)
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


int quicc_request_irq(unsigned int irq,
                void (*handler)(int, void *, struct pt_regs *),
                unsigned long flags, const char *devname, void *dev_id)
{
        if (irq >= INTERNAL_IRQS)
        {
                printk ("%s: Unknown IRQ %d from %s\n", __FUNCTION__, irq,
                                devname);
                return -ENXIO;
        }

	if (!int_irq_list[irq])
        {
                int_irq_list[irq] = new_irq_node();
                int_irq_list[irq]->flags   = IRQ_FLG_STD;
	}

	if (!(int_irq_list[irq]->flags & IRQ_FLG_STD))
        {
                if (int_irq_list[irq]->flags & IRQ_FLG_LOCK)
                {
                        printk("%s: IRQ %d from %s is not replaceable\n",
                                        __FUNCTION__, irq,
                                        int_irq_list[irq]->devname);
                        return -EBUSY;
                }
                if (flags & IRQ_FLG_REPLACE)
                {
                        printk("%s: %s can't replace IRQ %d from %s\n",
                                        __FUNCTION__, devname, irq,
                                        int_irq_list[irq]->devname);
                        return -EBUSY;
                }
        }
        int_irq_list[irq]->handler = handler;
        int_irq_list[irq]->flags   = flags;
        int_irq_list[irq]->dev_id  = dev_id;
        int_irq_list[irq]->devname = devname;

        /* enable in the CIMR*/
        if((irq >= CPM_VECTOR_BASE) && (irq <= CPM_VECTOR_END))
        {
                int mask = (1<<(irq - CPM_VECTOR_BASE));
                if (!int_irq_ablecount[irq])
                        pquicc->intr_cimr |= mask;
        }
        return 0;
}


void quicc_free_irq(unsigned int irq, void *dev_id)
{
	if (irq >= INTERNAL_IRQS)
        {
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

	/* disable in the CIMR */
        if((irq >= CPM_VECTOR_BASE) && (irq <= CPM_VECTOR_END))
        {
                int mask = (1<<(irq - CPM_VECTOR_BASE));
                if (!int_irq_ablecount[irq])
                        pquicc->intr_cimr &= ~mask;
        }
}

/*
 * Enable/disable a particular machine specific interrupt source.
 * Note that this may affect other interrupts in case of a shared interrupt.
 * This function should only be called for a _very_ short time to change some
 * internal data, that may not be changed by the interrupt at the same time.
 * int_(enable|disable)_irq calls may also be nested.
 */

void quicc_enable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (--int_irq_ablecount[irq])
		return;

	// enable in the CIMR 
    if((irq >= CPM_VECTOR_BASE) && (irq <= CPM_VECTOR_END))
    {
	    int mask = (1<<(irq - CPM_VECTOR_BASE));
	    if (!int_irq_ablecount[irq])
		    pquicc->intr_cimr |= mask;
    }
}

void quicc_disable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_ablecount[irq]++)
		return;

	// disable in the CIMR 
    if((irq >= CPM_VECTOR_BASE) && (irq <= CPM_VECTOR_END))
    {
	    int mask = (1<<(irq - CPM_VECTOR_BASE));
	    if (!int_irq_ablecount[irq])
		    pquicc->intr_cimr &= ~mask;
    }
}

/* The 68k family did not have a good way to determine the source
 * of interrupts until later in the family.  The EC000 core does
 * not provide the vector number on the stack, we vector everything
 * into one vector and look in the blasted mask register...
 * This code is designed to be fast, almost constant time, not clean!
 */
inline int quicc_do_irq(int vec, struct pt_regs *fp)
{
        int irq;
        int mask;
       
        irq = vec;

        if((irq >= CPM_VECTOR_BASE) && (irq <= CPM_VECTOR_END))
                mask = (1<<(irq - CPM_VECTOR_BASE)); 
        else
                mask=0;
       
        if (int_irq_list[irq] && int_irq_list[irq]->handler)
        {
                int_irq_list[irq]->handler(irq | IRQ_MACHSPEC,
                                int_irq_list[irq]->dev_id, fp);
                int_irq_count[irq]++;
               
                /*
                 *  Clear the interrupt in the cisr
                 *  Note: if the interrupt is not from the cisr, we write all
                 *   zeros to the cisr; writeing zeros does nothing.
                 */
                pquicc->intr_cisr = mask;
                /* indicate that irq has been serviced */
                return(0);
        }
        else
        {
                if(mask != 0)
                {
                        printk("unregistered interrupt %d!\n"
                               "Turning it off in the CIMR...\n", irq);
                        pquicc->intr_cimr &= ~mask;
                        return(0);
                }
                //Unable to handle intterrupt, leave to general handler
                return(-1);
        }
        return 0;
}

int quicc_get_irq_list(char *buf)
{
	int i, len = 0;
	irq_node_t *node;

	len += sprintf(buf+len, "Internal 68360 interrupts\n");

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

void config_quicc_irq(void)
{
	mach_default_handler = NULL;
	mach_init_IRQ        = quicc_init_IRQ;
	mach_request_irq     = quicc_request_irq;
	mach_free_irq        = quicc_free_irq;
	mach_enable_irq      = quicc_enable_irq;
	mach_disable_irq     = quicc_disable_irq;
	mach_get_irq_list    = quicc_get_irq_list;
	mach_process_int     = quicc_do_irq;
}
