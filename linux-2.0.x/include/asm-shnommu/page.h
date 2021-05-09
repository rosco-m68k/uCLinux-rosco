#ifndef _SH7615_PAGE_H
#define _SH7615_PAGE_H

/*
 * Modification History: 
 *                  Posh2 - This file does not contain any * assembly 
 *                  instructions as such.  But this has to be modified
 *                  after it is linked properly,
 *
 */
#include <linux/config.h>
#include <asm/shglcore.h>

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12UL
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__

#define STRICT_MM_TYPECHECKS

#ifdef STRICT_MM_TYPECHECKS
/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd[16]; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((&x)->pmd[0])
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) } )
#define __pmd(x)	((pmd_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

#else
/*
 * .. while these make it easier on the compiler
 */
typedef unsigned long pte_t;
typedef struct { unsigned long pmd[16]; } pmd_t;
typedef unsigned long pgd_t;
typedef unsigned long pgprot_t;

#define pte_val(x)	(x)
#define pmd_val(x)	((&x)->pmd[0])
#define pgd_val(x)	(x)
#define pgprot_val(x)	(x)

#define __pte(x)	(x)
#define __pmd(x)	((pmd_t) { (x) } )
#define __pgd(x)	(x)
#define __pgprot(x)	(x)

#endif

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/* This handles the memory map.. */

#define PAGE_OFFSET		0x4000000 /*Posh2- This to be corrected with linker script*/  

#define MAP_NR(addr)		((((unsigned long)(addr)) - PAGE_OFFSET) >> PAGE_SHIFT)

#endif /* __KERNEL__ */

#endif /* _SH7615_PAGE_H */
