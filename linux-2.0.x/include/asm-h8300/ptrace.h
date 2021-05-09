#ifndef _H8300_PTRACE_H
#define _H8300_PTRACE_H

#include <linux/config.h> /* get configuration macros */

#define PT_ER1	   0
#define PT_ER2	   1
#define PT_ER3	   2
#define PT_ER4	   3
#define PT_ER5	   4
#define PT_ER6	   5
#define PT_ER0	   6
#define PT_ORIG_ER0	   7
#define PT_CCR	   8
#define PT_PC	   9
#define PT_USP	   10

#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during a system call. */

struct pt_regs {
  long     er1;
  long     er2;
  long     er3;
  long     er4;
  long     er5;
  long     er0;
  long     orig_er0;
  long     pc;
  short    ccr;
  unsigned long kmod_pc;
};
/* 2.95.3でオフセットの計算を間違えることがある。
   ここは触らないほうがいいかも。 */

/*
 * This is the extended stack used by signal handlers and the context
 * switcher: it's pushed after the normal "struct pt_regs".
 */
struct switch_stack {
	unsigned long  er6;
	unsigned long  retpc;
};

#ifdef __KERNEL__

#ifndef PS_S
#define PS_S  (0x2000)
#define PS_M  (0x1000)
#endif

#define user_mode(regs) (!((regs)->ccr & 0x10))
#define instruction_pointer(regs) (*((unsigned long *)(regs)+7))
extern void show_regs(struct pt_regs *);
#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */
#endif /* _H8300H_PTRACE_H */
