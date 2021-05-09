#include <asm/spr_defs.h>
#include <asm/ptrace.h>
#include <asm/board.h>
#include <asm/mc.h>
#include <linux/errno.h>

#define RAM 0    

        .global __text_start
        .global __main
        .global ___bss_start
        .global __bss_end
        .global __ram_start
        .global __ram_end
        .global __rom_start
        .global __rom_end
        .global ___data_start
        .global __data_end
        .global ___data_rom_start

        .global splash_bits
        .global __start
        .global __stext

        .global __switch
        .global _putc

#define SAVE_REGS(mark) \
        l.addi  r1,r1,-(INT_FRAME_SIZE); \
        l.mfspr r3,r0,SPR_EPCR_BASE; \
        l.sw    PC(r1),r3; \
        l.mfspr r3,r0,SPR_ESR_BASE; \
        l.sw    SR(r1),r3; \
        l.lwz   r3,4(r0); /* Read r1 (sp) from tmp location */ \
        l.sw    SP(r1),r3; \
        l.sw    GPR2(r1),r2; \
        l.sw    GPR4(r1),r4; \
        l.sw    GPR5(r1),r5; \
        l.sw    GPR6(r1),r6; \
        l.sw    GPR7(r1),r7; \
        l.sw    GPR8(r1),r8; \
        l.sw    GPR9(r1),r9; \
        l.sw    GPR10(r1),r10; \
        l.sw    GPR11(r1),r11; \
        l.sw    GPR12(r1),r12; \
        l.sw    GPR13(r1),r13; \
        l.sw    GPR14(r1),r14; \
        l.sw    GPR15(r1),r15; \
        l.sw    GPR16(r1),r16; \
        l.sw    GPR17(r1),r17; \
        l.sw    GPR18(r1),r18; \
        l.sw    GPR19(r1),r19; \
        l.sw    GPR20(r1),r20; \
        l.sw    GPR21(r1),r21; \
        l.sw    GPR22(r1),r22; \
        l.sw    GPR23(r1),r23; \
        l.sw    GPR24(r1),r24; \
        l.sw    GPR25(r1),r25; \
        l.sw    GPR26(r1),r26; \
        l.sw    GPR27(r1),r27; \
        l.sw    GPR28(r1),r28; \
        l.sw    GPR29(r1),r29; \
        l.sw    GPR30(r1),r30; \
        l.sw    GPR31(r1),r31; \
        l.lwz   r3,0(r0);  /* Read r3 from tmp location */ \
        l.sw    GPR3(r1),r3; \
        l.sw    ORIG_GPR3(r1),r3; \
	l.sw	RESULT(r1),r0

#define SAVE_INT_REGS(mark) \
        l.sw    0(r0),r3;       /* Temporary store r3 to add 0!!! */ \
        l.sw    4(r0),r1;       /* Temporary store r1 to add 4!!! */ \
        l.mfspr r3,r0,SPR_SR; \
        l.addi  r1,r0,-1; \
        l.xori  r1,r1,(SPR_SR_IEE | SPR_SR_TEE); \
        l.and   r3,r3,r1; \
        l.mtspr r0,r3,SPR_SR; \
        l.lwz   r1,4(r0); \
        l.mfspr r3,r0,SPR_ESR_BASE; /* Interrupt from user/system mode */ \
        l.andi  r3,r3,SPR_SR_SM; \
        l.sfeqi r3,SPR_SR_SM; \
        l.bf    10f; /* SIMON */ /* Branch if SUPV */ \
        l.nop; \
        l.movhi r3,hi(_current_set); \
        l.ori   r3,r3,lo(_current_set); \
        l.lwz   r3,0(r3); \
        l.sw    TSS+TSS_USP(r3),r1; \
        l.mfspr r1,r0,SPR_EPCR_BASE; \
        l.sw    TSS+TSS_PC(r3),r1; \
        l.lwz   r1,TSS+TSS_KSP(r3); \
        l.addi  r1,r1,-(INT_FRAME_SIZE); \
        l.sw    TSS+TSS_REGS(r3),r1; \
        l.lwz   r1,TSS+TSS_KSP(r3); \
10:     SAVE_REGS(mark)

