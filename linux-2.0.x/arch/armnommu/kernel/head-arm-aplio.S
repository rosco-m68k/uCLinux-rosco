/*
 * linux/arch/armnommu/kernel/head-arm-trio.S
 *
 * Copyright (C) 1999 Aplio SA
 *
 * Kernel 32 bit startup code for APLIO Trio chip
 */
#include <linux/config.h>
#include <asm/assembler.h>
#include <asm/hardware.h>

		.text
		.align


/*----------------------------------
 * DMC & SMC Base Addresses
 * ----------------------------------
 */

#define ARM_MODE_SVC	0x13
#define DRAM_BASE     0x00000000
#define DRAM_SIZE     0x400000
#define DRAM_LIMIT    (DRAM_BASE + DRAM_SIZE)


		.globl	__entry,_entry
		.globl	__stext,_stext
		.globl _start,start

start:
_start:
_stext:
_entry:
__stext:
__entry:

		bl config_timer1

#if 0
/*
   | Initialise the Memory Controller
   | ---------------------------------
   | Copy the Image of the SMC & DMC Memory Controller
*/
        adr     r10, InitTableSMC    /* ; get the address of the SMC chip select image */
        adr     r11, InitTableDMC    /* ; get the address of the DMC chip select image  */

/*       
;- | Load the address where to jump, Zero out the most significant bits.
*/
#if 0
    ldr     r0, =InitRemap        /* ; get the real jump address ( after remap ) */
	mov	r0, r0, LSL #12
	mov	r0, r0, LSR #12

/*
;- | Calculate new PC after the SMC and DMC have been reconfigured.
*/
	ldr r1, [r10]
	ldr r2, =0xFFF00000
	and r1, r2, r1

	add r12, r0, r1
#endif

/*
;- | Copy The SMC Image into registers 0,1,2
*/
        ldmia   r10!, {r0-r3}


/*
;- | Copy The DMC Image into registers 3,4,5
*/
        ldmia   r11!, {r4-r7}

/*
;- Store the SMC values (r3 contains the base address of the SMC controller)
*/
        stmia   r3!, {r0-r2}
/*
;- Store the DMC values (r7 contains the base address of the DMC controller)
*/
        stmia   r7!, {r4-r6}

/*                   
;- | Jump to ROM at its new address
*/
new_remap_entry:
#if 0
	 mov         pc, r12             /* jump and break the pipeline */
#endif


#endif
/* we get here when RAM is mapped to address 0 */
InitRemap:

/*
; Setup the SVC mode and stack pointer on the top of the internal RAM
*/
    mov r0, #(ARM_MODE_SVC | I_BIT | F_BIT ) /* ; No interrupts */
    msr cpsr, r0
    ldr r13, =DRAM_LIMIT

/*
;------------------------------------------------------------------------------
;
; Call the Config routines...
;
;------------------------------------------------------------------------------
*/

    bl config_PIO

	adr	r5, LC0
	ldmia	r5, {r5, r6, r8, sp}			@ Setup stack
	mov	r4, #0
1:	cmp	r5, r8					@ Clear BSS
	strcc	r4, [r5],#4
	bcc	1b


/*
    bl config_usart0
    bl config_usart1
*/
	ldr r2,=0x41007100
	str r2,[r6]

	mov	fp, #0
	b	start_kernel




LC0:		.long	edata
		.long	arm_id
		.long	end
		.long	init_user_stack + 4096






InitTableDMC:
                .long	0x0000000D      /* DMC MRO */
                .long	0x1000000D      /* DMC MR1 */ 
                .long   0x03            /* DMC CR */
                .long   DMC_BASE       /* DMC Base Address */


InitTableSMC:
                .long   0x40003125      /* SMC CSR0 */
                .long	0				  /* SMC CSR1 */
                .long   0				  /* SMC CSR2 */
                .long   SMC_BASE		  /* SMC Base Address */



InitPIOs:
/*
;- |
;- | Driving a Low to any of these lines switches the LEDs on.
;- |
;- | PIOA 9     OUTPUT	LED D28
;- | PIOA 10-20 OUTPUT	LED D17-27 
;- | PIOA 21-31 OUTPUT	LED D29-D39
;- | PIOB 10-15 OUTPUT  LED D40-D45
*/        

