/*
 *  linux/arch/or32/mm/memory.c
 *
 *  Based on: linux/arch/m68knommu/mm/memory.c
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/malloc.h>

#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/traps.h>

/*
 * The following two routines map from a physical address to a kernel
 * virtual address and vice versa.
 */
unsigned long mm_vtop (unsigned long vaddr)
{
	return vaddr;
}

unsigned long mm_ptov (unsigned long paddr)
{
	return paddr;
}

/*
 * cache_clear() semantics: Clear any cache entries for the area in question,
 * without writing back dirty entries first. This is useful if the data will
 * be overwritten anyway, e.g. by DMA to memory. The range is defined by a
 * _physical_ address.
 */

void cache_clear (unsigned long paddr, int len)
{
}


/*
 * cache_push() semantics: Write back any dirty cache data in the given area,
 * and invalidate the range in the instruction cache. It needs not (but may)
 * invalidate those entries also in the data cache. The range is defined by a
 * _physical_ address.
 */

void cache_push (unsigned long paddr, int len)
{
}


/*
 * cache_push_v() semantics: Write back any dirty cache data in the given
 * area, and invalidate those entries at least in the instruction cache. This
 * is intended to be used after data has been written that can be executed as
 * code later. The range is defined by a _user_mode_ _virtual_ address  (or,
 * more exactly, the space is defined by the %sfc/%dfc register.)
 */

void cache_push_v (unsigned long vaddr, int len)
{
}

unsigned long mm_phys_to_virt (unsigned long addr)
{
    return PTOV (addr);
}

/* Map some physical address range into the kernel address space. The
 * code is copied and adapted from map_chunk().
 */

unsigned long kernel_map(unsigned long paddr, unsigned long size,
			 int nocacheflag, unsigned long *memavailp )
{
	return paddr;
}


void kernel_set_cachemode( unsigned long address, unsigned long size,
						   unsigned cmode )
{
}

#ifdef MAGIC_ROM_PTR
int is_in_rom(unsigned long addr) {
	extern unsigned long    __rom_start, _flashend;

	/* Anything not in operational RAM is returned as in rom! */
	if ((addr >= (unsigned long)&__rom_start) && (addr < (unsigned long)&_flashend))
		return(1);
	return(0);
}
#endif

