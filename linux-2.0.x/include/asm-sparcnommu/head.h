#ifndef __SPARCNOMMU_HEAD_H
#define __SPARCNOMMU_HEAD_H

#define KERNBASE        0x00000000  /* First address the kernel will eventually be */
#define INTS_ENAB        0x01           /* entry.S uses this. */

#define WRITE_PAUSE      nop; nop; nop; /* Have to do this after %wim/%psr chg */

/* Here are some trap goodies */

/* The reset trap */
#define RTRAP(vec) \
	mov 0xfc0, %psr; mov %g0, %tbr; mov %g0, %wim; ba,a (vec);
#define TRAP_SIMPLE(vec) \
	mov %psr, %l0; sethi %hi(vec), %l4; jmp %l4+%lo(vec); nop;

/* Generic trap entry. */
#define TRAP_ENTRY(type, label) \
	rd %psr, %l0; b label; rd %wim, %l3; nop;

/* This is for traps we should NEVER get. */
#define BAD_TRAP(num) \
        rd %psr, %l0; mov num, %l7; b bad_trap_handler; rd %wim, %l3;

/* Software trap for Linux system calls. */
#define LINUX_SYSCALL_TRAP \
        sethi %hi(C_LABEL(sys_call_table)), %l7; \
        or %l7, %lo(C_LABEL(sys_call_table)), %l7; \
        b linux_sparc_syscall; \
        rd %psr, %l0;

/* The Get Condition Codes software trap for userland. */
#define GETCC_TRAP \
        b getcc_trap_handler; mov %psr, %l0; nop; nop;

/* The Set Condition Codes software trap for userland. */
#define SETCC_TRAP \
        b setcc_trap_handler; mov %psr, %l0; nop; nop;

/* This is for hard interrupts from level 1-14, 15 is non-maskable (nmi) and
 * gets handled with another macro.
 */
#define TRAP_ENTRY_INTERRUPT(int_level) \
        mov int_level, %l7; rd %psr, %l0; b real_irq_entry; rd %wim, %l3;

/* Window overflows/underflows are special and we need to try to be as
 * efficient as possible here....
 */
#define WINDOW_SPILL \
        rd %psr, %l0; rd %wim, %l3; b spill_window_entry; andcc %l0, PSR_PS, %g0;

#define WINDOW_FILL \
        rd %psr, %l0; rd %wim, %l3; b fill_window_entry; andcc %l0, PSR_PS, %g0;

#endif __SPARCNOMMU_HEAD_H
