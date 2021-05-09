/*
 *  linux/arch/m68knommu/mm/init.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
 *
 *  Based on:
 *
 *  linux/arch/m68k/mm/init.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 *
 *  JAN/1999 -- hacked to support ColdFire (gerg@moreton.com.au)
 */

#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#ifdef CONFIG_BLK_DEV_RAM
#include <linux/blk.h>
#endif

#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>

#ifndef PAGE_OFFSET
#define PAGE_OFFSET 0
#endif

extern void die_if_kernel(char *,struct pt_regs *,long);
extern void show_net_buffers(void);

/*
 * BAD_PAGE is the page that is used for page faults when linux
 * is out-of-memory. Older versions of linux just did a
 * do_exit(), but using this instead means there is less risk
 * for a process dying in kernel mode, possibly leaving a inode
 * unused etc..
 *
 * BAD_PAGETABLE is the accompanying page-table: it is initialized
 * to point to BAD_PAGE entries.
 *
 * ZERO_PAGE is a special page that is used for zero-initialized
 * data and COW.
 */
static unsigned long empty_bad_page_table;

static unsigned long empty_bad_page;

unsigned long empty_zero_page;

unsigned long rom_length;

void show_mem(void)
{
    unsigned long i;
    int free = 0, total = 0, reserved = 0, nonshared = 0, shared = 0;

    printk("\nMem-info:\n");
    show_free_areas();
    printk("Free swap:       %6dkB\n",nr_swap_pages<<(PAGE_SHIFT-10));
    i = (high_memory - PAGE_OFFSET) >> PAGE_SHIFT;
    while (i-- > 0) {
	total++;
	if (PageReserved(mem_map+i))
	    reserved++;
	else if (!mem_map[i].count)
	    free++;
	else if (mem_map[i].count == 1)
	    nonshared++;
	else
	    shared += mem_map[i].count-1;
    }
    printk("%d pages of RAM\n",total);
    printk("%d free pages\n",free);
    printk("%d reserved pages\n",reserved);
    printk("%d pages nonshared\n",nonshared);
    printk("%d pages shared\n",shared);
    show_buffers();
#ifdef CONFIG_NET
    show_net_buffers();
#endif
}

extern unsigned long free_area_init(unsigned long, unsigned long);

/*
 * paging_init() continues the virtual memory environment setup which
 * was begun by the code in arch/head.S.
 * The parameters are pointers to where to stick the starting and ending
 * addresses  of available kernel virtual memory.
 */
unsigned long paging_init(unsigned long start_mem, unsigned long end_mem)
{
#ifdef DEBUG
	printk ("start_mem is %#lx\nvirtual_end is %#lx\n",
		start_mem, end_mem);
#endif

	/*
	 * initialize the bad page table and bad page to point
	 * to a couple of allocated pages
	 */
	empty_bad_page_table = start_mem;
	start_mem += PAGE_SIZE;
	empty_bad_page = start_mem;
	start_mem += PAGE_SIZE;
	empty_zero_page = start_mem;
	start_mem += PAGE_SIZE;
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	/*
	 * Set up SFC/DFC registers (user data space)
	 */
	set_fs (USER_DS);

#ifdef DEBUG
	printk ("before free_area_init\n");

	printk ("free_area_init -> start_mem is %#lx\nvirtual_end is %#lx\n",
		start_mem, end_mem);
#endif

	return PAGE_ALIGN(free_area_init (start_mem, end_mem));
}

void mem_init(unsigned long start_mem, unsigned long end_mem)
{
	int codek = 0;
	int datapages = 0;
	unsigned long tmp;
#if defined( CONFIG_COLDFIRE ) || defined( CONFIG_EXCALIBUR )
	extern char _etext, _stext, __data_start;
#else
	extern char _etext, _romvec, __data_start;
#endif
	unsigned long len = end_mem-(unsigned long)&__data_start;

	/* Bloody watchdog... */
#ifdef CONFIG_SHGLCORE
       (*((volatile unsigned char*)0xFFFA21)) = 128 | 64/* | 32 | 16*/;
       (*((volatile unsigned short*)0xFFFA24)) &= ~512;
       (*((volatile unsigned char*)0xFFFA27)) = 0x55;
       (*((volatile unsigned char*)0xFFFA27)) = 0xAA;
       
       /*printk("Initiated watchdog, SYPCR = %x\n", *(volatile char*)0xFFFA21);*/
#endif	                

#ifdef DEBUG
	printk("Mem_init: start=%lx, end=%lx\n", start_mem, end_mem);
#endif

	end_mem &= PAGE_MASK;
	high_memory = end_mem;

	start_mem = PAGE_ALIGN(start_mem);
	while (start_mem < high_memory) {
		clear_bit(PG_reserved, &mem_map[MAP_NR(start_mem)].flags);
		start_mem += PAGE_SIZE;
	}

	for (tmp = PAGE_OFFSET ; tmp < end_mem ; tmp += PAGE_SIZE) {

#ifdef MAX_DMA_ADDRESS
		if (VTOP (tmp) >= MAX_DMA_ADDRESS)
			clear_bit(PG_DMA, &mem_map[MAP_NR(tmp)].flags);
#endif

		if (PageReserved(mem_map+MAP_NR(tmp))) {
			datapages++;
			continue;
		}
		mem_map[MAP_NR(tmp)].count = 1;
#ifdef CONFIG_BLK_DEV_INITRD
		if (!initrd_start ||
		    (tmp < (initrd_start & PAGE_MASK) || tmp >= initrd_end))
#endif
			free_page(tmp);
	}
	
#if defined( CONFIG_COLDFIRE ) || defined( CONFIG_EXCALIBUR )
	codek = (&_etext - &_stext) >> 10;
#else
	codek = (&_etext - &_romvec) >> 10;
#endif
	tmp = nr_free_pages << PAGE_SHIFT;
	printk("Memory available: %luk/%luk RAM, %luk/%luk ROM (%dk kernel data, %dk code)\n",
	       tmp >> 10,
	       len >> 10,
	       (rom_length > 0) ? ((rom_length >> 10) - codek) : 0,
	       rom_length >> 10,
	       datapages << (PAGE_SHIFT-10),
	       codek
	       );
}

void si_meminfo(struct sysinfo *val)
{
    unsigned long i;

    i = (high_memory - PAGE_OFFSET) >> PAGE_SHIFT;
    val->totalram = 0;
    val->sharedram = 0;
    val->freeram = nr_free_pages << PAGE_SHIFT;
    val->bufferram = buffermem;
    while (i-- > 0) {
	if (PageReserved(mem_map+i))
	    continue;
	val->totalram++;
	if (!mem_map[i].count)
	    continue;
	val->sharedram += mem_map[i].count-1;
    }
    val->totalram <<= PAGE_SHIFT;
    val->sharedram <<= PAGE_SHIFT;
    return;
}

