#ifndef _SH7615_SIGCONTEXT_H
#define _SH7615_SIGCONTEXT_H

struct sigcontext_struct {
	unsigned long  sc_mask; 	/* old sigmask */
	unsigned long  sc_usp;		/* old user stack pointer */
	long     r1;
  	long     r2;
  	long     r3;
  	long     r4;
	long     r5;
  	long     r6;
  	long     r7;
  	long     r8;
  	long	 r9;
	long     r10;
	long     r11;
  	long	 r12;
	long     r13;
	long     r14;
	long     r0;
	long     orig_r0;
	long	 pr;
	long     vec;
	long	 pc;
	long	 sr;
};

#endif
