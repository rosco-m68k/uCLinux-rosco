/*
 *  linux/mm/kmalloc.c
 *
 *  Copyright (C) 1994, 1995, 1996 R.M.King, ???, Linus Torvalds
 *
 * Changelog:
 *  4/1/96	RMK	Changed allocation sizes to suite requested
 *			sizes.  On i386, and other 4k page machines,
 *			it should use a around 2/3 of the memory
 *			that it used to.
 *			Allowed recursive mallocing for machines with
 *			rediculously large page sizes to reduce
 *			memory wastage.
 */

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/system.h>
#include <asm/dma.h>

/*
 *
 * A study of the sizes requested of kmalloc in v1.3.35 shows the following pattern:
 *
 *			Req.		------- Allocd -------
 * Size   Times req.	Total(B)	size	 Total	 Pages		Wastage
 *   16		 7	  112		  32	   224	   1		  16
 *   36		 3	  108		  64	   192	   1		  28
 *   56		22	 1232		  64	  1408	   1		   8
 *   68		 3	  204		 128	   384	   1		  60
 *  128		26	 3328		 256	  6656	   2		   0
 *  512		 5	 2560		1020	  5100	   2		 508
 *  516		 4	 2064		1020	  4080	   1		 504
 *  752		 4	 3008		1020	  4080	   1		 268
 * 1024		 1	 1024		2040	  1024	   1		   0
 * 1060		 4	 4240		2040	  8160	   2		 980
 * 2048		 2	 4096		4080	  8160	   2		2032
 * 2956		 1	 2956		4080	  4080	   1		1124
 * 4096		 8	32768		8176	 65408	  16		4080
 * ---------------------------------------------------------------------------
 * 	TOTALS		57700			108956	  32
 *							(128k)
 *
 * On average, the old kmalloc uses twice the amount of memory required with a
 * page size of 4k.  On 32k (which is what I'm dealing with):
 *
 *			Req.		------- Allocd -------
 * Size   Times req.	Total(B)	size	 Total	 Pages		Wastage
 *   16		 7	  112		  32	   224	   1		   8
 *   36		 3	  108		  64	   192	   1		  20
 *   56		22	 1232		  64	  1408	   1		   0
 *   68		 3	  204		 128	   384	   1		  52
 *  128		26	 3328		 256	  6656	   1		 120
 *  512		 5	 2560		1020	  5100	   2		 500
 *  516		 4	 2064		1020	  4080	   1		 496
 *  752		 4	 3008		1020	  4080	   1		 260
 * 1024		 1	 1024		2040	  1024	   1		   0
 * 1060		 4	 4240		2040	  8160	   2		 972
 * 2048		 2	 4096		4080	  8160	   2		2026
 * 2956		 1	 2956		4080	  4080	   1		1116
 * 4096		 8	32768		8176	 65408	   2		4072
 * ---------------------------------------------------------------------------
 * 	TOTALS		57700			108956	  17
 *							(544k)
 *
 * Using the old kmalloc system, this ate up a lot of memory in the rounding
 * up of the sizes.
 * We now use a similar strategy to the old kmalloc, except we allocate in
 * sizes determined by looking at the use of kmalloc.  There is an overhead of
 * 2 words on each malloc for mallocs private data, and 4 words at the beginning
 * of the page/4k block.
 *
 *			Req.		------ Allocd --(pages)
 * Size	  Times req.	Total(B)	size	total	4k  32k		wastage
 *   16		 7	  112		  24	  168	 1 		   0
 *   36		 3	  108		  48	  144	 1		   0
 *   56		22	 1232		  64	 1408	 1		   0
 *   68		 3	  204		  80	  240	 1		   4
 *  128		26	 3328		 136	 3536	 1		   0
 *  512		 5	 2560		 580	 2900	 1   2		  60
 *  516		 4	 2064		 580	 2320	 1		  56
 *  752		 4	 3008		 816	 3264	 1		  56
 * 1024		 1	 1024		1360	 1360	 1		 328
 * 1060		 4	 4240		1360	 5440	 2		2048
 * 2048		 2	 4096		4096	 8192	 2		1140
 * 2956		 1	 2956		4096	 4096	 1		  20
 * 4096		 8	32768		4676	37408	10   2		 582
 * --------------------------------------------------------------------------
 *	TOTALS		57700			70476	24   4
 *						      (96k) (128k)
 *
 */