#define RETURN_FROM_INT(mark) \
90:     l.addi  r4,r0,-1;       /* Disable interrupts */ \
        l.xori  r4,r4,(SPR_SR_IEE | SPR_SR_TEE); \
        l.mfspr r3,r0,SPR_SR; \
        l.and   r3,r3,r4; \
        l.mtspr r0,r3,SPR_SR; \
        l.movhi r2,hi(_intr_count); \
        l.ori   r2,r2,lo(_intr_count); \
        l.lwz   r3,0(r2); \
        l.sfeqi r3,0; \
        l.bnf   00f; \
        l.nop; \
        l.movhi r4,hi(_bh_mask); \
        l.ori   r4,r4,lo(_bh_mask); \
        l.lwz   r4,0(r4); \
        l.movhi r5,hi(_bh_active); \
        l.ori   r5,r5,lo(_bh_active); \
        l.lwz   r5,0(r5); \
        l.and   r4,r4,r5; \
        l.sfeqi r4,0; \
        l.bf    00f; \
        l.nop; \
        l.addi  r3,r3,1; \
        l.sw    0(r2),r3; \
        l.jal   _do_bottom_half; \
        l.nop; \
        l.movhi r2,hi(_intr_count); \
        l.ori   r2,r2,lo(_intr_count); \
        l.lwz   r3,0(r2); \
        l.addi  r3,r3,-1; \
        l.sw    0(r2),r3; \
00:     l.lwz   r2,SR(r1); \
        l.andi  r3,r2,SPR_SR_SM; \
        l.sfeqi r3,0; \
        l.bnf   10f; /* SIMON */ /* Branch if SUPV */ \
        l.nop; \
        l.andi  r3,r2,SPR_SR_ICE; \
        l.sfeqi r3,0; \
        l.bf    05f; /* Branch if IC disabled */ \
        l.nop; \
        l.jal   _ic_invalidate; \
        l.nop; \
05:     l.movhi r3,hi(_current_set); /* need to save kernel stack pointer */ \
        l.ori   r3,r3,lo(_current_set); \
        l.lwz   r3,0(r3); \
        l.addi  r4,r1,INT_FRAME_SIZE; \
        l.sw    TSS+TSS_KSP(r3),r4; \
        l.lwz   r4,STATE(r3); /* If state != 0, can't run */ \
        l.sfeqi r4,0; \
        l.bf    06f; \
        l.nop; \
        l.jal   _schedule; \
        l.nop; \
        l.j     90b; \
        l.nop; \
06:     l.lwz   r4,COUNTER(r3); \
        l.sfeqi r4,0; \
        l.bnf   07f; \
        l.nop; \
        l.jal   _schedule; \
        l.nop; \
        l.j     90b; \
        l.nop; \
07:     l.addi  r5,r0,-1; \
        l.lwz   r4,BLOCKED(r3); /* Check for pending unblocked signals */ \
        l.xor   r4,r4,r5; \
        l.lwz   r5,SIGNAL(r3); \
        l.and   r5,r5,r4; \
        l.sfeqi r5,0; \
        l.bf    10f; \
        l.nop; \
        l.addi  r3,r4,0; \
        l.addi  r4,r1,0; \
        l.jal   _do_signal; \
        l.nop; \
10:     l.lwz   r3,PC(r1); \
        l.mtspr r0,r3,SPR_EPCR_BASE; \
        l.lwz   r3,SR(r1); \
        l.mtspr r0,r3,SPR_ESR_BASE; \
        l.lwz   r2,GPR2(r1); \
        l.lwz   r3,GPR3(r1); \
        l.lwz   r4,GPR4(r1); \
        l.lwz   r5,GPR5(r1); \
        l.lwz   r6,GPR6(r1); \
        l.lwz   r7,GPR7(r1); \
        l.lwz   r8,GPR8(r1); \
        l.lwz   r9,GPR9(r1); \
        l.lwz   r10,GPR10(r1); \
        l.lwz   r11,GPR11(r1); \
        l.lwz   r12,GPR12(r1); \
        l.lwz   r13,GPR13(r1); \
        l.lwz   r14,GPR14(r1); \
        l.lwz   r15,GPR15(r1); \
        l.lwz   r16,GPR16(r1); \
        l.lwz   r17,GPR17(r1); \
        l.lwz   r18,GPR18(r1); \
        l.lwz   r19,GPR19(r1); \
        l.lwz   r20,GPR20(r1); \
        l.lwz   r21,GPR21(r1); \
        l.lwz   r22,GPR22(r1); \
        l.lwz   r23,GPR23(r1); \
        l.lwz   r24,GPR24(r1); \
        l.lwz   r25,GPR25(r1); \
        l.lwz   r26,GPR26(r1); \
        l.lwz   r27,GPR27(r1); \
        l.lwz   r28,GPR28(r1); \
        l.lwz   r29,GPR29(r1); \
        l.lwz   r30,GPR30(r1); \
        l.lwz   r31,GPR31(r1); \
        l.lwz   r1,SP(r1); \
        l.rfe; \
        l.nop

