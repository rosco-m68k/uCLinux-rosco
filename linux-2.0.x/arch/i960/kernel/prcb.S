#include <linux/linkage.h>
/* initial PRCB  */

	.globl	_prcb
	.align	4 /* or .align 2 */

_prcb:
	.word	boot_flt_table		#  0 - Fault Table
	.word	_boot_control_table	#  4 - Control Table
	.word	0x00001000		#  8 - AC reg mask overflow fault
	.word	0x40000001		# 12 - Flt CFG- Allow Unaligned
	.word	boot_intr_table		# 16 - Interrupt Table
	.word	sys_proc_table		# 20 - System Procedure Table
	.word	0			# 24 - Reserved
	.word	_intr_stack		# 28 - Interrupt Stack Pointer
	.word	0x00000000		# 32 - Inst. Cache - enable cache
	.word	0x5			# 36 - Register Cache Config.- 5 sets cached

/* ROM system procedure table */

	.equ	supervisor_proc, 2
	.text
	.align	6 /* or .align 2 or .align 4 */

sys_proc_table:
	.space	12				# Reserved
	.word	_supervisor_stack		# Supervisor stack pointer
	.space	32				# Preserved
	.word	SYMBOL_NAME(system_call)	# sysproc 0
	.space	258*4				# sysproc 1-258

/* Fault Table */

	.equ	syscall, 2
	.equ	fault_proc, 7
	.text
	.align	4

boot_flt_table:

	.word	(fault_proc<<2) + syscall    # 0-Parallel Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 1-Trace Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 2-Operation Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 3-Arithmetic Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 4-Reserved
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 5-Constraint Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 6-Reserved
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 7-Protection Fault
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 8-Reserved
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 9-Reserved
	.word	0x27f
	.word	(fault_proc<<2) + syscall    # 0xa-Type Fault
	.word	0x27f
	.space	21*8                         # reserved

/* Boot Interrupt Table */

	.text

boot_intr_table:

	.word	0
	.word	0, 0, 0, 0, 0, 0, 0, 0
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
	.word	_intr, _intr, _intr, _intr, _intr, _intr, _intr, _intr
