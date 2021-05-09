/*
 *  linux/arch/arm/mm/fault.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995, 1996 Russell King
 */

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

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/pgtable.h>

#define FAULT_CODE_FORCECOW	0x80
#define FAULT_CODE_PREFETCH	0x04
#define FAULT_CODE_WRITE	0x02
#define FAULT_CODE_USER		0x01

extern void die_if_kernel(char *, struct pt_regs *, long, int, int);

static void
handle_dataabort (unsigned long addr, int mode, struct pt_regs *regs)
{
    struct task_struct *tsk = current;
    struct mm_struct *mm = tsk->mm;
    struct vm_area_struct *vma;

    down(&mm->mmap_sem);
    vma = find_vma (mm, addr);
    if (!vma)
	goto bad_area;
    if (addr >= vma->vm_start)
	goto good_area;
    if (!(vma->vm_flags & VM_GROWSDOWN) || expand_stack (vma, addr))
	goto bad_area;

    /*
     * Ok, we have a good vm_area for this memory access, so
     * we can handle it..
     */
good_area:
    if (mode & FAULT_CODE_WRITE) { /* write? */
	if (!(vma->vm_flags & VM_WRITE))
	    goto bad_area;
    } else {
	if (!(vma->vm_flags & (VM_READ|VM_EXEC)))
	    goto bad_area;
    }
    /*
     * Dirty hack for now...  Should solve the problem with ldm though!
     */
    handle_mm_fault (vma, addr, mode & (FAULT_CODE_WRITE|FAULT_CODE_FORCECOW));
    up(&mm->mmap_sem);
    return;

    /*
     * Something tried to access memory that isn't in our memory map..
     * Fix it, but check if it's kernel or user first..
     */
bad_area:
    up(&mm->mmap_sem);
    if (mode & FAULT_CODE_USER) {
	tsk->tss.error_code = mode;
	tsk->tss.trap_no = 14;
	printk ("%s: memory violation at pc=0x%08lx, lr=0x%08lx (bad address=0x%08lx, code %d)\n",
		tsk->comm, regs->ARM_pc, regs->ARM_lr, addr, mode);
	force_sig(SIGSEGV, tsk);
	return;
    } else {
	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
    	pgd_t *pgd;
    	pmd_t *pmd;
	if (addr < PAGE_SIZE)
	    printk (KERN_ALERT "Unable to handle kernel NULL pointer dereference");
	else
	    printk (KERN_ALERT "Unable to handle kernel paging request");
	printk (" at virtual address %08lx\n", addr);
	printk (KERN_ALERT "current->tss.memmap = %08lX\n", tsk->tss.memmap);
	pgd = pgd_offset (vma->vm_mm, addr);
	printk (KERN_ALERT "*pgd = %08lx", pgd_val (*pgd));
	if (!pgd_none (*pgd)) {
	    pmd = pmd_offset (pgd, addr);
	    printk (", *pmd = %08lx", pmd_val (*pmd));
	    if (!pmd_none (*pmd))
		printk (", *pte = %08lx", pte_val (*pte_offset (pmd, addr)));
	}
	printk ("\n");
	die_if_kernel ("Oops", regs, mode, 0, SIGKILL);
	do_exit (SIGKILL);
    }
}

/*
 * Handle a data abort.  Note that we have to handle a range of addresses
 * on ARM2/3 for ldm.  If both pages are zero-mapped, then we have to force
 * a copy-on-write
 */
asmlinkage void
do_DataAbort (unsigned long min_addr, unsigned long max_addr, int mode, struct pt_regs *regs)
{
    handle_dataabort (min_addr, mode, regs);

    if ((min_addr ^ max_addr) >> PAGE_SHIFT)
    	handle_dataabort (max_addr, mode | FAULT_CODE_FORCECOW, regs);
}

asmlinkage int
do_PrefetchAbort (unsigned long addr, int mode, struct pt_regs *regs)
{
#if 0
    if (the memc mapping for this page exists - can check now...) {
	printk ("Page in, but got abort (undefined instruction?)\n");
	return 0;
    }
#endif
    handle_dataabort (addr, mode, regs);
    return 1;
}
