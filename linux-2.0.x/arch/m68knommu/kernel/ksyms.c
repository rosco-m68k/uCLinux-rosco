#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/user.h>
#include <linux/elfcore.h>

#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/irq.h>
#include <asm/semaphore.h>
#ifdef CONFIG_M68360
#include <asm/m68360.h>
#endif

extern char m68k_debug_device[];

extern void dump_thread(struct pt_regs *, struct user *);
extern int dump_fpu(elf_fpregset_t *);

/*
 * libgcc functions - functions that are used internally by the
 * compiler...  (prototypes are not correct though, but that
 * doesn't really matter since they're not versioned).
 */
extern void __gcc_bcmp(void);
extern void __ashldi3(void);
extern void __ashrdi3(void);
extern void __cmpdi2(void);
extern void __divdi3(void);
extern void __divsi3(void);
extern void __lshrdi3(void);
extern void __moddi3(void);
extern void __modsi3(void);
extern void __muldi3(void);
extern void __mulsi3(void);
extern void __negdi2(void);
extern void __ucmpdi2(void);
extern void __udivdi3(void);
extern void __udivmoddi4(void);
extern void __udivsi3(void);
extern void __umoddi3(void);
extern void __umodsi3(void);

#ifdef CONFIG_COLDFIRE
extern unsigned int *dma_device_address;
extern unsigned long dma_base_addr, _ramend;
extern asmlinkage void trap(void);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
extern asmlinkage void mcf_signal_return(void);
extern asmlinkage void mcf_signal_return_end(void);
#endif
extern void *_ramvec;
extern void wake_up_process(struct task_struct * tsk);
extern struct task_struct init_task;
extern struct task_struct *task[NR_TASKS];
extern unsigned int ksize(void *);
#endif

static struct symbol_table arch_symbol_table = {
#include <linux/symtab_begin.h>
	/* platform dependent support */

	X(memcmp),
	X(request_irq),
	X(free_irq),
	X(dump_fpu),
	X(dump_thread),
	X(strnlen),
	X(strstr),
#ifdef CONFIG_M68360
    X(m68360_get_pquicc),
#endif

	/* The following are special because they're not called
	   explicitly (the C compiler generates them).  Fortunately,
	   their interface isn't gonna change any time soon now, so
	   it's OK to leave it out of version control.  */
	XNOVERS(memcpy),
	XNOVERS(memset),

	XNOVERS(__down_failed),
	XNOVERS(__up_wakeup),

        /* gcc lib functions */
	XNOVERS(__gcc_bcmp),
	XNOVERS(__ashldi3),
	XNOVERS(__ashrdi3),
	XNOVERS(__cmpdi2),
	XNOVERS(__divdi3),
	XNOVERS(__divsi3),
	XNOVERS(__lshrdi3),
	XNOVERS(__moddi3),
	XNOVERS(__modsi3),
	XNOVERS(__muldi3),
	XNOVERS(__mulsi3),
	XNOVERS(__negdi2),
	XNOVERS(__ucmpdi2),
	XNOVERS(__udivdi3),
	XNOVERS(__udivmoddi4),
	XNOVERS(__udivsi3),
	XNOVERS(__umoddi3),
	XNOVERS(__umodsi3),

	XNOVERS(is_in_rom),

#ifdef CONFIG_COLDFIRE
	XNOVERS(dma_device_address),
	XNOVERS(dma_base_addr),
	XNOVERS(_ramend),
	XNOVERS(trap),
	XNOVERS(_ramvec),
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	/* We need these for the 2.0.x kernel */
	XNOVERS(mcf_signal_return),
	XNOVERS(mcf_signal_return_end),
#endif
	XNOVERS(task),
	XNOVERS(init_task),
	XNOVERS(wake_up_process),
	XNOVERS(ksize),
#endif

#include <linux/symtab_end.h>
};

void arch_syms_export(void)
{
	register_symtab(&arch_symbol_table);

	/*our_syms_export()*/
}
