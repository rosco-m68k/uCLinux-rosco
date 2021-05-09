#ifndef _ASM_OR32_ELF_H
#define _ASM_OR32_ELF_H

/*
 * ELF register definitions..
 */

#include <asm/ptrace.h>

typedef unsigned long elf_greg_t;

#define ELF_NGREG 33 /* pc/sr/sp/r2-r31 */

typedef elf_greg_t elf_gregset_t[ELF_NGREG];
typedef struct user_or32fp_struct elf_fpregset_t;

/* Define the ELF target machines */
#define EM_OR32 80	/* OpenRISC 1000 */

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x) == EM_OR32)

/* Max number of setion in one elf file */
#define ELF_SECTION_NB 20
/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB;
#define ELF_ARCH	EM_OR32

#undef USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096

enum reloc_type
{
	R_OR32_NONE = 0,
	R_OR32_32,
	R_OR32_16,
	R_OR32_8,
	R_OR32_CONST,
	R_OR32_CONSTH,
	R_OR32_JUMPTARG,
	R_OR32_max
};

#define ELF_CORE_COPY_REGS(pr_reg, regs)				\
	/* Bleech. */							\
	pr_reg[0] = regs->pc;						\
	pr_reg[1] = regs->sr;						\
	pr_reg[2] = regs->sp;						\
	pr_reg[3] = regs->gprs[0];					\
	pr_reg[4] = regs->gprs[1];					\
	pr_reg[5] = regs->gprs[2];					\
	pr_reg[6] = regs->gprs[3];					\
	pr_reg[7] = regs->gprs[4];					\
	pr_reg[8] = regs->gprs[5];					\
	pr_reg[9] = regs->gprs[6];					\
	pr_reg[10] = regs->gprs[7];					\
	pr_reg[11] = regs->gprs[8];					\
	pr_reg[12] = regs->gprs[9];					\
	pr_reg[13] = regs->gprs[10];					\
	pr_reg[14] = regs->gprs[11];					\
	pr_reg[15] = regs->gprs[12];					\
	pr_reg[16] = regs->gprs[13];					\
	pr_reg[17] = regs->gprs[14];					\
	pr_reg[18] = regs->gprs[15];					\
	pr_reg[19] = regs->gprs[16];					\
	pr_reg[20] = regs->gprs[17];					\
	pr_reg[21] = regs->gprs[18];					\
	pr_reg[22] = regs->gprs[19];					\
	pr_reg[23] = regs->gprs[20];					\
	pr_reg[24] = regs->gprs[21];					\
	pr_reg[25] = regs->gprs[22];					\
	pr_reg[26] = regs->gprs[23];					\
	pr_reg[27] = regs->gprs[24];					\
	pr_reg[28] = regs->gprs[25];					\
	pr_reg[29] = regs->gprs[26];					\
	pr_reg[30] = regs->gprs[27];					\
	pr_reg[31] = regs->gprs[28];					\
	pr_reg[32] = regs->gprs[29];					\
	}

#endif
