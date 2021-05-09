/* $Id: sigcontext.h,v 1.1 2003/02/26 04:13:07 gerg Exp $ */
#ifndef _ASMnios_SIGCONTEXT_H
#define _ASMnios_SIGCONTEXT_H

#include <asm/ptrace.h>


#ifndef __ASSEMBLY__

/* SunOS system call sigstack() uses this arg. */
struct sunos_sigstack {
	unsigned long sig_sp;
	int onstack_flag;
};

/* This is what SunOS does, so shall I. */
struct sigcontext_struct {
	int sigc_onstack;      /* state to restore */
	int sigc_mask;         /* sigmask to restore */
	int sigc_sp;           /* stack pointer */
	int sigc_pc;           /* program counter */
	int sigc_npc;          /* next program counter */
	int sigc_psr;          /* for condition codes etc */
	int sigc_g1;           /* User uses these three registers */
	int sigc_o0;           /* within the trampoline code. */
	int sigc_o7;           /* This is the orig return addr */

};
#endif /* !(__ASSEMBLY__) */

#endif /* !(_ASMsparc_SIGCONTEXT_H) */

