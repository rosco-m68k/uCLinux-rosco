#ifndef __ARCH_I960_ATOMIC__
#define __ARCH_I960_ATOMIC__

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

typedef int atomic_t;

static __inline__ atomic_t atomic_add(atomic_t i, atomic_t *v) 
{
	atomic_t r;
	__asm__ __volatile__("atadd	%3, %2, %1"
			     : "=m" (*v), "=d" (r)
			     : "dI" (i), "d" (v), "m"(*v));
	return r;
}

static __inline__ atomic_t atomic_sub(atomic_t i, atomic_t *v) 
{
	atomic_t tmp, retval;
	__asm__ __volatile__("subo	%4, 0, %2\n\t" 
			     "atadd	%5, %6, %1\n\t"
			     : "=m"(*v), "=d" (retval), "=d" (tmp)
			     : "m" (*v), "dI" (i), "d" (v), "2" (tmp));
	return retval;
}

static __inline__ atomic_t atomic_inc(atomic_t *v)
{
	atomic_t retval;
	__asm__ __volatile__("atadd	%2, 1, %1"
			     : "=m"(*v), "=d"(retval)
			     : "d" (v), "m"(*v));
	return retval;
}

static __inline__ atomic_t atomic_dec(atomic_t *v)
{
	return(atomic_add(-1, v));
}

static __inline__ atomic_t atomic_dec_and_test(atomic_t *v)
{
	return atomic_dec(v) == 1;
}

#endif /* __ARCH_I960_ATOMIC __ */
