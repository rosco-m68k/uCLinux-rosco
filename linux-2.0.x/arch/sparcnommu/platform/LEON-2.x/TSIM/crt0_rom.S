/*
 * crt0_$(MODEL).S for LEON
 *
 * Originally boot.S from the leon distribution, this has been merged with
 * head.S from the Linux/Sparc distribution.
 *
 * Copyright (c) 2001 by The LEOX team <team@leox.org>
 * Copyright (c) 2000 D. Jeff Dionne, Lineo Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/version.h>
#include <linux/config.h>

#include <asm/leon.h>
#include "bootcfg.h"

#include <asm/cprefix.h>
#include <asm/head.h>
#include <asm/asi.h>
#include <asm/contregs.h>
#include <asm/ptrace.h>
#include <asm/psr.h>
#include <asm/page.h>
#include <asm/winmacro.h>
 
#define PSR_INIT   0x10c0	/* Disable traps, set s and ps */
#define WIM_INIT   2

WINDOWSIZE = (16 * 4)
ARGPUSHSIZE = (6 * 4)
ARGPUSH = (WINDOWSIZE + 4)
MINFRAME = (WINDOWSIZE + ARGPUSHSIZE + 4)
#define STACK_ALIGN	8
#define SA(X)	(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))

#define WTRAP(H)  mov %psr, %l0; sethi %hi(H), %l4; jmp %l4+%lo(H); nop;

	.section .romvec