#define MF_USED	 0xffaa0055
#define MF_DMA   0xff00aa55
#define MF_FREE  0x0055ffaa
#define MF_INUSE 0x00ff55aa

#define PAGE_MALLOC 4096

#undef DEBUG

struct mblk_header {
	unsigned long mb_flags;
	union {
		unsigned long vmb_size;
		struct mblk_header *vmb_next;
	} v;
};
#define mb_size v.vmb_size
#define mb_next v.vmb_next

struct page_header {
	unsigned long       ph_flags;
	struct page_header *ph_next;
	struct mblk_header *ph_free;
	unsigned char       ph_order;
	unsigned char       ph_dma;
	unsigned short      ph_nfree;
};

struct size_descriptor {
	struct page_header *sd_firstfree;
	struct page_header *sd_dmafree;
	struct page_header *sd_used;
	int sd_size;
	int sd_blocks;

	int sd_gfporder;
};

static unsigned long bytes_wanted, bytes_malloced, pages_malloced;
/*
 * These are more suited to the sizes that the kernel will claim.
 * Note the two main ones for 56 and 128 bytes (64 and 136), which
 * get the most use.  gfporder has special values:
 *  -1 = don't get a free page, but malloc a small page.
 */
static struct size_descriptor sizes[32] = {
	{ NULL, NULL, NULL,     24, 170, -1 }, /* was 32 - shrink so that we have more */
#if 1
	{ NULL, NULL, NULL,     48,  85, -1 }, /* was 64 - shrink so that we have more, however... */
#endif
	{ NULL, NULL, NULL,     64,  63, -1 }, /* contains 56 byte mallocs */
#if 1
	{ NULL, NULL, NULL,     80,  51. -1 }, /* contains 68 byte mallocs + room for expansion */
#endif
	{ NULL, NULL, NULL,    136,  30, -1 }, /* was 128 - swallow up 128B */
	{ NULL, NULL, NULL,    580,   7, -1 }, /* was 512 - swallow up 516B and allow for expansion */
	{ NULL, NULL, NULL,    816,   5, -1 }, /* was 1024 - we get an extra block */
	{ NULL, NULL, NULL,   1360,   3, -1 }, /* was 2048 - we get an extra block */
	{ NULL, NULL, NULL,   4096,   7, 0 }, /* we need this one for mallocing 4k */
	{ NULL, NULL, NULL,   4676,   7, 0 },
	{ NULL, NULL, NULL,   8188,   4, 0 },
	{ NULL, NULL, NULL,  16368,   2, 0 },
	{ NULL, NULL, NULL,  32752,   1, 0 },
	{ NULL, NULL, NULL,  65520,   1, 1 },
	{ NULL, NULL, NULL, 131056,   1, 2 },
	{ NULL, NULL, NULL, 262128,   1, 3 },
	{ NULL, NULL, NULL,      0,   0, 0 },
};

static int num_sizes = 32;

static inline int get_order (size_t size)
{
	int order;

	for (order = 0; sizes[order].sd_size; order ++)
		if (size <= sizes[order].sd_size && sizes[order].sd_gfporder != -3)
			return order;
	return -1;
}

