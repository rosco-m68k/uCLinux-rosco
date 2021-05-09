/*
 * Modification History:
 *
 * posh2- Changed the ptregs to SH specific
 *
 * This file needs to be changed , but the register values
 * has to be modified before that. So we have to change
 * the register values
 */
#ifndef _SH7615_PTRACE_H
#define _SH7615_PTRACE_H

#include <linux/config.h> /* get configuration macros */

/*#define SHCOMPILE*/
#ifdef  SHCOMPILE

#define PT_D1	   0
#define PT_D2	   1
#define PT_D3	   2
#define PT_D4	   3
#define PT_D5	   4
#define PT_D6	   5
#define PT_D7	   6
#define PT_A0	   7
#define PT_A1	   8
#define PT_A2	   9
#define PT_A3	   10
#define PT_A4	   11
#define PT_A5	   12
#define PT_A6	   13
#define PT_D0	   14
#define PT_USP	   15
#define PT_ORIG_D0 16
#define PT_SR	   17
#define PT_PC	   18

#else  /*SH Specific*/

#define PT_R1	   0
#define PT_R2	   1
#define PT_R3	   2
#define PT_R4	   3
#define PT_R5	   4
#define PT_R6	   5
#define PT_R7	   6
#define PT_R8	   7
#define PT_R9	   8
#define PT_R10	   9
#define PT_R11	   10
#define PT_R12	   11
#define PT_R13	   12
#define PT_R14	   13
#define PT_R0	   14
#define PT_ORIG_R0 15
#define PT_PR 	   16
#define PT_SR	   17
#define PT_PC	   18
#define PT_VEC     19
#define PT_USP     100  /* Posh2 added to avoid error  We have to find the exact use*/
#endif

#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during a system call. */
#ifdef  SHCOMPILE
struct pt_regs {
  long     d1;
  long     d2;
  long     d3;
  long     d4;
  long     d5;
  long     a0;
  long     a1;
  long     d0;
  long     orig_d0;
  long     stkadj;
#ifdef CONFIG_COLDFIRE
  unsigned format :  4; /* frame format specifier */
  unsigned vector : 12; /* vector offset */
  unsigned short sr;
  unsigned long  pc;
#else
  unsigned short sr;
  unsigned long  pc;
#ifndef NO_FORMAT_VEC
  unsigned format :  4; /* frame format specifier */
  unsigned vector : 12; /* vector offset */
#endif
#endif
};

#else  /*SH specific*/
struct pt_regs {
  long     r1;
  long     r2;
  long     r3;
  long     r4;
  long     r5;
  long     r6;
  long     r7;
  long     r8;
  long	   r9;
  long     r10;
  long     r11;
  long	   r12;
  long     r13;
  long     r14;
  long     r0;
  long     orig_r0;
  long	   pr;
  long     vec;
  long	   pc;
  long	   sr;

  };
#endif

/*
 * This is the extended stack used by signal handlers and the context
 * switcher: it's pushed after the normal "struct pt_regs".
 */
 
#ifdef SHCOMPILE
struct switch_stack {
	unsigned long  d6;
	unsigned long  d7;
	unsigned long  a2;
	unsigned long  a3;
	unsigned long  a4;
	unsigned long  a5;
	unsigned long  a6;
	unsigned long  retpc;
};
#else
struct switch_stack {
	unsigned long  r6;
	unsigned long  retpc;
};
#endif


#ifdef __KERNEL__

#ifndef PS_S
#define PS_S  (0x000000f0)
#define PS_M  (0x1000)
#endif

#define user_mode(regs) (!((regs)->sr & PS_S))
#define instruction_pointer(regs) ((regs)->pc)
extern void show_regs(struct pt_regs *);
#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */
#endif /* _SH7615_PTRACE_H */
