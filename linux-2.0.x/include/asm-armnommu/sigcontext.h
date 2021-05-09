#ifndef _ASMARM_SIGCONTEXT_H
#define _ASMARM_SIGCONTEXT_H

#include <asm/ptrace.h>

struct sigcontext_struct {
	unsigned long magic;
	struct pt_regs reg;
	unsigned long trap_no;
	unsigned long error_code;
	unsigned long oldmask;
};

#endif