long kmalloc_init (long start_mem, long end_mem)
{
    int i, gfporder, errors = 0;
    /*
     * kmalloc_init is now a bit more intelligent.
     *
     * It now sets up the gfp orders.
     */
#ifndef PAGE_MALLOC
    gfporder = 0;
#else
    gfporder = -1;
#endif
    pages_malloced = bytes_wanted = bytes_malloced = 0;
	
    for (i = 0; i < num_sizes && sizes[i].sd_size; i++) {
	sizes[i].sd_firstfree = NULL;
	sizes[i].sd_dmafree = NULL;
	sizes[i].sd_used = NULL;
	if (gfporder >= 0 && sizes[i].sd_size > (PAGE_SIZE << gfporder))
	    gfporder += 1;
#ifdef PAGE_MALLOC
	if (gfporder < 0 && sizes[i].sd_size >= PAGE_MALLOC)
	    gfporder += 1;
	if (gfporder < 0)
	    sizes[i].sd_blocks = (PAGE_MALLOC - sizeof(struct page_header)) / sizes[i].sd_size;
	else
#endif
	    sizes[i].sd_blocks = ((PAGE_SIZE << gfporder) - sizeof(struct page_header)) /
	    				sizes[i].sd_size;
	sizes[i].sd_gfporder = gfporder;
    }

    for (i = 0; i < num_sizes && sizes[i].sd_size; i++) {
#ifdef PAGE_MALLOC
	if (sizes[i].sd_gfporder < 0) {
	    if ((sizes[i].sd_size * sizes[i].sd_blocks + sizeof (struct page_header))
		<= PAGE_MALLOC)
	     	continue;
	} else
#endif
	{
	    if ((sizes[i].sd_size * sizes[i].sd_blocks + sizeof (struct page_header))
		<= (PAGE_SIZE << sizes[i].sd_gfporder))
		continue;
	}
	printk ("Cannot use order %d (size %d, blocks %d)\n", i, sizes[i].sd_size, sizes[i].sd_blocks);
	errors ++;
    }
    if (errors)
	panic ("This only happens when someone messes with kmalloc");

    return start_mem;
}

/*
 * kmalloc of any size.
 *
 * if size < PAGE_MALLOC, then when we get a PAGE_MALLOC size, we malloc a
 * PAGE_MALLOC block and override the malloc header with our own.
 */
