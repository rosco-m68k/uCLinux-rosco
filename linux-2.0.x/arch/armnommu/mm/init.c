/*
 *  linux/arch/arm/mm/init.c
 *
 *  Copyright (C) 1995, 1996  Russell King
 */

#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/smp.h>
#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#endif

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <asm/hardware.h>
#include <asm/proc/mm-init.h>

unsigned long *empty_zero_page;
unsigned long *empty_bad_page;
unsigned long max_mapnr;

void show_mem(void)
{
	extern void show_net_buffers(void);
	int i,free = 0,total = 0,reserved = 0;
	int shared = 0;

	printk("Mem-info:\n");
	show_free_areas();
#if defined(CONFIG_CPU_ARM2) || defined(CONFIG_CPU_ARM3)
	kmalloc_stats();
#endif
	printk("Free swap:       %6dkB\n",nr_swap_pages<<(PAGE_SHIFT-10));
	i = max_mapnr;
	while (i-- > 0) {
		total++;
		if (PageReserved(mem_map+i))
			reserved++;
		else if (!mem_map[i].count)
			free++;
		else
			shared += mem_map[i].count-1;
	}
	printk("%d pages of RAM\n",total);
	printk("%d free pages\n",free);
	printk("%d reserved pages\n",reserved);
	printk("%d pages shared\n",shared);
	show_buffers();
#ifdef CONFIG_NET
	show_net_buffers();
#endif
}

void si_meminfo(struct sysinfo *val)
{
	int i;

	i = max_mapnr;
	val->totalram = 0;
	val->sharedram = 0;
	val->freeram = nr_free_pages << PAGE_SHIFT;
	val->bufferram = buffermem;
	while (i-- > 0)  {
		if (PageReserved(mem_map+i))
			continue;
		val->totalram++;
		if (!mem_map[i].count)
			continue;
		val->sharedram += mem_map[i].count-1;
	}
	val->totalram <<= PAGE_SHIFT;
	val->sharedram <<= PAGE_SHIFT;
}

extern unsigned long free_area_init(unsigned long, unsigned long);

unsigned long paging_init(unsigned long start_mem, unsigned long end_mem)
{
#ifdef DEBUG
	unsigned long mem_avail = end_mem - start_mem;
	printk ("memory available is %ldKB\n", mem_avail >> 10);
#endif

	/*
	 * virtual address after end of kernel
	 * "availmem" is setup by the code in head.S.
	 */
	/*start_mem = availmem;*/

#ifdef DEBUG
	printk ("start_mem is %#lx\nvirtual_end is %#lx\n",
			start_mem, end_mem);
#endif
	/*
	 * initialize the bad page table and bad page to point
	 * to a couple of allocated pages
	 */
	empty_bad_page = (unsigned long*)start_mem;
	start_mem += PAGE_SIZE;
	empty_zero_page = (unsigned long*)start_mem;
	start_mem += PAGE_SIZE;
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

#ifdef DEBUG
	printk ("before free_area_init\n");

	printk ("free_area_init -> start_mem is %#lx\nvirtual_end is %#lx\n",
			start_mem, end_mem);
#endif

	return PAGE_ALIGN(free_area_init (start_mem, end_mem));
}

/*
 * mem_init() marks the free areas in the mem_map and tells us how much
 * memory is free.  This is done after various parts of the system have
 * claimed their memory after the kernel image.
 */
void mem_init(unsigned long start_mem, unsigned long end_mem)
{
	extern void sound_init(void);
	int codepages = 0;
	int reservedpages = 0;
	int datapages = 0;
	unsigned long tmp;

	/* mark usable pages in the mem_map[] */
	mark_usable_memory_areas(&start_mem, end_mem);

	end_mem &= PAGE_MASK;
	high_memory = end_mem;
	max_mapnr = MAP_NR(end_mem);

	for (tmp = PAGE_OFFSET; tmp < high_memory ; tmp += PAGE_SIZE) {
		extern int etext;
		extern int _stext;
		if (PageReserved(mem_map+MAP_NR(tmp))) {
			if (tmp < KERNTOPHYS(_stext))
				reservedpages++;
			else if (tmp < KERNTOPHYS(etext))
				codepages++;
			else
				datapages++;
			continue;
		}
		mem_map[MAP_NR(tmp)].count = 1;
#ifdef CONFIG_BLK_DEV_INITRD
		if (!initrd_start || (tmp < initrd_start || tmp >= initrd_end))
#endif
			free_page(tmp);
	}
	tmp = nr_free_pages << PAGE_SHIFT;
	printk ("Memory: %luk/%luk available (%dk kernel code, %dk reserved, %dk data)\n",
		 tmp >> 10,
		 ((unsigned long)DRAM_SIZE) >> 10, 
		 codepages << (PAGE_SHIFT-10),
		 reservedpages << (PAGE_SHIFT-10),
		 datapages << (PAGE_SHIFT-10));
}
