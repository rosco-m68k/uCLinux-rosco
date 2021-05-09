#ifndef _I960JX_H
#define _I960JX_H

#include <asm/i960.h>
/* Default Logical Memory Configuration Register */
#define DLMCON	0xFF008100

/* Logical Memory Address/Mask Registers */
#define LMADR0	0xFF008108
#define LMMR0	0xFF00810C
#define LMADR1	0xFF008110
#define LMMR1	0xFF008114

/* Instruction Breakpoint Registers */
#define IPB0	0xFF008400
#define IPB1	0xFF008404

/* Data Address Breakpoint Registers */
#define DAB0	0xFF008420
#define DAB1	0xFF008424

/* Breakpoint Control Register */
#define BPCON	0xFF008440

/* Interrupt Mask - IMSK and Interrupt Pending Registers - -IPND */
#define IPND	0xFF008500
#define IMSK	0xFF008504

/* Interrupt Control Register */
#define ICON	0xFF008510

/* Interrupt Mapping Registers */
#define IMAP0	0xFF008520
#define IMAP1	0xFF008524
#define IMAP2	0xFF008528

/* Physical Memory Control Registers */
#define PMCON0_1	0xFF008600
#define PMCON2_3	0xFF008608
#define PMCON4_5	0xFF008610
#define PMCON6_7	0xFF008618
#define PMCON8_9	0xFF008620
#define PMCON10_11	0xFF008628
#define PMCON12_13	0xFF008630
#define PMCON14_15	0xFF008638

/* Bus Control Register */
#define BCON	0xFF0086FC

/* Initial PRCB address; might be changed by calls to sysctl */
#define INITIAL_PRCB	0xFF008700

#ifndef __ASSEMBLY__
/* PRCB */
typedef struct {
	void*	pr_fault_tab;		/* fault handlers */
	void*	pr_ctl_tab;		/* control table */
	reg_t	pr_acreg;		/* AC reg initial image */
	reg_t	pr_fault_cw;		/* fault config word */
	itab_t*	pr_intr_tab;		/* interrupt handlers */
	void*	pr_syscall_tab;		/* system calls */
	void*	pr_reserved;		/* unused */
	void*	pr_intr_stack;		/* interrupt stack */
	reg_t	pr_icache_cw;		/* icache config word */
	reg_t	pr_regcache_cw;		/* register cache config word */
} prcb_t;
#endif /* __ASSEMBLY__ */

#endif
