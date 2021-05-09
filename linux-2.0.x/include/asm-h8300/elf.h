#ifndef __ASM_H8300H_ELF_H
#define __ASM_H8300H_ELF_H

/*
 * ELF register definitions..
 */

#include <asm/ptrace.h>

typedef unsigned long elf_greg_t;

#define ELF_NGREG 1
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef void elf_fpregset_t;

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x) == EM_H8300H)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB;
#define ELF_ARCH	EM_H8300H

	/* For SVR4/m68k the function pointer to be registered with
	   `atexit' is passed in %a1.  Although my copy of the ABI has
	   no such statement, it is actually used on ASV.  */
#define ELF_PLAT_INIT(_r)

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096

#define ELF_CORE_COPY_REGS(pr_reg, regs)

/* ELFサポートしたら修正必要 */

#endif
