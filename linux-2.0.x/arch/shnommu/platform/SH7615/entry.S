
/* -*- mode: asm -*-
 *
 *  linux/arch/shnommu/kernel/entry.S
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
 *
 * (The icky duplicate functions are my fault -- kja)
 *
 * Based on:
 *
 *  linux/arch/m68k/kernel/entry.S
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 *
 * Linux/m68k support by Hamish Macdonald
 *
 * 68060 fixes by Jesper Skov
 *
 * Modified by anurag to include support for SH2 7615 -12/8/02
 */

/*
 * entry.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 * Stack layout in 'ret_from_exception':
 *
 *	This allows access to the syscall arguments in registers ro-r4
 *
 *	 0(sp) - r1
 *	 4(sp) - r2
 *	 8(sp) - r3
 *	 C(sp) - r4
 *	10(sp) - r5
 *	14(sp) - r6
 *	18(sp) - r7
 *	1C(sp) - r8
 *	20(sp) - r9
 *	24(sp) - r10
 *	28(sp) - r11
 *	2C(sp) - r12
 *	30(sp) - r13
 *	34(sp) - r14
 *	38(sp) - r0
 *	3C(sp) - orig_r0
 *	40(sp) - pr
 *	44(sp) - vec
 *	48(sp) - pc
 *      4C(sp) - sr
 */

#include <linux/sys.h>
#include <linux/config.h>
#include <linux/linkage.h>
#include <asm/setup.h>
#include <asm/segment.h>

LENOSYS = 38

/*
 * these are offsets into the task-struct
 */
LTASK_STATE	=  0
LTASK_COUNTER	=  4
LTASK_PRIORITY	=  8
LTASK_SIGNAL	= 12
LTASK_BLOCKED	= 16
LTASK_FLAGS	= 20

/* the following macro is used when enabling interrupts */
#define ALLOWINT 0xf8ff
#define	MAX_NOINT_IPL	0

/* Posh2 we have to define the values after finalising ptregs*/
LR0		= 0x38 
LORIG_R0	= 0x3C
LSR		= 0x4C
LFORMATVEC	= 0x44
LNR_syscalls	= 0xFF /* Posh2 need to be changed*/
/*
 * This defines the normal kernel pt-regs layout.
 */
	


	.macro SAVE_ALL
	
	mov.l	r0,@-r15
	mov.l	r1,@-r15

	mova	SYMBOL_NAME(entry_cnt),r0  /* entry-cnt is checked for nested kernel path*/
	mov.l	@r0,r1                    /* if > 1 nested kernel path we have to jmp */
	add	#1,r1                    /* without modifying sw_ksp*/
	mov.l	r1,@r0

/*	mov.l	SYMBOL_NAME(intr3),r1
	mov.l	@r1,r1*/
      	
	mov 	#1,r0
	cmp/hi	r0,r1
	bt	2f   
	nop


	mov.l	current_set01,r1  /* getting IMASK bits from task structure*/
	mov.l	@r1,r1
	mov.l	lsr,r0
	add	r0,r1
	mov.l	@r1,r0

	and	#0xF0, r0     /*sr of task structure(IMASK) is checked*/
	cmp/eq	#0,r0
	bf	1f
	nop
	
	mov.l	sw_ksp,r1 
	mov.l	@r1,r1
	add	#0xFC,r1

	mov	#12,r0       /*pushing sr to ksp*/
	mov.l	 @(r0,r15),r0
	mov.l	 r0,@r1
	add	#0xFC,r1   /*decrementing ksp by 4*/
	
	mov	#8,r0
	mov.l	 @(r0,r15),r0    /*pushing pc to ksp*/
	mov.l	 r0,@r1
	add	#0xFC,r1   /*decrementing ksp by 4*/
	
	mov	#4,r0
	mov.l	 @(r0,r15),r0    /*pushing r0 to ksp*/
	mov.l	 r0,@r1
	add	#0xFC,r1   /*decrementing ksp by 4*/
	
	
	
        mov.l	 @r15,r0
	mov.l	 r0,@r1      /*pushing r1 to ksp*/

	mov.l	sw_ksp,r0   /*sw_ksp should now point to the memory after pushing sr,pc,ro,r1*/	
	mov.l	r1,@r0
	 	

/*	add	#0xFC,r1 */  /*decrementing ksp by 4*/
		
	mov.l	sw_usp,r1    /*sw_usp should point to stkpointer before exception*/
	mov	#16,r0        /*pc,sr,r0 and r1*/
	add	r15,r0
	mov.l	r0,@r1     /*check here for sw_usp*/
        

/* saving usp to task structure*/
	mov.l	r12,@-r15
	mov.l	r13,@-r15
	mov.l	@r1,r1
	mov.l	current_set01,r12
	mov.l	@r12,r12
	mov.l	lusp,r13
	add	r12,r13
        mov.l	r1,@r13
	
	mov.l	@r15+,r13
	mov.l	@r15+,r12
	
	mov.l	sw_ksp,r1
	mov.l	@r1,r1 
	mov	r1,r15
	
1:
	
	mov.l	sw_ksp,r1
	mov.l	r15,@r1 

2:
	mov.l	@r15+,r1
	mov.l	@r15+,r0

	mov.l	r0,@-r15	/*dummy push for irq number*/	
	sts.l	pr,@-r15   
	mov.l	r0,@-r15   /* orig-r0*/
	mov.l	r0,@-r15	
	mov.l	r14,@-r15	
	mov.l	r13,@-r15	
	mov.l	r12,@-r15	
	mov.l	r11,@-r15	
	mov.l	r10,@-r15	
	mov.l	r9,@-r15	
	mov.l	r8,@-r15	
	mov.l	r7,@-r15	
	mov.l	r6,@-r15	
	mov.l	r5,@-r15	
	mov.l	r4,@-r15	
	mov.l	r3,@-r15	
	mov.l	r2,@-r15	
	mov.l	r1,@-r15

	.endm

  	.macro  RESTORE_ALL
	mov.l	@r15+,r1	
	mov.l	@r15+,r2
	mov.l	@r15+,r3	
	mov.l	@r15+,r4	
	mov.l	@r15+,r5	
	mov.l	@r15+,r6	
	mov.l	@r15+,r7	
	mov.l	@r15+,r8	
	mov.l	@r15+,r9	
	mov.l	@r15+,r10	
	mov.l	@r15+,r11	
	mov.l	@r15+,r12	
	mov.l	@r15+,r13	
	mov.l	@r15+,r14	
	mov.l	@r15+,r0	
	add	#4,r15		/* orig_r0*/
	lds.l	@r15+,pr		
	add	#4,r15	
	
	mov.l	r0,@-r15
	mov.l	r1,@-r15
	stc.l	sr,@-r15  /* this only dummy push of sr not using now*/
	

	mov.l	SYMBOL_NAME(entry_cnt1),r0
	mov.l	@r0,r1
	add	#0xFF,r1
	mov.l	r1,@r0
	
