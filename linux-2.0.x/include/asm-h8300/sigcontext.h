#ifndef _ASMh8300_SIGCONTEXT_H
#define _ASMh8300_SIGCONTEXT_H

struct sigcontext_struct {
	unsigned long  sc_mask; 	/* old sigmask */
	unsigned long  sc_usp;		/* old user stack pointer */
	unsigned long  sc_er0;
	unsigned long  sc_er1;
	unsigned long  sc_er2;
	unsigned short sc_sr;
	unsigned long  sc_pc;
};

#endif
