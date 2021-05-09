/*
 *  linux/arch/shnommu/kernel/traps.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>
 *                      The Silver Hammer Group, Ltd.
 *
 *  (Blame me for the icky bits -- kja)
 *
 *  Based on:
 *
 *  linux/arch/m68k/kernel/traps.c
 *
 *  Copyright (C) 1993, 1994 by Hamish Macdonald
 *
 *  68040 fixes by Michael Rausch
 *  68040 fixes by Martin Apel
 *  68060 fixes by Roman Hodek
 *  68060 fixes by Jesper Skov
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 *  Feb/1999 - Greg Ungerer (gerg@moreton.com.au) changes to support ColdFire.
 */

/*
 * Sets up all exception vectors
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/a.out.h>
#include <linux/user.h>
#include <linux/string.h>
#include <linux/linkage.h>

#include <asm/setup.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/traps.h>
#include <asm/pgtable.h>
#include <asm/machdep.h>


void trap_init (void)
{
	if (mach_trap_init)
		mach_trap_init();
}

static inline void console_verbose(void)
{
	extern int console_loglevel;
	console_loglevel = 15;
	if (mach_debug_init)
		mach_debug_init();
}


extern void die_if_kernel(char *,struct pt_regs *,int);
asmlinkage void trap_c(int vector, struct pt_regs  *fp);

/*posh2 for traps_c and busserr_c struct frame changed to struct pt_regs*/
asmlinkage void buserr_c(struct  pt_regs *fp)
{
#if 1
	extern void dump(struct pt_regs *fp);
	printk("%s(%d): BUSS ERROR TRAP\n", __FILE__, __LINE__);
	dump((struct pt_regs *) fp);
#endif

	/* Only set esp0 if coming from user mode */
	if (user_mode(fp)) {
		current->tss.esp0 = (unsigned long) fp;
	} else {
		die_if_kernel("Kernel mode BUS error",(struct pt_regs *)fp,0);
	}

	/* insert handler here */
	force_sig(SIGSEGV, current);
}


static int bad_super_trap_count = 0;

void bad_super_trap (int vector, struct frame *fp)
{
  if (bad_super_trap_count++) {
    while(1);
  }

  printk ("KERNEL: Bad trap from supervisor state, vector=%d\n", vector);
  dump((struct pt_regs *) fp);
  panic ("Trap from supervisor state");
}

asmlinkage void trap_c(int vec, struct pt_regs *fp)
{
	int sig;

/*	if ((fp->sr & PS_S)
	    && ((fp->ptregs.vec) >> 2) == VEC_TRACE
	    && !(fp->ptregs.sr & PS_T)) {
		unsigned char *lp = ((unsigned char *)&fp->un.fmt2) + 4;
		current->flags |= PF_DTRACE;
		(*(unsigned short *)lp) &= ~PS_T;
		return;
	} else if (fp->ptregs.sr & PS_S) {
		bad_super_trap(vec, fp);
		return;
	}*/

	/* send the appropriate signal to the user program */
	switch (vec) {
	    case VEC_ADDRERR:
		sig = SIGBUS;
		break;
	    case VEC_BUSERR:
		sig = SIGSEGV;
		break;
	    case VEC_ILLEGAL:
	    case VEC_PRIV:
	    case VEC_LINE10:
	    case VEC_LINE11:
	    case VEC_COPROC:
	    case VEC_TRAP33:
	    case VEC_TRAP34:
	    case VEC_TRAP35:
	    case VEC_TRAP36:
	    case VEC_TRAP37:
	    case VEC_TRAP38:
	    case VEC_TRAP39:
	    case VEC_TRAP40:
	    case VEC_TRAP41:
	    case VEC_TRAP42:
	    case VEC_TRAP43:
	    case VEC_TRAP44:
	    case VEC_TRAP45:
		
	    case VEC_TRAP46:
	    case VEC_TRAP47:
	    case VEC_TRAP48:
	    case VEC_TRAP49:
	    case VEC_TRAP50:
	    case VEC_TRAP51:
	    case VEC_TRAP52:
	    case VEC_TRAP53:
	    case VEC_TRAP54:
	    case VEC_TRAP55:
	    case VEC_TRAP56:
	    case VEC_TRAP57:
	    case VEC_TRAP58:
	
	    case VEC_TRAP59:
	    case VEC_TRAP60:
	    case VEC_TRAP61:
	   /* case VEC_TRAP62:
	    case VEC_TRAP63:
	    case VEC_TRAP64:*/
		sig = SIGILL;
		break;
#ifndef NO_FPU
	    case VEC_FPBRUC:
	    case VEC_FPIR:
	    case VEC_FPDIVZ:
	    case VEC_FPUNDER:
	    case VEC_FPOE:
	    case VEC_FPOVER:
	    case VEC_FPNAN:
		{
		  unsigned char fstate[216];

		  __asm__ __volatile__ ("fsave %0@" : : "a" (fstate) : "memory");
		  /* Set the exception pending bit in the 68882 idle frame */
		  if (*(unsigned short *) fstate == 0x1f38)
		    {
		      fstate[fstate[1]] |= 1 << 3;
		      __asm__ __volatile__ ("frestore %0@" : : "a" (fstate));
		    }
		}
		/* fall through */
#endif
	    case VEC_ZERODIV:
	    case VEC_TRAP:
		sig = SIGFPE;
		break;
	    case VEC_TRACE:	/* ptrace single step */
		fp->sr &= ~PS_T;
	    case VEC_TRAP62:	/* breakpoint posh2 changed from 15 to 63 */
		sig = SIGTRAP;
		break;
	    case VEC_TRAP63:		/* gdbserver breakpoint posh2 chk */
		/* kwonsk: is this right? */
		/*fp->pc -= 2;*/
		sig = SIGTRAP;
		break;
	    default:
		sig = SIGILL;
		break;
	}

	send_sig (sig, current, 1);
}

asmlinkage void set_esp0 (unsigned long ssp)
{
  current->tss.esp0 = ssp;
}

void die_if_kernel (char *str, struct pt_regs *fp, int nr)
{
	if (!(fp->sr & PS_S))
		return;

	console_verbose();
	dump(fp);

	do_exit(SIGSEGV);
}
