#ifndef _SH7615_IRQ_H_
#define _SH7615_IRQ_H_

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

#include <linux/config.h>

/* Configuring the INTC in the IRQ mode 
 * # of IRQs 4, On chip Interrupts 41, NMI, HUDI and User Break
 */
#define SYS_IRQS  59 
#define NR_IRQS    59
#define	IRQ_MACHSPEC   1  /*posh2 for dummy to compile serial.c*/
/*
 * Interrupt source definitions
 * General interrupt sources are the level 1-7.
 * Adding an interrupt service routine for one of these sources
 * results in the addition of that routine to a chain of routines.
 * Each one is called in succession.  Each individual interrupt
 * service routine should determine if the device associated with
 * that routine requires service.
 */


/* Posh 4 IRQs*/

#define IRQ0		(0)	/* level 1 interrupt */
#define IRQ1		(1)	/* level 2 interrupt */
#define IRQ2		(2)	/* level 3 interrupt */
#define IRQ3		(3)	/* level 4 interrupt */

/*
 * "Generic" interrupt sources
 */

/*#define IRQ_SCHED_TIMER	(8) */   /* interrupt source for scheduling timer */

/*
 * Machine specific interrupt sources.
 *
 * Adding an interrupt service routine for a source with this bit
 * set indicates a special machine specific interrupt source.
 * The machine specific files define these sources.
 */

/*#define IRQ_MACHSPEC	(0x10000000L)
#define IRQ_IDX(irq)	((irq) & ~IRQ_MACHSPEC)*/

/*
 * various flags for request_irq()
 */
#define IRQ_FLG_LOCK	(0x0001)	/* handler is not replaceable	*/
#define IRQ_FLG_REPLACE	(0x0002)	/* replace existing handler	*/
#define IRQ_FLG_FAST	(0x0004)
#define IRQ_FLG_SLOW	(0x0008)
#define IRQ_FLG_STD	(0x8000)	/* internally used		*/



/*
 * This structure is used to chain together the ISRs for a particular
 * interrupt source (if it supports chaining).
 */
typedef struct irq_node {
	void		(*handler)(int, void *, struct pt_regs *);
	unsigned long	flags;
	void		*dev_id;
	const char	*devname;
	struct irq_node *next;
} irq_node_t;

/*
 * This function returns a new irq_node_t
 */
extern irq_node_t *new_irq_node(void);


/*
 * This structure has only 4 elements for speed reasons
 */
typedef struct irq_handler {
	void		(*handler)(int, void *, struct pt_regs *);
	unsigned long	flags;
	void		*dev_id;
	const char	*devname;
} irq_handler_t;

/* count of spurious interrupts */
extern volatile unsigned int num_spurious;

#endif /* _SH7615_IRQ_H_ */
