/*
 * linux/arch/arm/lib/getconsdata.c
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <linux/unistd.h>

#define OFF_TSK(n) (unsigned long)&(((struct task_struct *)0)->n)
#define OFF_MM(n) (unsigned long)&(((struct mm_struct *)0)->n)

#ifndef NO_MM
unsigned long tss_memmap = OFF_TSK(tss.memmap);
unsigned long pgd = OFF_MM(pgd);
#endif
unsigned long mm = OFF_TSK(mm);
unsigned long tss_save = OFF_TSK(tss.save);
unsigned long tss_fpesave = OFF_TSK(tss.fpstate.soft.save);
#if defined(CONFIG_CPU_ARM2) || defined(CONFIG_CPU_ARM3)
unsigned long tss_memcmap = OFF_TSK(tss.memcmap);
#endif
