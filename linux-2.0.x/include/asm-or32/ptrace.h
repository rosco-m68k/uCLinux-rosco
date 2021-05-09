#ifndef _OR32_PTRACE_H
#define _OR32_PTRACE_H

#include <linux/config.h> /* get configuration macros */

#define STATE 		0
#define COUNTER 	4
#define SIGNAL 		12
#define BLOCKED 	16
#define TASK_FLAGS 	20
#define TSS 		528
#define TSS_PC 		0
#define TSS_SR 		4
#define TSS_KSP 	8
#define TSS_USP 	12
#define TSS_REGS 	16

#define PC 		0
#define SR 		4
#define SP 		8
#define GPR2 		12
#define GPR3 		16
#define GPR4 		20
#define GPR5 		24
#define GPR6 		28
#define GPR7 		32
#define GPR8 		36
#define GPR9 		40
#define GPR10 		44
#define GPR11 		48
#define GPR12 		52
#define GPR13 		56
#define GPR14 		60
#define GPR15 		64
#define GPR16 		68
#define GPR17 		72
#define GPR18 		76
#define GPR19 		80
#define GPR20 		84
#define GPR21 		88
#define GPR22 		92
#define GPR23 		96
#define GPR24 		100
#define GPR25 		104
#define GPR26 		108
#define GPR27 		112
#define GPR28 		116
#define GPR29 		120
#define GPR30 		124
#define GPR31 		128
#define ORIG_GPR3	132
#define RESULT		136

#define INT_FRAME_SIZE 	140

#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during a system call. */

struct pt_regs {
	long	pc;
	long	sr;
	long	sp;
	long	gprs[30];
	long	orig_gpr3;	/* Used for restarting system calls */
        long	result;		/* Result of a system call */
};

/*
 * This is the extended stack used by signal handlers and the context
 * switcher: it's pushed after the normal "struct pt_regs".
 */
/* SIMON: This is can not be like this */
/*struct switch_stack {
	unsigned long  d6;
	unsigned long  d7;
	unsigned long  a2;
	unsigned long  a3;
	unsigned long  a4;
	unsigned long  a5;
	unsigned long  a6;
	unsigned long  retpc;
};
*/
#ifdef __KERNEL__

#ifndef PS_S
#define PS_S  (0x2000)
#define PS_M  (0x1000)
#endif

#define user_mode(regs) (!((regs)->sr & SPR_SR_SM))
#define instruction_pointer(regs) ((regs)->pc)
extern void show_regs(struct pt_regs *);
#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */
#endif /* _OR32_PTRACE_H */
