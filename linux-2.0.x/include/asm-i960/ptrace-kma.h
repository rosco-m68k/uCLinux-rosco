/*
 *   FILE: ptrace.h
 * AUTHOR: kma
 *  DESCR: 
 */

#ifndef PTRACE_H
#define PTRACE_H

#ident "$Id: ptrace-kma.h,v 1.1 1999/12/03 06:02:35 gerg Exp $"

/*
 * Always get the AC, PC
 */
struct pt_regs {
	long	PC;
	long	AC;
};

#endif