t_rst:	RTRAP(hardreset)                    /* reset trap                    */
t_tflt:	BAD_TRAP(0x1)                       /* Inst. Access Exception        */
t_bins:	TRAP_ENTRY(0x2, bad_instruction)    /* Illegal Instruction           */
t_pins:	TRAP_ENTRY(0x3, priv_instruction)   /* Privileged Instruction        */
t_fpd:	TRAP_SIMPLE(fpu_dis)                /* Floating Point Disabled       */
t_wovf:	WINDOW_SPILL                        /* Window Overflow               */
t_wunf:	WINDOW_FILL                         /* Window Underflow              */
t_mna:	TRAP_ENTRY(0x7, mna_handler)        /* Memory Address Not Aligned    */
t_fpe:	TRAP_ENTRY(0x8, fpe_trap_handler)   /* Floating Point Exception      */
t_dflt:	BAD_TRAP(0x9)                       /* Data Miss Exception           */
t_tio:	BAD_TRAP(0xa)                       /* Tagged Instruction Ovrflw     */
t_wpt:	BAD_TRAP(0xb)                       /* Watchpoint Detected           */
t_badc:	BAD_TRAP(0xc) BAD_TRAP(0xd) BAD_TRAP(0xe) BAD_TRAP(0xf) BAD_TRAP(0x10)
t_irq1:	TRAP_ENTRY_INTERRUPT(1)             /* IRQ Software/SBUS Level 1     */
t_irq2:	TRAP_ENTRY_INTERRUPT(2)             /* IRQ SBUS Level 2              */
t_irq3:	TRAP_ENTRY_INTERRUPT(3)             /* IRQ SCSI/DMA/SBUS Level 3     */
t_irq4:	TRAP_ENTRY_INTERRUPT(4)             /* IRQ Software Level 4          */
t_irq5:	TRAP_ENTRY_INTERRUPT(5)             /* IRQ SBUS/Ethernet Level 5     */
t_irq6:	TRAP_ENTRY_INTERRUPT(6)             /* IRQ Software Level 6          */
t_irq7:	TRAP_ENTRY_INTERRUPT(7)             /* IRQ Video/SBUS Level 5        */
t_irq8:	TRAP_ENTRY_INTERRUPT(8)             /* IRQ SBUS Level 6              */
t_irq9:	TRAP_ENTRY_INTERRUPT(9)             /* IRQ SBUS Level 7              */
t_irq10:TRAP_ENTRY_INTERRUPT(10)            /* IRQ Timer #1 (one we use)     */
t_irq11:TRAP_ENTRY_INTERRUPT(11)            /* IRQ Floppy Intr.              */
t_irq12:TRAP_ENTRY_INTERRUPT(12)            /* IRQ Zilog serial chip         */
t_irq13:TRAP_ENTRY_INTERRUPT(13)            /* IRQ Audio Intr.               */
t_irq14:TRAP_ENTRY_INTERRUPT(14)            /* IRQ Timer #2                  */
t_nmi:	BAD_TRAP(15)                        /* Level 15 (NMI)                */
t_racc:	TRAP_ENTRY(0x20, do_reg_access)     /* General Register Access Error */
t_iacce:BAD_TRAP(0x21)                      /* Instr Access Error            */
t_bad22:BAD_TRAP(0x22) BAD_TRAP(0x23)
t_cpdis:TRAP_ENTRY(0x24, do_cp_disabled)    /* Co-Processor Disabled         */
t_uflsh:TRAP_ENTRY(0x25, do_bad_flush)      /* Unimplemented FLUSH inst.     */
t_bad26:BAD_TRAP(0x26) BAD_TRAP(0x27)
t_cpexc:TRAP_ENTRY(0x28, do_cp_exception)   /* Co-Processor Exception        */
t_dacce:BAD_TRAP(0x29)                       /* Data Access Error             */
t_hwdz:	TRAP_ENTRY(0x2a, do_hw_divzero)     /* Division by zero, you lose... */
t_dserr:BAD_TRAP(0x2b)                      /* Data Store Error              */
t_daccm:BAD_TRAP(0x2c)                      /* Data Access MMU-Miss          */
t_bad2d:BAD_TRAP(0x2d) BAD_TRAP(0x2e) BAD_TRAP(0x2f) BAD_TRAP(0x30) BAD_TRAP(0x31)
t_bad32:BAD_TRAP(0x32) BAD_TRAP(0x33) BAD_TRAP(0x34) BAD_TRAP(0x35) BAD_TRAP(0x36)
t_bad37:BAD_TRAP(0x37) BAD_TRAP(0x38) BAD_TRAP(0x39) BAD_TRAP(0x3a) BAD_TRAP(0x3b)
t_iaccm:BAD_TRAP(0x3c)                      /* Instr Access MMU-Miss         */
t_bad3d:BAD_TRAP(0x3d) BAD_TRAP(0x3e) BAD_TRAP(0x3f) BAD_TRAP(0x40) BAD_TRAP(0x41)
t_bad42:BAD_TRAP(0x42) BAD_TRAP(0x43) BAD_TRAP(0x44) BAD_TRAP(0x45) BAD_TRAP(0x46)
t_bad47:BAD_TRAP(0x47) BAD_TRAP(0x48) BAD_TRAP(0x49) BAD_TRAP(0x4a) BAD_TRAP(0x4b)
t_bad4c:BAD_TRAP(0x4c) BAD_TRAP(0x4d) BAD_TRAP(0x4e) BAD_TRAP(0x4f) BAD_TRAP(0x50)
t_bad51:BAD_TRAP(0x51) BAD_TRAP(0x52) BAD_TRAP(0x53) BAD_TRAP(0x54) BAD_TRAP(0x55)
t_bad56:BAD_TRAP(0x56) BAD_TRAP(0x57) BAD_TRAP(0x58) BAD_TRAP(0x59) BAD_TRAP(0x5a)
t_bad5b:BAD_TRAP(0x5b) BAD_TRAP(0x5c) BAD_TRAP(0x5d) BAD_TRAP(0x5e) BAD_TRAP(0x5f)
t_bad60:BAD_TRAP(0x60) BAD_TRAP(0x61) BAD_TRAP(0x62) BAD_TRAP(0x63) BAD_TRAP(0x64)
t_bad65:BAD_TRAP(0x65) BAD_TRAP(0x66) BAD_TRAP(0x67) BAD_TRAP(0x68) BAD_TRAP(0x69)
t_bad6a:BAD_TRAP(0x6a) BAD_TRAP(0x6b) BAD_TRAP(0x6c) BAD_TRAP(0x6d) BAD_TRAP(0x6e)
t_bad6f:BAD_TRAP(0x6f) BAD_TRAP(0x70) BAD_TRAP(0x71) BAD_TRAP(0x72) BAD_TRAP(0x73)
t_bad74:BAD_TRAP(0x74) BAD_TRAP(0x75) BAD_TRAP(0x76) BAD_TRAP(0x77) BAD_TRAP(0x78)
t_bad79:BAD_TRAP(0x79) BAD_TRAP(0x7a) BAD_TRAP(0x7b) BAD_TRAP(0x7c) BAD_TRAP(0x7d)
t_bad7e:BAD_TRAP(0x7e) BAD_TRAP(0x7f)
t_sunos:BAD_TRAP(0x80)                      /* SunOS System Call             */
t_sbkpt:BAD_TRAP(0x81)                      /* Software Breakpoint/KGDB      */
t_divz:	BAD_TRAP(0x82)                      /* Divide by zero trap           */
t_flwin:WTRAP(_flush_windows)               /* Flush Windows Trap            */
t_clwin:BAD_TRAP(0x84)                      /* Clean Windows Trap            */
t_rchk:	BAD_TRAP(0x85)                      /* Range Check                   */
t_funal:BAD_TRAP(0x86)                      /* Fix Unaligned Access Trap     */
t_iovf:	BAD_TRAP(0x87)                      /* Integer Overflow Trap         */
t_slowl:BAD_TRAP(0x88)                      /* Slowaris System Call          */
t_netbs:BAD_TRAP(0x89)                      /* Net-B.S. System Call          */
t_bad8a:BAD_TRAP(0x8a) BAD_TRAP(0x8b) BAD_TRAP(0x8c) BAD_TRAP(0x8d) BAD_TRAP(0x8e)
t_bad8f:BAD_TRAP(0x8f)
t_linux:LINUX_SYSCALL_TRAP                  /* Linux System Call             */
t_bad91:BAD_TRAP(0x91) BAD_TRAP(0x92) BAD_TRAP(0x93) BAD_TRAP(0x94) BAD_TRAP(0x95)
t_bad96:BAD_TRAP(0x96) BAD_TRAP(0x97) BAD_TRAP(0x98) BAD_TRAP(0x99) BAD_TRAP(0x9a)
t_bad9b:BAD_TRAP(0x9b) BAD_TRAP(0x9c) BAD_TRAP(0x9d) BAD_TRAP(0x9e) BAD_TRAP(0x9f)
t_getcc:GETCC_TRAP                          /* Get Condition Codes           */
t_setcc:SETCC_TRAP                          /* Set Condition Codes           */
t_bada2:BAD_TRAP(0xa2) BAD_TRAP(0xa3) BAD_TRAP(0xa4) BAD_TRAP(0xa5) BAD_TRAP(0xa6)
t_bada7:BAD_TRAP(0xa7) BAD_TRAP(0xa8) BAD_TRAP(0xa9) BAD_TRAP(0xaa) BAD_TRAP(0xab)
t_badac:BAD_TRAP(0xac) BAD_TRAP(0xad) BAD_TRAP(0xae) BAD_TRAP(0xaf) BAD_TRAP(0xb0)
t_badb1:BAD_TRAP(0xb1) BAD_TRAP(0xb2) BAD_TRAP(0xb3) BAD_TRAP(0xb4) BAD_TRAP(0xb5)
t_badb6:BAD_TRAP(0xb6) BAD_TRAP(0xb7) BAD_TRAP(0xb8) BAD_TRAP(0xb9) BAD_TRAP(0xba)
t_badbb:BAD_TRAP(0xbb) BAD_TRAP(0xbc) BAD_TRAP(0xbd) BAD_TRAP(0xbe) BAD_TRAP(0xbf)
t_badc0:BAD_TRAP(0xc0) BAD_TRAP(0xc1) BAD_TRAP(0xc2) BAD_TRAP(0xc3) BAD_TRAP(0xc4)
t_badc5:BAD_TRAP(0xc5) BAD_TRAP(0xc6) BAD_TRAP(0xc7) BAD_TRAP(0xc8) BAD_TRAP(0xc9)
t_badca:BAD_TRAP(0xca) BAD_TRAP(0xcb) BAD_TRAP(0xcc) BAD_TRAP(0xcd) BAD_TRAP(0xce)
t_badcf:BAD_TRAP(0xcf) BAD_TRAP(0xd0) BAD_TRAP(0xd1) BAD_TRAP(0xd2) BAD_TRAP(0xd3)
t_badd4:BAD_TRAP(0xd4) BAD_TRAP(0xd5) BAD_TRAP(0xd6) BAD_TRAP(0xd7) BAD_TRAP(0xd8)
t_badd9:BAD_TRAP(0xd9) BAD_TRAP(0xda) BAD_TRAP(0xdb) BAD_TRAP(0xdc) BAD_TRAP(0xdd)
t_badde:BAD_TRAP(0xde) BAD_TRAP(0xdf) BAD_TRAP(0xe0) BAD_TRAP(0xe1) BAD_TRAP(0xe2)
t_bade3:BAD_TRAP(0xe3) BAD_TRAP(0xe4) BAD_TRAP(0xe5) BAD_TRAP(0xe6) BAD_TRAP(0xe7)
t_bade8:BAD_TRAP(0xe8) BAD_TRAP(0xe9) BAD_TRAP(0xea) BAD_TRAP(0xeb) BAD_TRAP(0xec)
t_baded:BAD_TRAP(0xed) BAD_TRAP(0xee) BAD_TRAP(0xef) BAD_TRAP(0xf0) BAD_TRAP(0xf1)
t_badf2:BAD_TRAP(0xf2) BAD_TRAP(0xf3) BAD_TRAP(0xf4) BAD_TRAP(0xf5) BAD_TRAP(0xf6)
t_badf7:BAD_TRAP(0xf7) BAD_TRAP(0xf8) BAD_TRAP(0xf9) BAD_TRAP(0xfa) BAD_TRAP(0xfb)
t_badfc:BAD_TRAP(0xfc) BAD_TRAP(0xfd)
dbtrap:	BAD_TRAP(0xfe)                      /* Debugger/PROM breakpoint #1   */
dbtrap2:BAD_TRAP(0xff)                      /* Debugger/PROM breakpoint #2   */	

	.globl	C_LABEL(end_traptable)