void *kmalloc (size_t size, int priority)
{
    int dma_flag, order, i;
    unsigned long flags;
    struct page_header *ph, **php;
    struct mblk_header *mb;
    struct size_descriptor *sz;

#ifdef DEBUG
    printk (KERN_DEBUG "km: s %4d ", size);
#endif

    {
    	unsigned int realsize = size + sizeof (struct mblk_header);

    	order = 0;
    	sz = sizes;
    	do {
    	    if (realsize <= sz->sd_size)
    		break;
    	    order ++;
    	    sz ++;
    	    if (!sz->sd_size) {
		printk ("\n" KERN_ERR "kmalloc of too large a block (%d bytes).\n", (int) size);
		return NULL;
	    }
	} while (1);
    }
#ifdef DEBUG
    printk ("o %2d ", order);
#endif

    dma_flag = priority & GFP_DMA;
    priority &= GFP_LEVEL_MASK;

    /* Sanity check... */
    if (intr_count && priority != GFP_ATOMIC) {
	static int count = 0;
	if (++count < 5)
	    printk ("\n" KERN_ERR "kmalloc called non-atomically from interrupt %p\n",
		__builtin_return_address(0));
	priority = GFP_ATOMIC;
    }

    save_flags_cli (flags);

    php = dma_flag ? &sz->sd_dmafree : &sz->sd_firstfree;
again:
    ph = *php;
    if (!ph)
	goto no_free_page;
#ifdef DEBUG
    printk ("ph %p n %p f %p ", ph, ph->ph_next, ph->ph_free);
#endif

    if (ph->ph_flags != MF_INUSE)
	goto major_problem;

    if ((mb = ph->ph_free) != NULL) {
	if (mb->mb_flags != MF_FREE)
	    goto major_problem_2;
	ph->ph_free = mb->mb_next;
	if (--ph->ph_nfree == 0) {
#ifdef DEBUG
	    printk ("nxp %p n %p f %p\n"KERN_DEBUG"    ", ph, ph->ph_next, ph->ph_free);
#endif
	    *php = ph->ph_next;
	    ph->ph_next = sz->sd_used;
	    sz->sd_used = ph;
	}
	mb->mb_flags = MF_USED;
	mb->mb_size = size;
	bytes_wanted += size;
	bytes_malloced += sz->sd_size;
	restore_flags (flags);
#ifdef DEBUG
	printk (" -> %p malloced\n", mb);
#endif
	return mb + 1; /* increments past header */
    } else {
	printk ("\n" KERN_CRIT
		"kmalloc: problem: page %p has null free pointer - "
		"discarding page (pc=%p)\n", ph,
		__builtin_return_address(0));
	if (ph != ph->ph_next)
	    *php = ph->ph_next;
	else
	    *php = NULL;
	goto again;
    }
no_free_page:
    restore_flags (flags);

    /* We need to get a new 4k page.  Whether we get a new page or malloc 4k depends on
     * the page size
     */
#ifdef PAGE_MALLOC
    if (sz->sd_gfporder < 0) { /* malloc it */
#ifdef DEBUG
	printk ("nsp:\n" KERN_DEBUG "  ");
#endif
	mb = kmalloc (PAGE_MALLOC - sizeof (struct mblk_header), priority | dma_flag);
	/*
	 * override malloc header with our own.  This means that we
	 * destroy the size entry.  However, we change the flags entry
	 * so that a free won't free it without the blocks inside it
	 * are freed, and the data put back as it was.
	 */
	if (mb)
	    ph = (struct page_header *) (mb - 1);
	else
	    ph = NULL;
#ifdef DEBUG
	printk (KERN_DEBUG);
#endif
    } else
#endif
    {
	unsigned long max_addr;

	max_addr = dma_flag ? MAX_DMA_ADDRESS : ~0UL;
#ifdef DEBUG
	printk ("nlp:\n" KERN_DEBUG "  ");
#endif
	ph = (struct page_header *) __get_free_pages (priority, sz->sd_gfporder, max_addr);
	if (ph)
	    pages_malloced += PAGE_SIZE;
    }

    if (!ph) {
	static unsigned long last = 0;
	if (priority != GFP_BUFFER && (last + 10*HZ) < jiffies) {
	    last = jiffies;
	    printk ("\n" KERN_CRIT "kmalloc: couldn't get a free page.....\n");
	}
	return NULL;
    }
    ph->ph_flags = MF_INUSE;
    ph->ph_order = order;
    ph->ph_nfree = sz->sd_blocks;
    ph->ph_dma   = dma_flag;

    for (i = sz->sd_blocks, mb = (struct mblk_header *)(ph + 1); i  > 1;
				i --, mb = mb->mb_next) {
	mb->mb_flags = MF_FREE;
	mb->mb_next = (struct mblk_header *) (((unsigned long)mb) + sz->sd_size);
    }
    ph->ph_free = (struct mblk_header *)(ph + 1);
    mb->mb_flags = MF_FREE;
    mb->mb_next = NULL;

    cli();
#ifdef DEBUG
    printk ("New page %p, next %p\n" KERN_DEBUG "                ", ph, *php);
#endif
    ph->ph_next = *php;
    *php = ph;
    goto again;

major_problem_2:
#ifdef DEBUG
    printk ("\n\nmb->flags = %08lX\n", mb->mb_flags);
#endif
    panic ("kmalloc: problem: block %p on freelist %p isn't free (pc=%p)\n",
	ph->ph_free, ph, __builtin_return_address(0));
major_problem:
    panic ("kmalloc: problem: page %p in freelist isn't real (pc=%p)\n",
	ph, __builtin_return_address(0));
}

