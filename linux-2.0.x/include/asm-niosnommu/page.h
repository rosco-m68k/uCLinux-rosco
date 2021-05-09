#ifndef _NIOS_PAGE_H
  #define _NIOS_PAGE_H

  #include <asm/nios.h>

  #define PAGE_SHIFT   12
  /* Allow 2MB of RAM for kernel before free page section */
  #define PAGE_OFFSET  (((int)(na_sdram)) + 0x00200000)
  #define PAGE_SIZE    (1 << PAGE_SHIFT)
  #define PAGE_MASK    (~(PAGE_SIZE-1))

  #ifdef __KERNEL__
    #ifndef __ASSEMBLY__

      /* to align the pointer to the (next) page boundary */
      #define PAGE_ALIGN(addr)  (((addr)+PAGE_SIZE-1)&PAGE_MASK)

      /* We now put the free page pool mapped contiguously in high memory above
       * the kernel.
       */
      #define MAP_NR(addr) ((((unsigned long) (addr)) - PAGE_OFFSET) >> PAGE_SHIFT)

    #endif /* !(__ASSEMBLY__) */

  #endif /* __KERNEL__ */

#endif /* _NIOS_PAGE_H */
