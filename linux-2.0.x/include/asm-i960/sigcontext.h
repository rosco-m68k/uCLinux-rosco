#ifndef _ASMm68k_SIGCONTEXT_H
#define _ASMm68k_SIGCONTEXT_H

/* FIXME huh? */

struct sigcontext_struct {
	unsigned long  sc_mask; 	/* old sigmask */
	unsigned long  sc_usp;		/* old user stack pointer */
	unsigned long  sc_d0;
	unsigned long  sc_d1;
	unsigned long  sc_a0;
	unsigned long  sc_a1;
	unsigned short sc_sr;
	unsigned long  sc_pc;
	unsigned short sc_formatvec;
};

#endif