void kfree (void *ptr)
{
    int order, size;
    unsigned long flags;
    struct page_header *ph;
    struct mblk_header *mb;

    mb = ((struct mblk_header *)ptr) - 1;

    if (mb->mb_flags != MF_USED) {
	printk (KERN_ERR "kfree of non-kmalloc'd memory: %p\n", ptr);
	return;
    }
    size = mb->mb_size;
    order = get_order (size + sizeof(struct mblk_header));

    if (order < 0) {
	printk (KERN_ERR "kfree of non-kmalloc'd memory: %p,"
		" size %d, order %d (pc=%p)\n", ptr, size, order, __builtin_return_address(0));
	return;
    }

#ifdef PAGE_MALLOC
    if (sizes[order].sd_gfporder < 0)
	ph = (struct page_header *) ((((unsigned long) mb) & ~(PAGE_MALLOC - 1)) +
		sizeof (struct page_header));
    else
#endif
	ph = (struct page_header *) (((unsigned long) mb) & PAGE_MASK);
#ifdef DEBUG
    printk (KERN_DEBUG "kfree: page starts at %p\n", ph);
#endif

    if (ph->ph_flags != MF_INUSE && ph->ph_order != order) {
	printk (KERN_ERR "kfree of non-kmalloc'd memory: %p,"
		" size %d, order %d (pc=%p)\n", ptr, size, order, __builtin_return_address(0));
	return;
    }

    mb->mb_flags = MF_FREE;
    save_flags (flags);
    cli ();
    bytes_wanted -= size;
    bytes_malloced -= sizes[order].sd_size;
    mb->mb_next = ph->ph_free;
    ph->ph_free = mb;

    if (++ph->ph_nfree == 1) {
	/*
	 * Page went from full to one free block: put it on the free list.
	 */
	struct page_header *pp;

	if (sizes[order].sd_used == ph)
	    sizes[order].sd_used = ph->ph_next;
	else {
	    for (pp = sizes[order].sd_used; pp != NULL && pp->ph_next != ph; pp = pp->ph_next);

	    if (pp->ph_next == ph)
		pp->ph_next = ph->ph_next;
	    else {
		printk (KERN_ERR "kfree: page %p not found on used list (pc=%p)\n", ph,
			__builtin_return_address(0));
		restore_flags (flags);
		return;
	    }
	}

	ph->ph_next = sizes[order].sd_firstfree;
	sizes[order].sd_firstfree = ph;
    }

    if (ph->ph_nfree == sizes[order].sd_blocks) {
	if (sizes[order].sd_firstfree == ph)
	    sizes[order].sd_firstfree = ph->ph_next;
	else if (sizes[order].sd_dmafree == ph)
	    sizes[order].sd_dmafree = ph->ph_next;
	else {
	    struct page_header *pp;

	    for (pp = sizes[order].sd_firstfree; pp != NULL && pp->ph_next != ph; pp = pp->ph_next);

	    if (pp == NULL)
		for (pp = sizes[order].sd_dmafree; pp != NULL && pp->ph_next != ph; pp = pp->ph_next);

	    if (pp)
		pp->ph_next = ph->ph_next;
	    else
		printk (KERN_ERR "Oops.  Page %p not found on free list\n", ph);
	}
	restore_flags (flags);

#ifdef PAGE_MALLOC
	if (sizes[order].sd_gfporder < 0) {
	    mb = (struct mblk_header *)ph;
	    mb->mb_flags = MF_USED;
	    mb->mb_size = PAGE_MALLOC - sizeof (struct mblk_header);
	    kfree (mb + 1);
	} else
#endif
	{
	    pages_malloced -= PAGE_SIZE;
	    free_pages ((unsigned long)ph, sizes[order].sd_gfporder);
	}
    }
}


void kmalloc_stats (void)
{
    printk ("kmalloc usage: %ld bytes requested of malloc, %ld actually malloced, %ld bytes claimed\n", bytes_wanted,
    		bytes_malloced, pages_malloced);
}

