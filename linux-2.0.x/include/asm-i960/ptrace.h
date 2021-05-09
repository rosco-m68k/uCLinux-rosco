#ifndef _I960_PTRACE_H
#define _I960_PTRACE_H

/* this file describes the stack. While our actual stack frame contains only
 * the local registers, a few words before the frame pointer we'll have the
 * PC and AC, so we just consider this structure as if it were the actual
 * regset. Since interrupts save the g-regs, we might as well make those
 * available as well.
 */

/* 16 local registers */
#define PT_R0	0
#define PT_R1	1
#define PT_R2	2
#define PT_R3	3
#define PT_R4	4
#define PT_R5	5
#define PT_R6	6
#define PT_R7	7
#define PT_R8	8
#define PT_R9	9
#define PT_R10	10
#define PT_R11	11
#define PT_R12	12
#define PT_R13	13
#define PT_R14	14
#define PT_R15	15
/* AKAs for local registers */
#define PT_PFP	0
#define PT_SP	1
#define PT_RIP	2


#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during system calls and interrupts */

struct pt_regs {
	/* FP - 16 */
	long	pc;		/* processor control reg */
	long	ac;		/* arithmetic control reg */
	unsigned char vecs[4];	/* vecs[3] is irq; rest are reserved */
	long	reserved;

	/* FP */
	long	lregs[16];	/* 16 local regs */
	long	gregs[16];	/* 16 global regs */
};


#ifdef __KERNEL__

#if 1
#define user_mode(regs) (!test_bit(1, &(regs)->pc) )
#else
/*
 * Two cases: if 1st bit of pfp is set, it was a system call from user mode.
 * Otherwise, if the pfp's return field is 7 (i.e., it was an interrupt) we
 * can check the image of the PC. When we're in syscalls, we can't trust the
 * PC image in regs...
 */
#define user_mode(regs)	\
( ((regs)->lregs[PT_PFP] & 0x2) || \
(((regs)->lregs[PT_PFP] & 0x7)==0x7 && (!test_bit(1, &(regs)->pc))) )
extern void show_regs(struct pt_regs *);
#endif	/* 0 */

/* get RIP out of previous frame */
#define instruction_pointer(regs)	\
( ((unsigned long*)((regs)->lregs[PT_PFP]))[PT_RIP])

#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */

#endif /* _I960_PTRACE_H */
