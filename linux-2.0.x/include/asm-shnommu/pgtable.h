#ifndef _SH7615_PGTABLE_H
#define _SH7615_PGTABLE_H

/*
 * Modification History :
 *
 * 13 Aug 2002
 *
 *    posh2
 *     xtern inline void SET_PAGE_DIR(struct task_struct * tsk, pgd_t * pgdir)
 *
 *          In the above function few lines are commented out.
 *          
 *          
 *       extern inline void nocache_page (unsigned long vaddr)
 *       static inline void cache_page (unsigned long vaddr)
 *
 *       These two functions are made dummy as it is in H8
 *       
 *        static inline void __flush_tlb(void)
 *       static inline void __flush_tlb_one(unsigned long addr)
 *
 *       #define set_pte(pteptr, pteval) do{     
 *              ((*(pteptr)) = (pteval));       \
 *                } while(0)
 *
 *       This macro is modified like this
 *       
 *       
 *       #define flush_icache() \
 *       #define flush_icache() \
 *      #define __flush_cache_all()						
 *       #define __flush_cache_030()						
 *             These macros  are  made dummy
 *     extern inline void flush_cache_page(struct vm_area_struct *vma,
 * 				    unsigned long vmaddr)
 *
 *        nline void flush_page_to_ram (unsigned long address)*
 *
 *       This function is made empty
 *          
 *          inline void flush_pages_to_ram (unsigned long address, int n)
 *
 *
 * 		This function is made empty
 *
 */
extern unsigned long mm_vtop(unsigned long addr) __attribute__ ((const));
extern unsigned long mm_ptov(unsigned long addr) __attribute__ ((const));

#define VTOP(addr)  (mm_vtop((unsigned long)(addr)))
#define PTOV(addr)  (mm_ptov((unsigned long)(addr)))


extern inline void flush_cache_mm(struct mm_struct *mm)
{
}

extern inline void flush_cache_range(struct mm_struct *mm,
				     unsigned long start,
				     unsigned long end)
{
}

/* Push the page at kernel virtual address and clear the icache */
extern inline void flush_page_to_ram (unsigned long address)
{
}

/* Push n pages at kernel virtual address and clear the icache */
extern inline void flush_pages_to_ram (unsigned long address, int n)
{
}

#endif /* _SH7615_PGTABLE_H */