/*	mov.l	SYMBOL_NAME(intr4),r1
	mov.l	@r1,r1*/
      	
	mov 	#0,r0 /*changed to 0 from 1 */
	cmp/hi	r0,r1 /*changed cmp/eq*/
	bt	1f   
	nop
	
        mov	#16,r0
       

	mov.l	current_set02,r1  /* getting IMASK bits from task structure*/
	mov.l	@r1,r1
	mov.l	lsr1,r0
	add	r0,r1
	mov.l	@r1,r0



	/* mov.l	@(r0,r15), r0 */
	and	#0xF0, r0       /* IMASK bits from sr in task structure is checked */
	cmp/eq	#0,r0		/*from kernel so jump*/
	bf	1f
	nop
	
	mov.l	sw_usp01,r1
	mov.l	@r1,r1
	add	#0xFC,r1

	mov	#16,r0        /*pc,sr,r0 and r1 and original sr*/
	add	r15,r0      
	mov.l	@r0,r0        /* getting sr from ksp */
	and	#0x0f,r0     /* sr is made 0 before switching to user mode */

	mov.l	r0,@r1
	add	#0xFC,r1      /*decrementing usp by 4*/
	
	mov	#12,r0        
	add	r15,r0
	mov.l	@r0,r0        /* getting pc from ksp */
	mov.l	r0,@r1


	add	#0xFC,r1      /*decrementing usp by 4*/

	mov	#8,r0        
	add	r15,r0
	mov.l	@r0,r0        /* getting r0 from ksp */
	mov.l	r0,@r1
	

	add	#0xFC,r1      /*decrementing usp by 4*/

	mov	#4,r0        
	add	r15,r0
	mov.l	@r0,r0        /* getting r1 from ksp */
	mov.l	r0,@r1


	add	#0xFC,r1      /*decrementing usp by 4*/

	mov	#0,r0        
	add	r15,r0
	mov.l	@r0,r0        /* getting present sr from ksp */
	mov.l	r0,@r1
/*	add	#0xFC,r1      decrementing usp by 4*/
	
	add	#20,r15       /*saving present stk pointer as ksp */
	mov.l	sw_ksp01,r0
	mov.l	r15,@r0
		
	mov	r1,r15	      /*restoring usp as stack pointer*/

1:
	add	#4,r15		/* popping SR pushed for testing*/
	mov.l	@r15+,r1	
	mov.l	@r15+,r0	/* now stack should contain pc and sr for rte */

       .endm


/* We have to find the regs used by gcc*/
#define SWITCH_STACK_SIZE (0*4+4)	/* includes return address */

	.macro  SAVE_PR 
	 sts.l	pr,@-r15
	.endm

	.macro  RESTORE_PR
	 lds.l @r15+,pr
	.endm


.globl SYMBOL_NAME(system_call), SYMBOL_NAME(cpubuserr)
.globl SYMBOL_NAME(resume), SYMBOL_NAME(_ret_from_exception)
.globl SYMBOL_NAME(ret_from_signal)
.globl SYMBOL_NAME(_intwdt)
.globl SYMBOL_NAME(inthandler0)
.globl SYMBOL_NAME(inthandler1)
.globl SYMBOL_NAME(inthandler2)
.globl SYMBOL_NAME(inthandler3)
.globl SYMBOL_NAME(inthandler_wrap)
.globl SYMBOL_NAME(timerhandler)
.globl SYMBOL_NAME(sys_call_table)
.globl SYMBOL_NAME(sys_fork), SYMBOL_NAME(sys_clone)
.globl SYMBOL_NAME(ret_from_interrupt), SYMBOL_NAME(bad_interrupt)







.text
ENTRY(_cpubuserr)

	bra	handlebuserr
	nop


ENTRY(_dmabuserr)

	
	bra	handlebuserr
	nop


ENTRY(_genilleginst)

	
	mov.l   r7,@-r15

	mov	#4,r4
	bra	handleexcep
	nop

ENTRY(_sltilleginst)

	
	mov.l   r7,@-r15

	mov	#6,r4
	bra	handleexcep
	nop


ENTRY(_system_call)
	mov.l    r0,@-r15

	stc sr,r0
	or #0xf0,r0
	ldc r0,sr
	mov.l	@r15+,r0
	bra     system_call_handler
	nop

ENTRY(_trap33)

	mov.l   r7,@-r15

	
	mov	#33,r4
	bra	handleexcep
	nop

ENTRY(_trap34)

	mov.l   r7,@-r15

	
	mov	#34,r4
	bra	handleexcep
	nop
ENTRY(_trap35)

	mov.l   r7,@-r15

	
	mov	#35,r4
	bra	handleexcep
	nop

ENTRY(_trap36)

	mov.l   r7,@-r15

	
	mov	#36,r4
	bra	handleexcep
	nop

ENTRY(_trap37)

	
	mov.l   r7,@-r15

	mov	#37,r4
	bra	handleexcep
	nop

ENTRY(_trap38)

	
	mov.l   r7,@-r15

	mov	#38,r4
	bra	handleexcep
	nop


ENTRY(_trap39)

	mov.l   r7,@-r15

	
	mov	#39,r4
	bra	handleexcep
	nop

ENTRY(_trap40)

	
	mov.l   r7,@-r15

	mov	#40,r4
	bra	handleexcep
	nop

ENTRY(_trap41)

	
	mov.l   r7,@-r15

	mov	#41,r4
	bra	handleexcep
	nop
ENTRY(_trap42)

	mov.l   r7,@-r15

	
	mov	#42,r4
	bra	handleexcep
	nop
ENTRY(_trap43)

	mov.l   r7,@-r15

	
	mov	#43,r4
	bra	handleexcep
	nop


ENTRY(_trap44)

	
	mov.l   r7,@-r15

	mov	#44,r4
	bra	handleexcep
	nop
ENTRY(_trap45)

	
	mov.l   r7,@-r15

	mov	#45,r4
	bra	handleexcep
	nop
ENTRY(_trap46)

	mov.l   r7,@-r15

	
	mov	#46,r4
	bra	handleexcep
	nop
ENTRY(_trap47)

	mov.l   r7,@-r15

	
	mov	#47,r4
	bra	handleexcep
	nop
ENTRY(_trap48)

	
	mov.l   r7,@-r15

	mov	#48,r4
	bra	handleexcep
	nop

ENTRY(_trap49)

	
	mov.l   r7,@-r15

	mov	#49,r4
	bra	handleexcep
	nop
ENTRY(_trap50)

	
	mov.l   r7,@-r15

	mov	#50,r4
	bra	handleexcep
	nop
ENTRY(_trap51)

	mov.l   r7,@-r15

	
	mov	#51,r4
	bra	handleexcep
	nop
ENTRY(_trap52)

	
	mov.l   r7,@-r15

	mov	#52,r4
	bra	handleexcep
	nop
ENTRY(_trap53)

	
	mov	#53,r4
	bra	handleexcep
	nop
ENTRY(_trap54)

	mov.l   r7,@-r15

	
	mov	#54,r4
	bra	handleexcep
	nop
ENTRY(_trap55)

	mov.l   r7,@-r15

	
	mov	#55,r4
	bra	handleexcep
	nop


ENTRY(_trap56)

	
	mov.l   r7,@-r15

	mov	#56,r4
	bra	handleexcep
	nop
