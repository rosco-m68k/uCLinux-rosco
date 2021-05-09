/*
 * linux/include/asm-arm/arch-ebsa/processor.h
 *
 * Copyright (C) 1996 Russell King
 */

#ifndef __ASM_ARCH_PROCESSOR_H
#define __ASM_ARCH_PROCESSOR_H

/*
 * Bus types
 */
#define EISA_bus 0
#define EISA_bus__is_a_macro /* for versions in ksyms.c */
#define MCA_bus 0
#define MCA_bus__is_a_macro /* for versions in ksyms.c */

/*
 * User space: 3GB
 */
#define TASK_SIZE	(0xc0000000UL)

#define INIT_MMAP	{ &init_mm, 0xc0000000, 0xc2000000, PAGE_SHARED, VM_READ | VM_WRITE | VM_EXEC }

#endif