#if 0
        .bss    
sys_stack:
        .space  4*4096
#else
#endif
	.comm sys_stack, 4*4096
sys_stack_top:
#if !(RAM)
        .section .romvec
        .org    0x100

        l.nop
        l.movhi r3,hi(__start)
        l.movhi r3,hi(MC_BASE_ADD)
        l.ori   r3,r3,MC_BA_MASK
        l.addi  r5,r0,0x00
        l.sw    0(r3),r5

        l.movhi r3,hi(__start)
        l.ori   r3,r3,lo(__start)
        l.jr    r3
        l.nop

        .org    0x200

        l.nop
        l.rfe
        l.nop

        .org    0x300

        l.nop
        l.j     _dpfault
        l.nop

        .org    0x400

        l.nop
        l.j     _ipfault
        l.nop

        .org    0x500

        l.nop
        l.j     _tick
        l.nop

        .org    0x600

        l.nop
        l.j     _align
        l.nop

        .org    0x800

        l.nop
        l.j     _ext_int
        l.nop

        .org    0x900

        l.nop
        l.j     _dtlbmiss
        l.nop

        .org    0xa00

        l.nop
        l.j     _itlbmiss
        l.nop

        .org    0xb00

        l.nop
        l.j     _sys_call
        l.nop

        .org    0xc00

        l.nop
        l.j     _sys_call
        l.nop
#endif
        .section .ramvec
        .org    0x100

        l.movhi r3,hi(__start)
        l.ori   r3,r3,lo(__start)
        l.jr    r3
        l.nop

        .org    0x200

        l.nop
        l.rfe
        l.nop

        .org    0x300

        l.nop
        l.j     _dpfault
        l.nop

        .org    0x400

        l.nop
        l.j     _ipfault
        l.nop

        .org    0x500

        l.nop
        l.j     _tick
        l.nop

        .org    0x600

        l.nop
        l.j     _align
        l.nop

        .org    0x800

        l.nop
        l.j     _ext_int
        l.nop

        .org    0x900

        l.nop
        l.j     _dtlbmiss
        l.nop

        .org    0xa00

        l.nop
        l.j     _itlbmiss
        l.nop

        .org    0xb00

        l.nop
        l.j     _sys_call
        l.nop

        .org    0xc00

        l.nop
        l.j     _sys_call
        l.nop


        .text
__start:
__stext:
        l.addi  r3,r0,SPR_SR_SM
        l.mtspr r0,r3,SPR_SR
#if 1

        /* Init uart */
        l.jal   _ua_init
        l.nop

        /* Jump to flash original location */
        l.movhi r3,hi(__flsh_start)
        l.ori   r3,r3,lo(__flsh_start)
        l.jr    r3
        l.nop

__flsh_start:

#if MC_INIT
        l.movhi r3,hi(MC_BASE_ADD)
        l.ori   r3,r3,lo(MC_BASE_ADD)
 
        l.addi  r4,r3,MC_CSC(0)
        l.movhi r5,hi(FLASH_BASE_ADD)
        l.srai  r5,r5,5
        l.ori   r5,r5,0x0025
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_TMS(0)
        l.movhi r5,hi(FLASH_TMS_VAL)
        l.ori   r5,r5,lo(FLASH_TMS_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_BA_MASK
        l.addi  r5,r0,MC_MASK_VAL
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_CSR
        l.movhi r5,hi(MC_CSR_VAL)
        l.ori   r5,r5,lo(MC_CSR_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_TMS(1)
        l.movhi r5,hi(SDRAM_TMS_VAL)
        l.ori   r5,r5,lo(SDRAM_TMS_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_CSC(1)
        l.movhi r5,hi(SDRAM_BASE_ADD)
        l.srai  r5,r5,5
        l.ori   r5,r5,0x0411
        l.sw    0(r4),r5

#ifdef FBMEM_BASE_ADD
        l.addi  r4,r3,MC_CSC(2)
        l.movhi r5,hi(FBMEM_BASE_ADD)
        l.srai  r5,r5,5
        l.ori   r5,r5,0x0005
        l.sw    0(r4),r5

        l.addi  r4,r3,MC_TMS(2)
        l.movhi r5,0xffff
        l.ori   r5,r5,0xffff
        l.sw    0(r4),r5 
#endif
#endif
#endif

#if ICACHE
        l.jal   _ic_enable
        l.nop
#endif

#if DCACHE
        l.jal   _dc_enable
        l.nop
#endif

        l.movhi r1, hi(sys_stack_top)           /* stack setup */
        l.ori   r1,r1,lo(sys_stack_top)

#if 1
        /* Copy data segment from ROM to RAM */
        l.movhi r3, hi(___data_rom_start)
        l.ori   r3,r3,lo(___data_rom_start)

        l.movhi r4, hi(___data_start)
        l.ori   r4,r4,lo(___data_start)

        l.movhi r5, hi(__data_end)
        l.ori   r5,r5,lo(__data_end)

        /* Copy %r3 to %r4 until %r4 == %r5 */
1:
        l.sfeq  r3,r4
        l.bf    3f
        l.nop
2:
        l.sfgeu r4,r5
        l.bf    1f
        l.nop
        l.lwz   r8,0(r3)
        l.sw    0(r4),r8
        l.addi  r3,r3,4
        l.j     2b
        l.addi  r4,r4,4

        /* Copy ramvec segment from ROM to RAM */
1:
        l.movhi r4, hi(__ramvec_start)
        l.ori   r4,r4,lo(__ramvec_start)

        l.movhi r5, hi(__ramvec_end)
        l.ori   r5,r5,lo(__ramvec_end)

        /* Copy %r3 to %r4 until %r4 == %r5 */
2:
        l.sfgeu r4,r5
        l.bf    1f
        l.nop
        l.lwz   r8,0(r3)
        l.sw    0(r4),r8
        l.addi  r3,r3,4
        l.j     2b
        l.addi  r4,r4,4
#if 0
        /* Copy initrd segment from ROM to RAM */
1:
        l.movhi r4, hi(__initrd_start)
        l.ori   r4,r4,lo(__initrd_start)

        l.movhi r5, hi(__initrd_end)
        l.ori   r5,r5,lo(__initrd_end)

        /* Copy %r3 to %r4 until %r4 == %r5 */
2:
        l.sfgeu r4,r5
        l.bf    1f
        l.nop
        l.lwz   r8,0(r3)
        l.sw    0(r4),r8
        l.addi  r3,r3,4
        l.j     2b
        l.addi  r4,r4,4
#endif
#endif
1:
3:
        l.movhi r3, hi(___bss_start)
        l.ori   r3,r3,lo(___bss_start)

        l.movhi r4, hi(end)
        l.ori   r4,r4,lo(end)

        /* Copy 0 to %r3 until %r3 == %r4 */
1:
        l.sfgeu r3,r4
        l.bf    1f
        l.nop
        l.sw    0(r3),r0
        l.j     1b
        l.addi  r3,r3,4
1:

#if IMMU
        l.jal   _immu_enable
        l.nop
#endif

#if DMMU
        l.jal   _dmmu_enable
        l.nop
#endif

        l.j     _start_kernel
        l.nop

_exit:
        l.j     _exit
        l.nop

_dpfault:

        
_ipfault:

_tick:
        SAVE_INT_REGS(0x0500)
        l.addi  r3,r1,0
        l.jal   _timer_interrupt
        l.nop
        RETURN_FROM_INT(0x500)

_align:
	l.addi	r1,r1,-128
	l.sw    0x08(r1),r2
        l.sw    0x0c(r1),r3
        l.sw    0x10(r1),r4
        l.sw    0x14(r1),r5
        l.sw    0x18(r1),r6
        l.sw    0x1c(r1),r7
        l.sw    0x20(r1),r8
        l.sw    0x24(r1),r9
        l.sw    0x28(r1),r10
        l.sw    0x2c(r1),r11
        l.sw    0x30(r1),r12
        l.sw    0x34(r1),r13
        l.sw    0x38(r1),r14
        l.sw    0x3c(r1),r15
        l.sw    0x40(r1),r16
        l.sw    0x44(r1),r17
        l.sw    0x48(r1),r18
        l.sw    0x4c(r1),r19
        l.sw    0x50(r1),r20
        l.sw    0x54(r1),r21
        l.sw    0x58(r1),r22
        l.sw    0x5c(r1),r23
        l.sw    0x60(r1),r24
        l.sw    0x64(r1),r25
        l.sw    0x68(r1),r26
        l.sw    0x6c(r1),r27
        l.sw    0x70(r1),r28
        l.sw    0x74(r1),r29
        l.sw    0x78(r1),r30
        l.sw    0x7c(r1),r31
	
	l.mfspr	r2,r0,SPR_EEAR_BASE	/* Load the efective addres */
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* Load the insn address */

	l.lwz	r3,0(r5)		/* Load insn */
	l.srli  r4,r3,26		/* Shift left to get the insn opcode */

	l.sfeqi r4,0x00			/* Check if the load/store insn is in delay slot */
	l.bf	jmp
	l.sfeqi r4,0x01
	l.bf	jmp
	l.sfeqi r4,0x03
	l.bf	jmp
	l.sfeqi r4,0x04
	l.bf	jmp
	l.sfeqi r4,0x11
	l.bf	jr
	l.sfeqi r4,0x12
	l.bf	jr
	l.nop
	l.j	1f
	l.addi  r5,r5,4			/* Increment PC to get return insn address */
	
jmp:
	l.slli	r4,r3,6			/* Get the signed extended jump length */
	l.srai	r4,r4,4

	l.lwz	r3,4(r5)		/* Load the real load/store insn */

	l.add	r5,r5,r4		/* Calculate jump target address */
	
	l.j	1f
	l.srli  r4,r3,26		/* Shift left to get the insn opcode */

jr:
	l.slli	r4,r3,9			/* Shift to get the reg nb */
	l.andi	r4,r4,0x7c

	l.lwz	r3,4(r5)		/* Load the real load/store insn */

	l.add	r4,r4,r1		/* Load the jump register value from the stack */
	l.lwz	r5,0(r4)

	l.srli  r4,r3,26		/* Shift left to get the insn opcode */
	
	
1:	l.mtspr	r0,r5,SPR_EPCR_BASE

	l.sfeqi	r4,0x26
	l.bf	lhs
	l.sfeqi	r4,0x25
	l.bf	lhz
	l.sfeqi	r4,0x22
	l.bf	lws
	l.sfeqi	r4,0x21
	l.bf	lwz
	l.sfeqi	r4,0x37
	l.bf	sh
	l.sfeqi	r4,0x35
	l.bf	sw
	l.nop

1:	l.j	1b			/* I don't know what to do */
	l.nop

lhs:	l.lbs	r5,0(r2)
	l.slli	r5,r5,8
	l.lbz	r6,1(r2)
	l.or	r5,r5,r6
	l.srli	r4,r3,19
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.j	align_end
	l.sw	0(r4),r5	
	
lhz:	l.lbz	r5,0(r2)
	l.slli	r5,r5,8
	l.lbz	r6,1(r2)
	l.or	r5,r5,r6
	l.srli	r4,r3,19
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.j	align_end
	l.sw	0(r4),r5	
		
lws:	l.lbs	r5,0(r2)
	l.slli	r5,r5,24
	l.lbz	r6,1(r2)
	l.slli	r6,r6,16
	l.or	r5,r5,r6
	l.lbz	r6,2(r2)
	l.slli	r6,r6,8
	l.or	r5,r5,r6
	l.lbz	r6,3(r2)
	l.or	r5,r5,r6
	l.srli	r4,r3,19
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.j	align_end
	l.sw	0(r4),r5	
		
lwz:	l.lbz	r5,0(r2)
	l.slli	r5,r5,24
	l.lbz	r6,1(r2)
	l.slli	r6,r6,16
	l.or	r5,r5,r6
	l.lbz	r6,2(r2)
	l.slli	r6,r6,8
	l.or	r5,r5,r6
	l.lbz	r6,3(r2)
	l.or	r5,r5,r6
	l.srli	r4,r3,19
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.j	align_end
	l.sw	0(r4),r5	
		
sh:
	l.srli	r4,r3,9
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.lwz	r5,0(r4)	
	l.sb	1(r2),r5
	l.slli	r5,r5,8
	l.j	align_end
	l.sb	0(r2),r5

sw:
	l.srli	r4,r3,9
	l.andi	r4,r4,0x7c
	l.add	r4,r4,r1
	l.lwz	r5,0(r4)	
	l.sb	3(r2),r5
	l.slli	r5,r5,8
	l.sb	2(r2),r5
	l.slli	r5,r5,8
	l.sb	1(r2),r5
	l.slli	r5,r5,8
	l.j	align_end
	l.sb	0(r2),r5

align_end:
	l.lwz   r2,0x08(r1)
        l.lwz   r3,0x0c(r1)
        l.lwz   r4,0x10(r1)
        l.lwz   r5,0x14(r1)
        l.lwz   r6,0x18(r1)
        l.lwz   r7,0x1c(r1)
        l.lwz   r8,0x20(r1)
        l.lwz   r9,0x24(r1)
        l.lwz   r10,0x28(r1)
        l.lwz   r11,0x2c(r1)
        l.lwz   r12,0x30(r1)
        l.lwz   r13,0x34(r1)
        l.lwz   r14,0x38(r1)
        l.lwz   r15,0x3c(r1)
        l.lwz   r16,0x40(r1)
        l.lwz   r17,0x44(r1)
        l.lwz   r18,0x48(r1)
        l.lwz   r19,0x4c(r1)
        l.lwz   r20,0x50(r1)
        l.lwz   r21,0x54(r1)
        l.lwz   r22,0x58(r1)
        l.lwz   r23,0x5c(r1)
        l.lwz   r24,0x60(r1)
        l.lwz   r25,0x64(r1)
        l.lwz   r26,0x68(r1)
        l.lwz   r27,0x6c(r1)
        l.lwz   r28,0x70(r1)
        l.lwz   r29,0x74(r1)
        l.lwz   r30,0x78(r1)
        l.lwz   r31,0x7c(r1)
	l.addi	r1,r1,128
	l.rfe


_ext_int:
        SAVE_INT_REGS(0x0800)
        l.addi  r3,r1,0
        l.jal   _handle_IRQ
        l.nop
        RETURN_FROM_INT(0x800)

_dtlbmiss:
        l.sw    0(r0),r3
        l.sw    4(r0),r4
        l.sw    8(r0),r5
        l.mfspr r3,r0,SPR_EEAR_BASE
        l.srli  r4,r3,DMMU_PAGE_ADD_BITS
        l.andi  r4,r4,DMMU_SET_ADD_MASK
        l.addi  r5,r0,-1
        l.xori  r5,r5,DMMU_PAGE_ADD_MASK
        l.and   r5,r3,r5
        l.ori   r5,r5,SPR_DTLBMR_V
        l.mtspr r4,r5,SPR_DTLBMR_BASE(0)
        l.movhi r5,hi(SPR_DTLBTR_PPN)
        l.ori   r5,r5,lo(SPR_DTLBTR_PPN)
        l.and   r5,r3,r5
        l.ori   r5,r5,DTLBTR_NO_LIMIT
        l.movhi r3,0x8000
        l.sfgeu r5,r3
        l.bnf   1f
        l.nop
        l.ori   r5,r5,SPR_DTLBTR_CI
1:      l.mtspr r4,r5,SPR_DTLBTR_BASE(0)
        l.lwz   r3,0(r0)
        l.lwz   r4,4(r0)
        l.lwz   r5,8(r0)
        l.rfe
        l.nop


_itlbmiss:
        l.sw    0(r0),r3
        l.sw    4(r0),r4
        l.sw    8(r0),r5
        l.mfspr r3,r0,SPR_EEAR_BASE
        l.srli  r4,r3,IMMU_PAGE_ADD_BITS
        l.andi  r4,r4,IMMU_SET_ADD_MASK
        l.addi  r5,r0,-1
        l.xori  r5,r5,IMMU_PAGE_ADD_MASK
        l.and   r5,r3,r5
        l.ori   r5,r5,SPR_ITLBMR_V
        l.mtspr r4,r5,SPR_ITLBMR_BASE(0)
        l.movhi r5,hi(SPR_ITLBTR_PPN)
        l.ori   r5,r5,lo(SPR_ITLBTR_PPN)
        l.and   r5,r3,r5
        l.ori   r5,r5,ITLBTR_NO_LIMIT
        l.mtspr r4,r5,SPR_ITLBTR_BASE(0)
        l.lwz   r3,0(r0)
        l.lwz   r4,4(r0)
        l.lwz   r5,8(r0)
        l.rfe
        l.nop

_sys_call:
        SAVE_INT_REGS(0x0c00)
        /* EPCR was pointing to l.sys instruction, we have to incremet it */
/*      l.lwz   r2,PC(r1)
        l.addi  r2,r2,4
        l.sw    PC(r1),r2
*/
	l.sfeqi	r11,0x7777		/* Special case for 'sys_sigreturn' */
	l.bnf	10f
	l.nop
	l.jal	_sys_sigreturn
	l.addi	r3,r1,0
	l.sfgtsi r11,0			/* Check for restarted system call */
	l.bf	99f
	l.nop
	l.j	20f
	l.nop
10:
        l.movhi r2,hi(_sys_call_table)
        l.ori   r2,r2,lo(_sys_call_table)
        l.slli  r11,r11,2
        l.add   r2,r2,r11
        l.lwz   r2,0(r2)
        l.addi  r8,r1,0                 /* regs pointer */
        l.jalr  r2
        l.nop
        l.sw    GPR11(r1),r11           /* save return value */
20:
        l.sw    RESULT(r1),r11          /* save result */
	l.sfgesi r11,0
	l.bf	99f
	l.nop
	l.sfeqi	r11,-ERESTARTNOHAND
	l.bnf	22f
	l.nop
	l.addi	r11,r0,EINTR
22:
	l.sw    RESULT(r1),r11
99:
        RETURN_FROM_INT(0xc00)

/*
 * This routine switches between two different tasks.  The process
 * state of one is saved on its kernel stack.  Then the state
 * of the other is restored from its kernel stack.  The memory
 * management hardware is updated to the second process's state.
 * Finally, we can return to the second process, via the 'return'.
 *
 * Note: there are two ways to get to the "going out" portion
 * of this code; either by coming in via the entry (_switch)
 * or via "fork" which must set up an environment equivalent
 * to the "_switch" path.  If you change this (or in particular, the
 * SAVE_REGS macro), you'll have to change the fork code also.
 */
__switch:
        l.sw    0(r0),r3                /* Temporary store r3 to add 0!!! */
        l.sw    4(r0),r1                /* Temporary store r1 to add 4!!! */
        l.mtspr r0,r9,SPR_EPCR_BASE     /* Link register to EPCR */
        l.mfspr r3,r0,SPR_SR            /* From SR to ESR */
        l.mtspr r0,r3,SPR_ESR_BASE
        SAVE_REGS(0x0FF0)
        l.sw    TSS_KSP(r3),r1          /* Set old stack pointer */
        l.lwz   r1,TSS_KSP(r4)          /* Load new stack pointer */
        RETURN_FROM_INT(0xFF0)

_ua_init:
        l.movhi r3,hi(UART_BASE_ADD)

        l.addi r4,r0,0x7
        l.sb 0x2(r3),r4        

        l.addi r4,r0,0x0
        l.sb 0x1(r3),r4

        l.addi r4,r0,0x3
        l.sb 0x3(r3),r4

        l.lbz  r5,3(r3)
        l.ori r4,r5,0x80
        l.sb  0x3(r3),r4
        l.sb  0x1(r3),r0
        l.addi  r4,r0,0x82
        l.sb  0x0(r3),r4
        l.sb  0x3(r3),r5
  
        l.jr  r9
        l.nop

_putc:
        l.movhi r4,hi(UART_BASE_ADD)

        l.addi  r6,r0,0x20
1:      l.lbz   r5,5(r4)
        l.andi  r5,r5,0x20
        l.sfeq  r5,r6
        l.bnf   1b
        l.nop
      
        l.sb    0(r4),r3

        l.addi  r6,r0,0x60
1:      l.lbz   r5,5(r4)
        l.andi  r5,r5,0x60
        l.sfeq  r5,r6
        l.bnf   1b
        l.nop

        l.jr    r9
        l.nop


        .data
env:
        .long   0