C_LABEL(end_traptable):

/*
 * Startup code for standalone system. Wash IU and FPU (if present) registers.
 */

hardreset:

	mov	%g0, %y
	set	0x80000000, %g1		! LEON register base
	clr	%g2
	clr	%g3
	clr	%g4
	clr	%g5
	clr	%g6
!	clr	%g7

  	st	%g0, [%g1 + FADDR]
  	st	%g0, [%g1 + MSTAT]
  	st	%g0, [%g1 + WPROT1]
  	st	%g0, [%g1 + WPROT2]
  	st	%g0, [%g1 + TCNT0]
  	st	%g0, [%g1 + TRLD0]
  	st	%g0, [%g1 + TCTRL0]
  	st	%g0, [%g1 + TCNT1]
  	st	%g0, [%g1 + TRLD1]
  	st	%g0, [%g1 + TCTRL1]
  	st	%g0, [%g1 + IPEND]

	set	MEMCFG1_VAL, %g2	! init memory config register 1
  	st	%g2, [%g1 + MCFG1]
	set	MEMCFG2_VAL, %g2	! init memory config register 2
  	st	%g2, [%g1 + MCFG2]
	set	0x0001000f, %g2		! Turn on the cache
	st	%g2, [%g1 + CCTRL]
	set	TIMER_SCALER_VAL, %g2	! init timer scaler register
  	st	%g2, [%g1 + SRLD]
  	st	%g2, [%g1 + SCNT]
	set	UART_SCALER_VAL, %g2	! init UART scaler register
  	st	%g2, [%g1 + USCAL0]
  	st	%g2, [%g1 + USCAL1]
	set	7, %g2			! Enable uarts
  	st	%g2, [%g1 + UCTRL0]
  	st	%g2, [%g1 + UCTRL1]
	set	0xaa00, %g2
  	st	%g2, [%g1 + IOREG]
  	st	%g2, [%g1 + IODIR]

	set	0x61, %g2               ! ##### Output an 'a'
	st	%g2, [%g1 + UDATA0]

