#ifndef _OR32_IRQ_H_
#define _OR32_IRQ_H_

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

#include <linux/config.h>


#define SYS_IRQS 32
#define NR_IRQS SYS_IRQS

/*
 * "Generic" interrupt sources
 */

#define IRQ_UART_0	(19)	/* interrupt source for UART dvice 0 */
#define IRQ_ETH_0	(15)	/* interrupt source for Ethernet dvice 0 */
#define IRQ_PS2_0	(12)	/* interrupt source for ps2 dvice 0 */
#define IRQ_SCHED_TIMER	(0)	/* interrupt source for scheduling timer */

/*
 * various flags for request_irq()
 */
#define IRQ_FLG_LOCK	(0x0001)	/* handler is not replaceable	*/
#define IRQ_FLG_REPLACE	(0x0002)	/* replace existing handler	*/
#define IRQ_FLG_PRI_HI	(0x0004)
#define IRQ_FLG_STD	(0x8000)	/* internally used		*/

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

#endif /* _OR32_IRQ_H_ */