ENTRY(_trap57)

	
	mov.l   r7,@-r15

	mov	#57,r4
	bra	handleexcep
	nop
ENTRY(_trap58)

	
	mov.l   r7,@-r15

	mov	#58,r4
	bra	handleexcep
	nop
ENTRY(_trap59)

	mov.l   r7,@-r15

	
	mov	#59,r4
	bra	handleexcep
	nop

ENTRY(_trap60)

	mov.l   r7,@-r15

	
	mov	#60,r4
	bra	handleexcep
	nop
        

ENTRY(_trap61)

	
	mov.l   r7,@-r15

	mov	#61,r4
	bra	handleexcep
	nop
ENTRY(_trap62)

	mov.l   r7,@-r15

	mov	#62,r4
	bra	handleexcep
	nop
ENTRY(_trap63)

        mov.l	r7,@-r15	
	mov 	#63,r7 /*trap number is passed as an argument to trap_c*/
	bra	handleexcep
	nop



/*
** This is the main interrupt handler, responsible for calling process_int()
*/
ENTRY(_inthandler0)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #0, r7
        bra  	handleintr 
	nop

/*
** This is the main interrupt handler, responsible for calling process_int()
*/
ENTRY(_inthandler1)
	
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #1, r7
	bra	handleintr	
	nop


/*
** This is the main interrupt handler, responsible for calling process_int()
*/
ENTRY(_inthandler2)
	
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #2, r7
	bra	handleintr
	nop

/* This is the main interrupt handler, responsible for calling process_int()
*/
ENTRY(_inthandler3)
	
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #3, r7
	bra	handleintr
	nop


/*
** This is the main interrupt handler, responsible for calling process_int()
*/
	

ENTRY(_intdmac0)
	
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #4, r7
	bra	handleintr	
	nop


ENTRY(_intdmac1)
	
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #5, r7
	bra     handleintr	
	nop


ENTRY(_intwdt)

	mov.l 	r7,@-r15
	mov.l	r8,@-r15
	mov.l	r0,@-r15
	
	stc sr,r0
	or #0xf0,r0
	ldc r0,sr
	mov.l  	@r15+,r0

	mov.l	wdt_reg,r7 	// Added to clear overflow bit. For this timer
	mov.l	tmr_dis,r8 	// is disabled and again enabled. Clearing the
	mov.w	r8,@r7		// overflow bit alone didn't work out

	mov.l	tmr_enb,r8
	mov.w	r8,@r7
	mov.l	tmr_cnt,r8
	mov.w	r8,@r7

  	mov     #6, r7
	bra	handleintr	
	nop


ENTRY(_timerhandler) /* FRT OCIA*/

	mov.l 	r7,@-r15
	mov.l	r8,@-r15
	mov.l	r0,@-r15
	
	stc sr,r0
	or #0xf0,r0
	ldc r0,sr
		
	mov.l	r4,@-r15         /* Debug Led code for measuring the time period*/
	MOV.L INT2_LED_REG,r7       
        MOV.L @r7,r8
        MOV.L INT2_INIT_LED1,r4     
        XOR   r4,r8
        MOV  #0,r0
        MOV.L r0,@r7
        MOV.L r8,@r7
	mov.l  	@r15+,r4


	mov.l	frt_reg,r7 
	mov.b	@r7,r0
	mov	#0x01,r8
	mov.b   r8,@r7
	mov.l  	@r15+,r0


  	mov     #6, r7
	bra	handleintr	
	nop