fixiu:
        set 	0x01FE0,%g3
	mov	%g3, %psr		! Set valid PSR
	nop; nop; nop

	set	8,%g2
wl0:
	clr	%i0
	clr	%i1
	clr	%i2
	clr	%i3
	clr	%i4
	clr	%i5
	clr	%i6
	clr	%i7
	clr	%l0
	clr	%l1
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	save
	subcc	%g2, 1, %g2
	bne	wl0
	nop

Ch1:
	ld	[%g1 + USTAT0], %g2
	and	%g2, 0x04, %g3
	cmp	%g3, 0
	beq	Ch1
	nop
	set	0x43, %g2
	st	%g2, [%g1 + UDATA0]

	set	WIM_INIT, %g2
	mov	%g2, %wim
	nop; nop; nop

#ifdef CHECKFPU
	or	%g0, 1, %o0
	ld	[%g0], %f0		! Check if FPU is present

	tst	%o0
	bnz	fixfpu
	nop

	ba    _sysini

! Wash FPU register file
fixfpu:

	ld	[%g0], %f0
	ld	[%g0], %f1
	ld	[%g0], %f2
	ld	[%g0], %f3
	ld	[%g0], %f4
	ld	[%g0], %f5
	ld	[%g0], %f6
	ld	[%g0], %f7
	ld	[%g0], %f8
	ld	[%g0], %f9
	ld	[%g0], %f10
	ld	[%g0], %f11
	ld	[%g0], %f12
	ld	[%g0], %f13
	ld	[%g0], %f14
	ld	[%g0], %f15
	ld	[%g0], %f16
	ld	[%g0], %f17
	ld	[%g0], %f18
	ld	[%g0], %f19
	ld	[%g0], %f20
	ld	[%g0], %f21
	ld	[%g0], %f22
	ld	[%g0], %f23
	ld	[%g0], %f24
	ld	[%g0], %f25
	ld	[%g0], %f26
	ld	[%g0], %f27
	ld	[%g0], %f28
	ld	[%g0], %f29
	ld	[%g0], %f30
	ld	[%g0], %f31

