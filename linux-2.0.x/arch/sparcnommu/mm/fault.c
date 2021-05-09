/* $Id: fault.c,v 1.62 1996/04/25 06:09:26 davem Exp $
 * fault.c:  Page fault handlers for the Sparc.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <asm/head.h>

#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/tasks.h>
#include <linux/signal.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/traps.h>

void window_overflow_fault(void)
{
	send_sig(SIGSEGV, current, 1);
}

void window_underflow_fault(unsigned long sp)
{
	send_sig(SIGSEGV, current, 1);
}

void window_ret_fault(struct pt_regs *regs)
{
	send_sig(SIGSEGV, current, 1);
}

#ifdef MAGIC_ROM_PTR
int is_in_rom(unsigned long addr) {
  if ( addr < 0x20000000 )
    return(1);

  return(0);
}
#endif
