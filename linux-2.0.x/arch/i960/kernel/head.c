/*
 *   FILE: head.c
 * AUTHOR: kma
 *  DESCR: Initialization code for the i960.
 */

#ident "$Id: head.c,v 1.2 2002/06/18 00:31:50 gerg Exp $"

#include <linux/autoconf.h>
#include <linux/kernel.h>

/* Enable the following if you want your system to run without any cache at all. */
#undef DISABLE_CACHE_FOR_TESTING

/*
 * So here we are! Running at ipl 31, in supervisor mode, on mon960's user
 * stack. Do we need to do much here? Let's just get right into start_kernel.
 */

int main(void)
{
	long oldac;

	extern void start_kernel(void);

	/* 64-bit math routines need the overflow mask bit set */
	__asm__ __volatile__
		("modac	%1, %1, %1" : "=r"(oldac) : "0"(1 << 12));

	/* Make the i960 think it's in an interrupt handler. This has the effect
	 * of not switching to the interrupt stack for every interrupt. Of
	 * course, we wouldn't need this if there were a sensible way of
	 * specifying an interrupt stack short of reinitializing the CPU, but
	 * such is life.
	 * 
	 * XXX: this has the effect of using user stacks for interrupts too.
	 * The world's a scary place.
	 */
	__asm__ __volatile__ ("modpc	%1, %1, %1" : "=r"(oldac) : "0"(1<<13));
	
#ifdef CONFIG_CMDLINE_PROMPT
	/* do crazy command-line obtainments... */
#endif

#ifdef DISABLE_CACHE_FOR_TESTING
/* Disable cache for testing purposes */
    asm("flushreg
	    dcctl	0, g0, g0
	    icctl	0, g0, g0");
#endif /* DISABLE_CACHE_FOR_TESTING */
    
    start_kernel();
	
	printk("XXX: start_kernel returned!!!\n");
	for(;;) ;
}

/* Define only if the compiler isn't the ctools one */
#if defined(__i960__) && !defined (__GCC960_VER)

/* This is a total hack in order to avoid having to link with the libgcc.a lib */
int __main()
{
    return;
}
#endif