! System init

#endif /* CHECKFPU */

_sysini:

  	set	0x80000000, %g1
	mov	%g1, %g7

Ch2:
	ld	[%g1 + USTAT0], %g2
	and	%g2, 0x04, %g3
	cmp	%g3, 0
	beq	Ch2
	nop
	set	0x44, %g2
	st	%g2, [%g1 + UDATA0]

_init:

	set	STACK_INIT, %sp
        set     PSR_INIT, %g1
        wr      %g1, 0x20, %psr		! enable traps
        nop; nop; nop;

Ch3:
	ld	[%g7 + USTAT0], %g2
	and	%g2, 0x04, %g3
	cmp	%g3, 0
	beq	Ch3
	nop
	set	0x47, %g2
	st	%g2, [%g7 + UDATA0]

	call	main
        sub     %sp, 0x40, %sp  	! room for main to save args
        nop

        mov     %g0, %psr		! enable traps
	nop; nop; nop
	ta 	0			! Halt if _main would return ...

___main:
	retl
	nop

! Bad trap
bad_trap_handler:
	ld	[%g7 + USTAT0], %g2
	and	%g2, 0x04, %g3
	cmp	%g3, 0
	beq	bad_trap_handler
	nop
	set	0x54, %g2		! print a 'T'
	st	%g2, [%g7 + UDATA0]
	ba	bad_trap_handler
	nop

