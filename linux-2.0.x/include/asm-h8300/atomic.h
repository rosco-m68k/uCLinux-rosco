#ifndef __ARCH_H8300_ATOMIC__
#define __ARCH_H8300_ATOMIC__

#include <linux/config.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

/*
 * We do not have SMP m68k systems, so we don't have to deal with that.
 */

typedef int atomic_t;

static __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
	__asm__ __volatile__("stc ccr,@-sp\n\t"
	                     "orc #0x80,ccr\n\t"
	                     "mov.l %0,er0\n\t"
	                     "mov.l %1,er1\n\t"
	                     "add.l er1,er0\n\t"
	                     "mov.l er0,%0\n\t"
	                     "ldc @sp+,ccr" : : "m" (*v), "ir" (i):"er0","er1");
}

static __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
	__asm__ __volatile__("stc ccr,@-sp\n\t"
	                     "orc #0x80,ccr\n\t"
	                     "mov.l %0,er0\n\t"
	                     "mov.l %1,er1\n\t"
	                     "sub.l er1,er0\n\t"
	                     "mov.l er0,%0\n\t"
	                     "ldc @sp+,ccr" : : "m" (*v), "ir" (i):"er0","er1");
}

static __inline__ void atomic_inc(atomic_t *v)
{
	__asm__ __volatile__("stc ccr,@-sp\n\t"
	                     "orc #0x80,ccr\n\t"
	                     "mov.l %0,er0\n\t"
	                     "inc.l #1,er0\n\t"
	                     "mov.l er0,%0\n\t"
	                     "ldc @sp+,ccr" : : "m" (*v):"er0");
}

static __inline__ void atomic_dec(atomic_t *v)
{
	__asm__ __volatile__("stc ccr,@-sp\n\t"
	                     "orc #0x80,ccr\n\t"
	                     "mov.l %0,er0\n\t"
	                     "dec.l #1,er0\n\t"
	                     "mov.l er0,%0\n\t"
	                     "ldc @sp+,ccr" : : "m" (*v):"er0");
}

static __inline__ int atomic_dec_and_test(atomic_t *v)
{
	int c;
	__asm__ __volatile__("stc ccr,@-sp\n\t"
	                     "orc #0x80,ccr\n\t"
	                     "mov.l %1,%0\n\t"
	                     "dec.l #1,%0\n\t"
	                     "mov.l %0,%1\n\t"
	                     "ldc @sp+,ccr\n\t" :"=r"(c): "m" (*v));
	return c == 0;
}

#endif /* __ARCH_H8300_ATOMIC __ */
