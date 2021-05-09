#ifndef __NIOS_MMU_CONTEXT_H
#define __NIOS_MMU_CONTEXT_H

/*
 * get a new mmu context.. do we need this on the m68k?
 */
#define get_mmu_context(x) do { } while (0)

/* Initialize the context related info for a new mm_struct
 * instance.
 */
/* vic - this is from sparc, but I don't think it's referenced anywhere */
/* vic - commented out for now ... */
/* vic #define init_new_context(mm) ((mm)->context = NO_CONTEXT) */

#endif /* !(__NIOS_MMU_CONTEXT_H) */