! FPU disabled trap address

fpu_dis:
	clr	%i0
	jmpl	%l2, %g0
	rett	%l2 + 4
	nop

/* Number of register windows */
#define NWINDOWS 	8

  	 		!Window overflow trap handler.
  	.global _window_overflow

_window_overflow:

  	mov  %wim, %l3       		! Calculate next WIM
  	mov  %g1, %l7           
  	srl  %l3, 1, %g1        
  	sll  %l3, NWINDOWS-1 , %l4
  	or   %l4, %g1, %g1

  	save                              ! Get into window to be saved.
  	mov  %g1, %wim
  	nop; nop; nop
        st   %l0, [%sp + 0];
        st   %l1, [%sp + 4];
        st   %l2, [%sp + 8];
        st   %l3, [%sp + 12];
        st   %l4, [%sp + 16]; 
        st   %l5, [%sp + 20];
        st   %l6, [%sp + 24]; 
        st   %l7, [%sp + 28]; 
        st   %i0, [%sp + 32]; 
        st   %i1, [%sp + 36]; 
        st   %i2, [%sp + 40]; 
        st   %i3, [%sp + 44]; 
        st   %i4, [%sp + 48]; 
        st   %i5, [%sp + 52]; 
        st   %i6, [%sp + 56]; 
        st   %i7, [%sp + 60];
  	restore				! Go back to trap window.
  	mov  %l7, %g1
  	jmp  %l1			! Re-execute save.
  	rett %l2

  /* Window underflow trap handler.  */

  	.global  _window_underflow

_window_underflow:

	mov  %wim, %l3			! Calculate next WIM
	sll  %l3, 1, %l4
  	srl  %l3, NWINDOWS-1, %l5
  	or   %l5, %l4, %l5
  	mov  %l5, %wim
  	nop; nop; nop
  	restore				! Two restores to get into the
  	restore				! window to restore
        ld   [%sp + 0], %l0; 		! Restore window from the stack
        ld   [%sp + 4], %l1; 
        ld   [%sp + 8], %l2; 
        ld   [%sp + 12], %l3; 
        ld   [%sp + 16], %l4; 
        ld   [%sp + 20], %l5; 
        ld   [%sp + 24], %l6; 
        ld   [%sp + 28], %l7; 
        ld   [%sp + 32], %i0; 
        ld   [%sp + 36], %i1; 
        ld   [%sp + 40], %i2; 
        ld   [%sp + 44], %i3; 
        ld   [%sp + 48], %i4; 
        ld   [%sp + 52], %i5; 
        ld   [%sp + 56], %i6; 
        ld   [%sp + 60], %i7;
  	save				! Get back to the trap window.
  	save
  	jmp  %l1			! Re-execute restore.
  	rett  %l2

	.global	_flush_windows	
_flush_windows:		

	mov	%psr, %g1
	or	%g1, 0x0f00, %g2
	restore				! enter previous frame (cannot trap)
        wr      %g2, 0x20, %psr		! enable traps, disable interrupts
	nop; nop; nop
	save				! 6 save to flush all windows
	save
	save
	save
	save
	save
	restore				! 5 restore to enter trapped frame
	restore
	restore
	restore
	restore
	mov      %g1, %psr		! restore %psr
	nop; nop; nop
  	jmp  %l2			! Jump to nPC
  	rett  %l2 + 4
