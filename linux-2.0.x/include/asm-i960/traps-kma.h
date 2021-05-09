/*
 *   FILE: traps.h
 * AUTHOR: kma
 *  DESCR: frame, pt_regs, etc.
 */

#ifndef TRAPS_H
#define TRAPS_H

#ident "$Id: traps-kma.h,v 1.1 1999/12/03 06:02:35 gerg Exp $"

#include <asm/ptrace.h>
#include <asm/i960.h>

/* structure of stack frame: note that since the i960 stacks grow up,
 * and we store our regs on the top, ptregs is at the end of each
 * member of the union.
 */

struct frame {
	union {
		struct pt_regs regs;
	};
};

#endif
