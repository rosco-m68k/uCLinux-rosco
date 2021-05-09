#ifndef _ASM_OR32_ATOMIC_H
#define _ASM_OR32_ATOMIC_H

#include <linux/config.h>
#include <asm/system.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

typedef int atomic_t;
 
static __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
        unsigned long flags;
 
        save_flags(flags); cli();
        *v += i;
        restore_flags(flags);
}
 
static __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
        unsigned long flags;
 
        save_flags(flags); cli();
        *v -= i;
        restore_flags(flags);
}
 
static __inline__ int atomic_sub_and_test(atomic_t i, atomic_t *v)
{
        unsigned long flags, result;
 
        save_flags(flags); cli();
        *v -= i;
        result = (*v == 0);
        restore_flags(flags);
        return result;
}
 
static __inline__ void atomic_inc(atomic_t *v)
{
        atomic_add(1, v);
}
 
static __inline__ void atomic_dec(atomic_t *v)
{
        atomic_sub(1, v);
}
 
static __inline__ int atomic_dec_and_test(atomic_t *v)
{
        return atomic_sub_and_test(1, v);
}

#endif /* _ASM_OR32_ATOMIC_H */
