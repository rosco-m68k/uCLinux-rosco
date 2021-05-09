/*
 *  linux/arch/m68knommu/kernel/traps.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>
 *  Copyright (C) 2000  Lineo, Inc.  (www.lineo.com) 
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
 *  Feb/1999 - Greg Ungerer (gerg@snapgear.com) changes to support ColdFire.
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
asmlinkage void trap_c(int vector, struct frame *fp);

asmlinkage void buserr_c(struct frame *fp)
{
#if 1
	extern void dump(struct pt_regs *fp);
	printk("%s(%d): BUSS ERROR TRAP\n", __FILE__, __LINE__);
	dump((struct pt_regs *) fp);
#endif

	/* Only set esp0 if coming from user mode */
	if (user_mode(&fp->ptregs)) {
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
  extern void dump(struct pt_regs *fp);

  if (bad_super_trap_count++) {
    while(1);
  }

  printk ("KERNEL: Bad trap from supervisor state, vector=%d\n", vector);
  dump((struct pt_regs *) fp);
  panic ("Trap from supervisor state");
}

asmlinkage void trap_c(int vector, struct frame *fp)
{
	int sig;

	if ((fp->ptregs.sr & PS_S)
	    && ((fp->ptregs.vector) >> 2) == VEC_TRACE
	    && !(fp->ptregs.sr & PS_T)) {
		/* traced a trapping instruction */
		unsigned char *lp = ((unsigned char *)&fp->un.fmt2) + 4;
		current->flags |= PF_DTRACE;
		/* clear the trace bit */
		(*(unsigned short *)lp) &= ~PS_T;
		return;
	} else if (fp->ptregs.sr & PS_S) {
		bad_super_trap(vector, fp);
		return;
	}

	/* send the appropriate signal to the user program */
	switch (vector) {
	    case VEC_ADDRERR:
		sig = SIGBUS;
		break;
	    case VEC_BUSERR:
#ifdef MCF_MEMORY_PROTECT
	    case VEC_RESV1:	/* ColdFire debug module trap */
#endif
		sig = SIGSEGV;
		break;
	    case VEC_ILLEGAL:
	    case VEC_PRIV:
	    case VEC_LINE10:
	    case VEC_LINE11:
	    case VEC_COPROC:
	    case VEC_TRAP2:
	    case VEC_TRAP3:
	    case VEC_TRAP4:
	    case VEC_TRAP5:
	    case VEC_TRAP6:
	    case VEC_TRAP7:
	    case VEC_TRAP8:
	    case VEC_TRAP9:
	    case VEC_TRAP10:
	    case VEC_TRAP11:
	    case VEC_TRAP12:
	    case VEC_TRAP13:
	    case VEC_TRAP14:
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
	    case VEC_TRACE:		/* ptrace single step */
		fp->ptregs.sr &= ~PS_T;
	    case VEC_TRAP15:		/* breakpoint */
		sig = SIGTRAP;
		break;
	    case VEC_TRAP1:		/* gdbserver breakpoint */
		/* kwonsk: is this right? */
		fp->ptregs.pc -= 2;
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
	extern void dump(struct pt_regs *fp);

	if (!(fp->sr & PS_S))
		return;

	console_verbose();
	dump(fp);

	do_exit(SIGSEGV);
}