config_PIO:
/*
;- Configure PIOA
*/
    ldr r1, PIOA_Enable_Reg
    ldr r2, PIOA_Enable_Value
    str r2, [r1]

    ldr r1, PIOA_Disable_Reg
    ldr r2, PIOA_Disable_Value
    str r2, [r1]
   
    ldr r1, PIOA_COD_Reg
    ldr r2, PIOA_COD_Value
    str r2, [r1]

    ldr r1, PIOA_OE_Reg
    ldr r2, PIOA_OE_Value
    str r2, [r1]
   
    ldr r1, PIOA_OD_Reg
    ldr r2, PIOA_OD_Value
    str r2, [r1]

/*        
;- Configure PIOB
*/
    ldr r1, PIOB_Enable_Reg
    ldr r2, PIOB_Enable_Value
    str r2, [r1]

    ldr r1, PIOB_Disable_Reg
    ldr r2, PIOB_Disable_Value
    str r2, [r1]
   
    ldr r1, PIOB_COD_Reg
    ldr r2, PIOB_COD_Value
    str r2, [r1]

    ldr r1, PIOB_OE_Reg
    ldr r2, PIOB_OE_Value
    str r2, [r1]
   
    ldr r1, PIOB_OD_Reg
    ldr r2, PIOB_OD_Value
    str r2, [r1]

    mov pc,lr

/*
;------------------------------------------------------------------------------
;
; PIO configuration details
;
;------------------------------------------------------------------------------
*/
PIOA_Enable_Reg:
    .long 0xFF00C000
PIOA_Enable_Value:
    .long 0xFFFFFFFF
PIOA_Disable_Reg:
    .long 0xFF00C004
PIOA_Disable_Value:
    .long 0x00000000
PIOA_OE_Reg:
    .long 0xFF00C010
PIOA_OE_Value:
    .long 0xFFFFFFFF
PIOA_OD_Reg:
    .long 0xFF00C014
PIOA_OD_Value:
    .long 0x00000000
PIOA_COD_Reg:
    .long 0xFF00C034
PIOA_COD_Value:
    .long 0xFFFFFFFF

PIOB_Enable_Reg:
    .long 0xFF010000
PIOB_Enable_Value:
    .long 0xFFFFFFFF
PIOB_Disable_Reg:
    .long 0xFF010004
PIOB_Disable_Value:
    .long 0x000003FF
PIOB_OE_Reg:
    .long 0xFF010010
PIOB_OE_Value:
    .long 0x0000FE4B
PIOB_OD_Reg:
    .long 0xFF010014
PIOB_OD_Value:
    .long 0x000001B4
PIOB_COD_Reg:
    .long 0xFF00C034
PIOB_COD_Value:
    .long 0xFFFFFFFF

/*
;------------------------------------------------------------------------------
;
; Configure timer 1, This timer channel is connected to DRAM controller
; and is used as the trigger for a DRAM refresh cycle.
; Trashes r1, r2
;
;------------------------------------------------------------------------------
*/
config_timer1:
    ldr r1, TC_CMR_REG
    ldr r2, TC_CMR_Value
    str r2, [r1]

    ldr r1, TC_RA_REG
    ldr r2, TC_RA_Value
    str r2, [r1]

    ldr r1, TC_RC_REG
    ldr r2, TC_RC_Value
    str r2, [r1]

    ldr r1, TC_CCR_REG
    ldr r2, TC_CCR_Value
    str r2, [r1]

    ldr r1, TC_BCR_REG
    ldr r2, TC_CCR_Value
    str r2, [r1]

    mov pc, lr

/*
;------------------------------------------------------------------------------
;
; Timer Control Configuration Data
;
;------------------------------------------------------------------------------
*/

TC_CMR_REG:
    .long 0xFF014004
TC_CMR_Value:
    .long 0xFFFFC000
TC_RA_REG:
    .long 0xFF014014
TC_RA_Value:
    .long 0x00000047
TC_RC_REG:
    .long 0xFF01401C
TC_RC_Value:
    .long 0x00000047
TC_CCR_REG:
    .long 0xFF014000
TC_CCR_Value:
    .long 0x00000001
TC_BCR_REG:
    .long 0xFF0140C0


