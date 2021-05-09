#ifndef _OR32_PGTABLE_H
#define _OR32_PGTABLE_H

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
#endif /* _OR32_PGTABLE_H */