ENTRY(_intrefcmi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

	mov     #7, r7
	bra	handleintr
	nop

ENTRY(_intscieri)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #8, r7
	bra	handleintr
	nop

ENTRY(_intscirxi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

	mov.l	r0,@-r15
	
	stc sr,r0
	or #0xf0,r0
	ldc r0,sr
	mov.l  	@r15+,r0
        mov     #9, r7
	bra	handleintr
	nop


ENTRY(_intscitxi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #10, r7
	bra	handleintr
	nop


ENTRY(_intscitei)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #11, r7
	bra	handleintr
	nop


ENTRY(_intfrtici)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #12, r7
	bra	handleintr
	nop


ENTRY(_intfrtovi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

					
        mov     #13, r7
	bra	handleintr
	nop


ENTRY(_intedmac)

	mov.l   r0,@-r15
	stc sr,r0
	or #0xf0,r0
	ldc r0,sr
	mov.l @r15+,r0
	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15
 
	mov.l   r4,@-r15
	

/*	MOV.L INT2_LED_REG,r7       led adress
        MOV.L @r7,r8
        MOV.L INT2_INIT_LED1,r4     xor value 
        XOR   r4,r8
        MOV  #0,r0
        MOV.L r0,@r7
        MOV.L r8,@r7*/

	mov.l @r15+,r4

 	
	mov     #14, r7
	bra	handleintr
	nop


ENTRY(_intsci2eri)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #15, r7
	bra	handleintr
	nop

ENTRY(_intsci2rxi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #16, r7
	bra	handleintr
	nop


ENTRY(_intsci2txi)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

        mov     #17, r7
	bra	handleintr
	nop


ENTRY(_intsci2tei)

	
	mov.l 	r7,@-r15
	mov.l	r8,@-r15

	mov	#18,r7
	bra	handleintr
	nop


ALIGN
handlebuserr:
	SAVE_ALL
	/*mov     #0x4C,r0  Saving usp to the global variable
	add     r15,r0
        mov.l	sw_usp1,r13
	mov.l    r0,@r13*/
	
	mov	#-1,r7
	sub	r0,r0
        add     #LORIG_R0, r0                                  
        mov.l   r7,@(r0,r15)         /* a -1 in the ORIG_R0 field */ 
					/* signifies that the stack frame */
					/* is NOT for syscall */
		
	mov	r15,r4	 		/* pt_reg passed as argument */
	mov.l   SYMBOL_NAME(labbuserr),r7
	jsr     @r7
	nop
	bra	SYMBOL_NAME(_ret_from_exception)
        nop

ALIGN
labbuserr:     		.long           _buserr_c


ALIGN

handleexcep:
	
        mov.l	r0,@-r15	
	mova 	trap_no,r0
	mov.l	r7,@r0
	mov.l	@r15+,r0
	mov.l	@r15+,r7

	SAVE_ALL
	
	mov	#-1,r7
	sub	r0,r0
        add     #LORIG_R0, r0                                  
        mov.l   r7,@(r0,r15)         /* a -1 in the ORIG_R0 field */ 
					/* signifies that the stack frame */
					/* is NOT for syscall */

	
	mova	trap_no,r0
	mov.l	@r0,r4		/*trap no and pointer to ptregs as params to trap_c*/
	mov	r15,r5 		/* pt_reg passed as argument */
	mov.l     SYMBOL_NAME(trap_c),r7
	jsr     @r7
	nop
	bra	SYMBOL_NAME(_ret_from_exception)
       nop
ALIGN
trap_c:		.long	_trap_c
trap_no:	.long	0

INT2_LED_REG:     .long  0x08000004
INT2_INIT_LED1:   .long  0x0001
/* Entry for onchip interrupts */

handleintr:
  	mov.l	r0,@-r15	
	mova 	irq_no,r0
	mov.l	r7,@r0
	mov.l	@r15+,r0
	mov.l	@r15+,r8
	mov.l	@r15+,r7

	SAVE_ALL
	
/*	mov.l	SYMBOL_NAME(current_set),r0
	mov.l	@r0,r0
	mov	r0,r10     //posh2 test code
	
	mov.l	SYMBOL_NAME(taskt),r7  
	add	#8,r7
	mov.l	@r7,r7
	cmp/eq	r0 ,r7             
	bf	jere
	nop

	mov.l	sw_kspt,r0
	mov.l	@r0,r0
	mov.l	sw_uspt,r1
	mov.l	@r1,r1
	cmp/hi	r1,r0
	bf	6f
	nop*/



/*	mov     #0x4C,r0   Saving usp to the global variable
	add     r15,r0
        mov.l	sw_usp3,r13
	mov.l    r0,@r13*/
jere:	
	mov	#-1,r8
	sub 	r0,r0
	add	#LORIG_R0,r0
	mov.l	r8,@(r0,r15) 	/* a -1 in the ORIG_R0 field */
					/* signifies that the stack frame */
					/* is NOT for syscall */
					

	mov.l	intr1, r1
  	mov.l	@r1,r2
	add	#1,r2
	mov.l	r2,@r1
	
	mova	irq_no,r0
	mov.l	@r0,r7
	sub	r0,r0
	add	#LFORMATVEC,r0
					/* getting irq number from variable irq_no to r7 */
	mov.l	r7, @(r0,r15)  		 /* The first param vec of process_int */

	mov	r7,r4			/*vect no and pointer to ptregs as params tp process_int*/
	mov	r15,r5			/*this is ptr to ptregs*/
	mov.l	SYMBOL_NAME(process_int),r7
	jsr	@r7        /*  process the IRQ */
        nop
        
	bra	ret_from_interrupt	/* this was fallthrough */
	nop
/*6:	bra	jere	posh2 testcode*/

ALIGN
process_int:	.long	_process_int

frt_reg:	.long	0xFFFFFE11
wdt_reg:	.long	0xFFFFFE80
tmr_dis:	.long   0xA51D
irq_no:		.long	0
intr1:		.long	_intr_count

wdt_reg1:	.long	0xFFFFFE80
tmr_enb:	.long	0xA53D
tmr_cnt:	.long	0x5A0B
taskt:		.long	_task


ENTRY(system_call_handler)

	SAVE_ALL

        mov	r0,r3			/*syscall no back up*/
	mov	#-LENOSYS,r7 		/* default return value(ENOSYS) in r0 */
 	mov	#LR0,r0
	mov.l   r7, @(r0,r15)		/* original r0 is in orig_r0 */

	
	/* save top of frame */
	mov 	r15 ,r4		/* passing stk ptr to set_espo*/
	mov.l	SYMBOL_NAME(labesp0),r7 /*assumng the parameter is passed thru stack*/
	jsr	@r7
	nop
	
	mov   #LNR_syscalls,r7  /*chk sys call # > maxval */
	mov.l	msbmask,r8
	and	r8,r7		/*sign adj*/
	cmp/hi  r7,r3		/* if r3 > T set */
	bt	SYMBOL_NAME(_ret_from_exception)
	nop

	shll2	r3		/*syscall no *4 =table offset*/
	mov.l	SYMBOL_NAME(sys_table),r0
	add 	r3,r0		
	mov.l	@r0,r0    /* chk if table addr is valid*/
	cmp/eq	#0,r0
	bt	SYMBOL_NAME(_ret_from_exception)
	nop
	
	mov	r0,r9	/*syscall address bk*/
	mov.l	SYMBOL_NAME(current_set),r2  /* pointer to task struct */
	mov.l	@r2,r2
	add	#(LTASK_FLAGS+3),r2
	mov.b	@r2, r0 /* chk if bits 31-8 r 0*/
	mov.l	msbmask,r8	/*sign extension avoidance*/
	and 	r8,r0
	mov	#16,r8
	and	r8,r0			/* chking bit 5 is set, r0 is sign checked */
					/* PF_TRACESYS */
	cmp/eq	#0,r0
	bt	1f
	nop

        mov	#LORIG_R0,r0
	mov 	r15,r14		/*save the stack pointer which is ptregs to r14 */
		                 /*syscall parametrs move pt_regs->r1 to r5  */
	mov.l 	@(0,r15),r4     /* Sh2 uses r4 -r7 and stack for params passing*/
	mov.l	@(4,r15),r5
	mov.l	@(8,r15),r6
	mov.l	@(12,r15),r7       /*chk whether args are pushed into the stack in the correct order*/
	mov.l	@(16,r15),r10
	mov.l	r10,@-r15        /*for param passing stk is adj by 8 bytes which will be unwinded by sysroutine*/
	
	jsr	@r9	  /*jump to syscall return in r0*/
	nop
	add	#4,r15    /*stk adjustment after parameter passing*/
	
	mov	r0,r10
	mov	#LR0,r0
	bra	SYMBOL_NAME(_ret_from_exception)
	mov.l	r10, @(r0, r15)   /*using slt advantage, save return value*/
1:
			  /*if trace is enabled sycall tracde is called before and after syscall*/	
	/*mov     #4,r7*/     /* SAVE_SWITCH STACK include return address so adj stack by 4*/
        /* sub     r7,r15 */   /* check this adjustment is required*/
	SAVE_PR
	mov.l	SYMBOL_NAME(syscall_trace),r7
	jsr	@r7
	nop
	RESTORE_PR
	/*add	#4,r15*/   /* sign extended addition,chk resultif err use abve*/
			   /*stk unwinding after SAVE_SWITCH macro*/

        mov	#LORIG_R0,r0
	mov 	r15,r14
					   
	mov.l 	@(0,r15),r4          /* Sh2 uses r4 -r7 and stack for params passing*/
	mov.l	@(4,r15),r5
	mov.l	@(8,r15),r6
	mov.l	@(12,r15),r7       /*chk whether args are pushed into the stack in the correct order*/
	mov.l	@(16,r15),r10
	mov.l	r10,@-r15  /*for param passing stk is adj by 8 bytes which will be unwinded by sysroutine*/

	jsr	@r9	  /*jump to syscall return in r0*/
	nop
	add 	#4,r15
	mov	r0,r10
	mov	#LR0,r0
	mov.l	r10, @(r0, r15)   /*save return value*/


      /*mov     #4,r7
        sub     r7,r15*/
	SAVE_PR
	mov.l	SYMBOL_NAME(syscall_trace),r7
	jsr	@r7
	nop
	bra	ret_from_signal
	nop
ALIGN

sw_usp:		.long	_sw_usp
sw_ksp:		.long	_sw_ksp
current_set01:	.long	_current_set
lusp:		.long   0x21C
lsr:		.long	0x220
intr3:		.long	_intr_count
entry_cnt:	.long	0

ALIGN
SYMBOL_NAME_LABEL(ret_from_signal)
	RESTORE_PR	/* stk unwinding after SAVE_PR*/	
	/*add	#4,r15*/

SYMBOL_NAME_LABEL(_ret_from_exception)


	

/* for the time being skipping this check we have to verify whether
this later*/


/*	mov.l	SYMBOL_NAME(current_set),r0    //posh test code
	mov.l	@r0,r0
	mov	r0,r10   
	
	mov.l	SYMBOL_NAME(task),r7  
	add	#8,r7
	mov.l	@r7,r7
	cmp/eq	r0 ,r7             
	bf	test
	nop

	mov.l	sw_kspt,r0
	mov.l	@r0,r0
	mov.l	sw_uspt,r1
	mov.l	@r1,r1
	cmp/hi	r1,r0
	bf	6f
	nop*/
	
	
/*	mov	#LSR,r0         adding LSR offset to stack
	mov.l	@(r0,r15), r0 			
	and	#0xF0, r0
	shlr2	r0
	shlr2	r0*/

test:
	mov.l	entry_cnt1,r0
	mov.l	@r0,r0
					
	mov	#1,r7					
	cmp/gt	r7,r0			/*Check if nested kernel.If nested kernel IMASK will be >1*/
	bt	2f			/* If nested kernel go to RESTORE_ALL */
	nop

	mov.l	SYMBOL_NAME(need_resched),r7
	mov.l	@r7,r0
	cmp/eq	#0,r0
	bt	1f    /* not required*/  /*changed from 1f to 2f*/
	nop

	mov.l	SYMBOL_NAME(labresched),r12
	jmp	@r12
	nop
1:	
	mov.l	SYMBOL_NAME(current_set),r0
	mov.l	@r0,r0
	mov	r0,r10   /*saving current stack*/
	mov.l	SYMBOL_NAME(task),r7  /* chk if curset , task equal jmp*/
	mov.l	@r7,r7
	cmp/eq	r0 ,r7             /*task[0] can't have signals*/
	bt	2f
	nop
				/*Posh2 adding LSTAsk +1 offset to r0*/

	mov	r10,r9
	add	#(LTASK_FLAGS +1),r9	
	mov.b	@r9,r0	     /* check for delayed trace */
	and	#0x20,r0
	cmp/eq	#0,r0
	bt	1f
	nop
	and	#0xdf,r0     /*clear bit 5*/
	mov.b	r0, @r9    /*store it back*/
	
	sub	r1,r1
	add	#1,r1	/*incrementing r1*/
	mov	r1, r4
	mov	r10, r5	/*send SIG_TRAP passing 3 params thru r4-r6  */
	add	#4,r1
	mov	r1, r6
	mov.l	SYMBOL_NAME(send_sig),r8
	jsr	@r8
	nop
	mov.l	SYMBOL_NAME(current_set),r10  /*Posh2 chk this statements are required using slt advn*/
	mov.l	@r10,r10
	

1:
	mov     r10,r9
	add	#LTASK_STATE,r9
	mov.l	@r9,r0	/* state  for reschedule*/
	cmp/eq	#0,r0
	bt	1f
	nop
	mov.l	SYMBOL_NAME(labresched),r12
	jmp	@r12
	nop
1:
	mov	r10,r9
	add	#LTASK_COUNTER,r9
	mov.l	@r9,r0 	/* counter */
	cmp/eq	#0,r0
	bf	1f
	nop

	mov.l	SYMBOL_NAME(labresched),r12
	jmp	@r12
	nop
1:
	mov	r10,r9
	mov	r10,r11
   	add	#LTASK_BLOCKED,r9
	mov.l	@r9,r0
	
			/* save blocked in d1 for sig handling */
        mov 	r0,r3
	not	r0,r0
	mov	r0,r2
	add	#(LTASK_FLAGS +3),r11
	sub	r7,r7
	mov.b	@r11,r7
	mov	#16,r8   /*bit 4 is tested and junps if 1*/
	and	r8,r7
	mov	r7,r0
	cmp/eq	#0,r0
	bt	1f
	nop
	sub	r2,r2
	mov	#1,r7
	sub     r7,r2	/*decrementing r1*/
1:
	mov	r10,r9
        add	#LTASK_SIGNAL,r9
	mov.l	@r9,r0
	and 	r2,r0
	cmp/eq	#0,r0
	bf	Lsignal_return
        nop
2:
	stc	sr,r0		/*disabling interrupts*/
	or	#0xF0,r0
	ldc	r0,sr

	RESTORE_ALL		/*left h8 specific fork return*/
	rte
	nop
/*6:	bra test		//Posh test code
	nop	*/
ALIGN
current_set:	.long		_current_set
syscall_trace:	.long		_syscall_trace
sys_table:  	.long	sys_call_table
sw_usp4:	.long	_sw_usp
msbmask:	.long	0x000000FF
sw_uspt:	.long	_sw_usp
sw_kspt:	.long	_sw_ksp



Lsignal_return:
	mov	r15,r5   /*pt regs ie Stkptr and r2 is passed */
	mov	r3,r4	/* chk value of r3 ?*/
	mov.l	SYMBOL_NAME(do_signal),r7
	jsr	@r7
	nop
	bra	2b
	nop
/*	RESTORE_ALL
	rte
	nop*/
ENTRY(reschedule)
	/* save top of frame */
	mov	r15,r4   /*stkptr is passed to setesp0*/
	mov.l	SYMBOL_NAME(labesp0),r7
	jsr	@r7
	nop
	mov.l	SYMBOL_NAME(ret_ex),r7
                    /*posh2 added to test schedule return with no process to schedule as prev*/
	mov.l	r7,@-r15
	lds.l	@r15+,pr
	
	mov.l	SYMBOL_NAME(schedule),r7   /*schedule function uses built in return addresses*/
	jmp	@r7
        nop
	ALIGN
schedule:	.long		_schedule
ret_ex:		.long		_ret_from_exception
/* posh2 trap 32 is used for system call */


SYMBOL_NAME_LABEL(ret_from_interrupt)
1:
	mov.l	SYMBOL_NAME(intr),r1
	mov.l	@r1,r0
      	/*mov	#1,r7*/
	/*sub     r7,r0*/	/*decrementing r1*/
	/*mov	r0,r10*/ /*bak up */
	/*mov	r1,r9*/
	mov 	#1,r7
	cmp/hi	r7,r0
	bt	4f    /* jmp if intr_count is> 1*/
	nop

2:
	mov.l	SYMBOL_NAME(bh_active),r0
	mov.l	@r0,r0
	mov.l	SYMBOL_NAME(bh_mask),r2
	mov.l	@r2,r2
	and	r2,r0
	cmp/eq	#0,r0
	bt	3f
	nop
	mov.l	SYMBOL_NAME(do_bottom_half),r7
	jsr	@r7
	nop
	bra	2b  	/*all botom half routines are finished*/
	nop 
3:	
	mov.l	SYMBOL_NAME(intr),r9
	sub	r0,r0
	mov.l	r0,@r9 /*intr cnt = 0 ,deliver signals ,reschedule etc*/
	
	bra	SYMBOL_NAME(_ret_from_exception)
	nop
	
4:
	mov.l	SYMBOL_NAME(intr),r1
	mov.l	@r1,r0
      	mov	#1,r7
	sub     r7,r0	/*decrementing r0*/

	mov.l	r0,@r1 /*posh2 check here for the value to be stored*/
	RESTORE_ALL
	rte
ALIGN
current_set02:	.long 	_current_set
lsr1:		.long 	0x220
intr:		.long	_intr_count
labesp0:	.long	_set_esp0
labresched:	.long   reschedule
intr4:		.long	_intr_count
entry_cnt1:	.long	entry_cnt
/* Handler for uninitialized and spurious interrupts */

SYMBOL_NAME_LABEL(bad_interrupt)
	mov.l	SYMBOL_NAME(spur),r0
	mov.l	@r0,r8
	mov	#1,r7
	add	r7,r8
	mov.l	r8,@r0
	rte

ENTRY(sys_fork)
	SAVE_PR
	mov	r14,r4 /*We will be passing the entire ptregs to the function*/
	mov.l	SYMBOL_NAME(sh2_fork),r0
	jsr	@r0
	nop
	RESTORE_PR
	rts
	nop
ENTRY(sys_clone)
	SAVE_PR
	mov	r14,r4
	mov.l	SYMBOL_NAME(sh2_clone),r7
	jsr	@r7
	nop
	RESTORE_PR
	rts
	nop
ENTRY(sys_sigsuspend)
	SAVE_PR
	mov	r14,r4
	mov.l	SYMBOL_NAME(do_sigsuspend),r7
	jsr	@r7
	nop
	RESTORE_PR
	rts
	nop
	
 ENTRY(sys_sigreturn)

	SAVE_PR
	mov	r14,r4
	mov.l	SYMBOL_NAME(do_sigreturn),r7
	jsr	@r7
	nop
	RESTORE_PR
	rts
	nop

LFLUSH_I_AND_D = 0x00000808
LTSS_KSP	= 0
LTSS_USP	= 4
LTSS_SR		= 8
LTSS_FS		= 10
LTSS_CRP	= 12
LTSS_FPCTXT	= 24
UMOD		= 1

SYMBOL_NAME_LABEL(resume)
	/*
	 * Beware - when entering resume, offset of tss is in d1,
	 * prev (the current task) is in a0, next (the new task)
	 * is in a1 and d2.b is non-zero if the mm structure is
	 * shared between the tasks, so don't change these
	 * registers until their contents are no longer needed.
	 *
	 *
	 * offset of tss struct (processor state) from beginning
	 *  of task struct 
	 * In sh2 r4 contains tsk struct and r5 offset
	 * this is never called in the case of m68knommu port
	 * Check the implementation later  r2-next r4-prev
	 */


/********************Saving prev task to task struct**********/

	stc	sr,r0		/*disabling interrupts*/
	or	#0xF0,r0
	ldc	r0,sr


	add	r1,r4  		/* tss offset to prev*/
	mov	r4,r8

	mov.l	sw_ksp1,r12	
	mov.l	@r12,r12
	add	#0xC0,r12	/* Now r12 points to pt_regs*/
	add	#0x4C,r12	/* This points to sr in pt_regs*/
	mov.l 	@r12,r0
	and	#0x0F,r0	/*IMASK of ptreg SR is made 0 */			
	mov	r0,r13
				/* saving the present sr to the tss of prev task*/
	add	#LTSS_SR,r4	
	mov.l	@r4,r0		/*getting IMASK bits from tss and  storing it to ptregs sr IMASK*/
	and	#0xF0,r0
	or	r0,r13		
/*	mov.l	r13,@r12 */

	
		
        mov 	r13,r0		/*check kernel process or user process*/
	and 	#0xF0,r0
	cmp/eq	#0,r0
	bf	1f
	nop

         			/*user mode process*/
/*1:*/	mov	r8,r0 		/*label- remove*/ 
	mov.l	sw_usp3,r7	/*prev task's usp is there in sw_usp.This is done in SAVE_ALL*/
	mov.l	@r7,r7          /*saving sw_usp in tss.usp*/
	mov.l	r7, @(LTSS_USP,r0)

1:				/*label- change to 1*/
	mov	r8,r0
/*	mov.l	sw_ksp1,r12
	mov.l	@r12,r12
	add	#0xC0,r12
	mov	r12,r15*/


	sts	pr,r7
	mov.l	r7,@-r15
	mov.l	r7,@-r15    	/*stk adjs for ret_from_exception and ptregs with switch stack*/
	mov.l 	r15, @(LTSS_KSP,r0) /*current stk pointer chk*/
		


/*******************Getting next task to run***********************/	

	mov.l	SYMBOL_NAME(current_set1),r10  /*modifying current_set with next task*/
	mov.l	r2,@r10  			/*next task in r2*/

	add	r1,r2  			/*tss offset of next task*/


        mov.l 	@(LTSS_SR,r2),r0	/*check kernel process or user process*/
	and 	#0xF0,r0
	cmp/eq	#0,r0
	bf	1f
	nop

					/*This is a user mode process*/
/*1:*/	mov.l	@(LTSS_USP,r2),r0	/*label- remove*/
	mov.l	sw_usp3,r3
	mov.l	r0,@r3  		/* We have to restore the user stack pointer */
	
	
	mov.l	@(LTSS_KSP,r2),r15
	mov.l	sw_ksp1,r3		/*current process's ksp will be stored in the variable sw_ksp*/
	mov.l	r15,@r3			/*This is again modified in RESTORE_ALL.But this is required for the 
					 switching from the init proces to another process for the first time						*/

	add     #4,r15
	lds.l	@r15+,pr  		/* getting address of ret_from_exception*/

/*	mov	#0x4c,r0
	mov 	r0,r12
	mov.l	@(r0,r15),r0
	and 	#0x0f,r0
	add	r15,r12
	mov.l 	r0,@r12*/
	rts
	nop

1:					/*kernel mode process */	/*label- change to 1*/
	mov.l	@(LTSS_KSP,r2),r15
	add	#4,r15
	lds.l	@r15+,pr
	rts
	nop


ALIGN
retex:		.long		_ret_from_exception
send_sig:	.long		_send_sig
spur:  		.long		_num_spurious
need_resched:	.long		_need_resched
task:		.long		_task
do_signal:	.long		_do_signal
do_sigreturn:	.long		_do_sigreturn
do_sigsuspend:	.long		_do_sigsuspend
bh_active:	.long		_bh_active
bh_mask:	.long		_bh_mask
do_bottom_half:  .long		_do_bottom_half
enter_sp:	.long		_enter_sp
sh2_fork:	.long		_sh2_fork
sh2_clone:	.long		_sh2_clone
fork_ret_vec:	.long		_fork_ret_vec
sw_usp3:	.long		_sw_usp
current_set1:	.long		_current_set
sw_ksp1:	.long		_sw_ksp
sw_ksp01:	.long		_sw_ksp
sw_usp01:	.long		_sw_usp
ksp_mask:	.long		0xFFFFF000
ksp_off:	.long		0xFA8
task1:		.long		_task
	.section	.bss
_enter_sp:	.space	4
kernel_sp:	.space	4
int_er0:	.space	4
_fork_ret_vec:	.space	4
fork_ret:	.space	4
.text
ALIGN
.globl SYMBOL_NAME(sys_call_table)
SYMBOL_NAME_LABEL(sys_call_table)
	.long SYMBOL_NAME(_sys_setup)		/* 0 */
	.long SYMBOL_NAME(_sys_exit)
	.long SYMBOL_NAME(sys_fork)
	.long SYMBOL_NAME(_sys_read)
	.long SYMBOL_NAME(_sys_write)
	.long SYMBOL_NAME(_sys_open)		/* 5 */
	.long SYMBOL_NAME(_sys_close)
	.long SYMBOL_NAME(_sys_waitpid)
	.long SYMBOL_NAME(_sys_creat)
	.long SYMBOL_NAME(_sys_link)
	.long SYMBOL_NAME(_sys_unlink)		/* 10 */
	.long SYMBOL_NAME(_sys_execve)
	.long SYMBOL_NAME(_sys_chdir)
	.long SYMBOL_NAME(_sys_time)
	.long SYMBOL_NAME(_sys_mknod)
	.long SYMBOL_NAME(_sys_chmod)		/* 15 */
	.long SYMBOL_NAME(_sys_chown)
	.long SYMBOL_NAME(_sys_break)
	.long SYMBOL_NAME(_sys_stat)
	.long SYMBOL_NAME(_sys_lseek)
	.long SYMBOL_NAME(_sys_getpid)		/* 20 */
	.long SYMBOL_NAME(_sys_mount)
	.long SYMBOL_NAME(_sys_umount)
	.long SYMBOL_NAME(_sys_setuid)
	.long SYMBOL_NAME(_sys_getuid)
	.long SYMBOL_NAME(_sys_stime)		/* 25 */
	.long SYMBOL_NAME(_sys_ptrace)
	.long SYMBOL_NAME(_sys_alarm)
	.long SYMBOL_NAME(_sys_fstat)
	.long SYMBOL_NAME(_sys_pause)
	.long SYMBOL_NAME(_sys_utime)		/* 30 */
	.long SYMBOL_NAME(_sys_stty)
	.long SYMBOL_NAME(_sys_gtty)
	.long SYMBOL_NAME(_sys_access)
	.long SYMBOL_NAME(_sys_nice)
	.long SYMBOL_NAME(_sys_ftime)		/* 35 */
	.long SYMBOL_NAME(_sys_sync)
	.long SYMBOL_NAME(_sys_kill)
	.long SYMBOL_NAME(_sys_rename)
	.long SYMBOL_NAME(_sys_mkdir)
	.long SYMBOL_NAME(_sys_rmdir)		/* 40 */
	.long SYMBOL_NAME(_sys_dup)
	.long SYMBOL_NAME(_sys_pipe)
	.long SYMBOL_NAME(_sys_times)
	.long SYMBOL_NAME(_sys_prof)
	.long SYMBOL_NAME(_sys_brk)		/* 45 */
	.long SYMBOL_NAME(_sys_setgid)
	.long SYMBOL_NAME(_sys_getgid)
	.long SYMBOL_NAME(_sys_signal)
	.long SYMBOL_NAME(_sys_geteuid)
	.long SYMBOL_NAME(_sys_getegid)		/* 50 */
	.long SYMBOL_NAME(_sys_acct)
	.long SYMBOL_NAME(_sys_phys)
	.long SYMBOL_NAME(_sys_lock)
	.long SYMBOL_NAME(_sys_ioctl)
	.long SYMBOL_NAME(_sys_fcntl)		/* 55 */
	.long SYMBOL_NAME(_sys_mpx)
	.long SYMBOL_NAME(_sys_setpgid)
	.long SYMBOL_NAME(_sys_ulimit)
	.long SYMBOL_NAME(_sys_olduname)
	.long SYMBOL_NAME(_sys_umask)		/* 60 */
	.long SYMBOL_NAME(_sys_chroot)
	.long SYMBOL_NAME(_sys_ustat)
	.long SYMBOL_NAME(_sys_dup2)
	.long SYMBOL_NAME(_sys_getppid)
	.long SYMBOL_NAME(_sys_getpgrp)		/* 65 */
	.long SYMBOL_NAME(_sys_setsid)
	.long SYMBOL_NAME(_sys_sigaction)
	.long SYMBOL_NAME(_sys_sgetmask)
	.long SYMBOL_NAME(_sys_ssetmask)
	.long SYMBOL_NAME(_sys_setreuid)		/* 70 */
	.long SYMBOL_NAME(_sys_setregid)
	.long SYMBOL_NAME(sys_sigsuspend)
	.long SYMBOL_NAME(_sys_sigpending)
	.long SYMBOL_NAME(_sys_sethostname)
	.long SYMBOL_NAME(_sys_setrlimit)	/* 75 */
	.long SYMBOL_NAME(_sys_getrlimit)
	.long SYMBOL_NAME(_sys_getrusage)
	.long SYMBOL_NAME(_sys_gettimeofday)
	.long SYMBOL_NAME(_sys_settimeofday)
	.long SYMBOL_NAME(_sys_getgroups)	/* 80 */
	.long SYMBOL_NAME(_sys_setgroups)
	.long SYMBOL_NAME(_old_select)
	.long SYMBOL_NAME(_sys_symlink)
	.long SYMBOL_NAME(_sys_lstat)
	.long SYMBOL_NAME(_sys_readlink)		/* 85 */
	.long SYMBOL_NAME(_sys_uselib)
	.long SYMBOL_NAME(_sys_swapon)
	.long SYMBOL_NAME(_sys_reboot)
	.long SYMBOL_NAME(_old_readdir)
	.long SYMBOL_NAME(_old_mmap)		/* 90 */
	.long SYMBOL_NAME(_sys_munmap)
	.long SYMBOL_NAME(_sys_truncate)
	.long SYMBOL_NAME(_sys_ftruncate)
	.long SYMBOL_NAME(_sys_fchmod)
	.long SYMBOL_NAME(_sys_fchown)		/* 95 */
	.long SYMBOL_NAME(_sys_getpriority)
	.long SYMBOL_NAME(_sys_setpriority)
	.long SYMBOL_NAME(_sys_profil)
	.long SYMBOL_NAME(_sys_statfs)
	.long SYMBOL_NAME(_sys_fstatfs)		/* 100 */
	.long SYMBOL_NAME(_sys_ioperm)
	.long SYMBOL_NAME(_sys_socketcall)
	.long SYMBOL_NAME(_sys_syslog)
	.long SYMBOL_NAME(_sys_setitimer)
	.long SYMBOL_NAME(_sys_getitimer)	/* 105 */
	.long SYMBOL_NAME(_sys_newstat)
	.long SYMBOL_NAME(_sys_newlstat)
	.long SYMBOL_NAME(_sys_newfstat)
	.long SYMBOL_NAME(_sys_uname)
	.long SYMBOL_NAME(_sys_ni_syscall)	/* iopl for i386 */ /* 110 */
	.long SYMBOL_NAME(_sys_vhangup)
	.long SYMBOL_NAME(_sys_idle)
	.long SYMBOL_NAME(_sys_ni_syscall)	/* vm86 for i386 */
	.long SYMBOL_NAME(_sys_wait4)
	.long SYMBOL_NAME(_sys_swapoff)		/* 115 */
	.long SYMBOL_NAME(_sys_sysinfo)
	.long SYMBOL_NAME(_sys_ipc)
	.long SYMBOL_NAME(_sys_fsync)
	.long SYMBOL_NAME(sys_sigreturn)
	.long SYMBOL_NAME(sys_clone)		/* 120 */
	.long SYMBOL_NAME(_sys_setdomainname)
	.long SYMBOL_NAME(_sys_newuname)
	.long SYMBOL_NAME(_sys_cacheflush)	/* modify_ldt for i386 */
	.long SYMBOL_NAME(_sys_adjtimex)
	.long SYMBOL_NAME(_sys_mprotect)		/* 125 */
	.long SYMBOL_NAME(_sys_sigprocmask)
	.long SYMBOL_NAME(_sys_create_module)
	.long SYMBOL_NAME(_sys_init_module)
	.long SYMBOL_NAME(_sys_delete_module)
	.long SYMBOL_NAME(_sys_get_kernel_syms)	/* 130 */
	.long SYMBOL_NAME(_sys_quotactl)
	.long SYMBOL_NAME(_sys_getpgid)
	.long SYMBOL_NAME(_sys_fchdir)
	.long SYMBOL_NAME(_sys_bdflush)
	.long SYMBOL_NAME(_sys_sysfs)		/* 135 */
	.long SYMBOL_NAME(_sys_personality)
	.long SYMBOL_NAME(_sys_ni_syscall)	/* for afs_syscall */
	.long SYMBOL_NAME(_sys_setfsuid)
	.long SYMBOL_NAME(_sys_setfsgid)
	.long SYMBOL_NAME(_sys_llseek)		/* 140 */
	.long SYMBOL_NAME(_sys_getdents)
	.long SYMBOL_NAME(_sys_select)
	.long SYMBOL_NAME(_sys_flock)
	.long SYMBOL_NAME(_sys_msync)
	.long SYMBOL_NAME(_sys_readv)		/* 145 */
	.long SYMBOL_NAME(_sys_writev)
	.long SYMBOL_NAME(_sys_getsid)
	.long SYMBOL_NAME(_sys_fdatasync)
	.long SYMBOL_NAME(_sys_sysctl)
	.long SYMBOL_NAME(_sys_mlock)		/* 150 */
	.long SYMBOL_NAME(_sys_munlock)
	.long SYMBOL_NAME(_sys_mlockall)
	.long SYMBOL_NAME(_sys_munlockall)
	.long SYMBOL_NAME(_sys_sched_setparam)
	.long SYMBOL_NAME(_sys_sched_getparam)   /* 155 */
	.long SYMBOL_NAME(_sys_sched_setscheduler)
	.long SYMBOL_NAME(_sys_sched_getscheduler)
	.long SYMBOL_NAME(_sys_sched_yield)
	.long SYMBOL_NAME(_sys_sched_get_priority_max)
	.long SYMBOL_NAME(_sys_sched_get_priority_min)  /* 160 */
	.long SYMBOL_NAME(_sys_sched_rr_get_interval)
	.long SYMBOL_NAME(_sys_nanosleep)
	.long SYMBOL_NAME(_sys_mremap)
	.long SYMBOL_NAME(_sys_test)
	.space (NR_syscalls-164)*4


/* Interrupt Vector Tables  */

	.section  	.vectors
	.long	SYMBOL_NAME(_start) /* 0 */
	.long 	0
	.long	SYMBOL_NAME(_start)
	.long	0
	.long	SYMBOL_NAME(_genilleginst)
	.long	0
        .long   SYMBOL_NAME(_sltilleginst) /* 6 */
	.space  4*2
        .long   SYMBOL_NAME(_cpubuserr) /* 9 */
	.long   SYMBOL_NAME(_dmabuserr)
/*	.long   SYMBOL_NAME(_nmi)
	.long   SYMBOL_NAME(_ubrk)
	.long   SYMBOL_NAME(_hudi) */ /* 13 */
	.space  19*4
	.long   SYMBOL_NAME(_system_call) /* 32 */
	.long   SYMBOL_NAME(_trap33)
	.long   SYMBOL_NAME(_trap34)
	.long   SYMBOL_NAME(_trap35)
	.long   SYMBOL_NAME(_trap36)
	.long   SYMBOL_NAME(_trap37)
	.long   SYMBOL_NAME(_trap38)
	.long   SYMBOL_NAME(_trap39)
	.long   SYMBOL_NAME(_trap40)
	.long   SYMBOL_NAME(_trap41)
	.long   SYMBOL_NAME(_trap42)
	.long   SYMBOL_NAME(_trap43)
	.long   SYMBOL_NAME(_trap44)
	.long   SYMBOL_NAME(_trap45)
	.long   SYMBOL_NAME(_trap46)
	.long   SYMBOL_NAME(_trap47)
	.long   SYMBOL_NAME(_trap48)
	.long   SYMBOL_NAME(_trap49)
	.long   SYMBOL_NAME(_trap50)
	.long   SYMBOL_NAME(_trap51)
	.long   SYMBOL_NAME(_trap52)
	.long   SYMBOL_NAME(_trap53)
	.long   SYMBOL_NAME(_trap54)
	.long   SYMBOL_NAME(_trap55)
	.long   SYMBOL_NAME(_trap56)
	.long   SYMBOL_NAME(_trap57)
	.long   SYMBOL_NAME(_trap58)
	.long   SYMBOL_NAME(_trap59)
	.long   SYMBOL_NAME(_trap60)
	.long   SYMBOL_NAME(_trap61)
	.long   SYMBOL_NAME(_trap62)
	.long   SYMBOL_NAME(_trap63)  /* 63 */
        .long   SYMBOL_NAME(_inthandler0) /* 64 */
        .long   SYMBOL_NAME(_inthandler1)
        .long   SYMBOL_NAME(_inthandler2)
        .long   SYMBOL_NAME(_inthandler3) /* 67 */
	.long   SYMBOL_NAME(_intdmac0)   /* 68 */
	.long   SYMBOL_NAME(_intdmac1)
	.long   SYMBOL_NAME(_intwdt)
	.long   SYMBOL_NAME(_intrefcmi)
	.long   SYMBOL_NAME(_intscieri) /* 72 */
	.long   SYMBOL_NAME(_intscirxi)
	.long   SYMBOL_NAME(_intscitxi)
	.long   SYMBOL_NAME(_intscitei)
	.long   SYMBOL_NAME(_intfrtici)
	.long   SYMBOL_NAME(_timerhandler) /* 77 */
	.long   SYMBOL_NAME(_intfrtovi)
	.long   SYMBOL_NAME(_intedmac)
	.long   SYMBOL_NAME(_intsci2eri)
	.long   SYMBOL_NAME(_intsci2rxi)
	.long   SYMBOL_NAME(_intsci2txi)
	.long   SYMBOL_NAME(_intsci2tei) /* 83 */
