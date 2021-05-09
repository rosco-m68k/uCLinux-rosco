#ifndef _OR32_DELAY_H
#define _OR32_DELAY_H

extern unsigned long loops_per_sec;
#include <linux/kernel.h>
#include <linux/config.h>

extern __inline__ void __delay(unsigned long loops)
{
	__asm__ __volatile__ ("1: l.sfeqi %0,0; \
				l.bnf	1b; \
				l.addi %0,%0,-1;"
				: "=r" (loops) : "0" (loops));
}

extern __inline__ void udelay(unsigned long usecs)
{
	/* Sigh */
	__delay(usecs);
}

#define muldiv(a, b, c)    (((a)*(b))/(c))

#endif /* defined(_OR32_DELAY_H) */
