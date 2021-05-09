#ifndef __ASM_COMPATMAC_H__
#define __ASM_COMPATMAC_H__

#include<asm/bitops.h>

#define test_and_clear_bit(nr,vaddr) \
  (__builtin_constant_p(nr) ? \
   __constant_test_and_clear_bit(nr, vaddr) : \
   __generic_test_and_clear_bit(nr, vaddr))

extern __inline__ int __constant_test_and_clear_bit(int nr, void * vaddr)
{
    char retval;

    __asm__ __volatile__ ("bclr %1,%2; sne %0"
            : "=d" (retval)
            : "di" (nr & 7), "m" (((char *)vaddr)[(nr^31) >> 3]));

    return retval;
}

extern __inline__ int __generic_test_and_clear_bit(int nr, void * vaddr)
{
    char retval;

    __asm__ __volatile__ ("bfclr %2@{%1:#1}; sne %0"
            : "=d" (retval) : "d" (nr^31), "a" (vaddr));

    return retval;
}
#endif
