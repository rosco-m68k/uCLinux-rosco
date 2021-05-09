/*
 * linux/arch/armnommu/kernel/head-arm-atmel.S
 *
 */

#include <linux/config.h>
#include <asm/assembler.h>
#include <asm/hardware.h>

#define ARM_MODE_SVC 0x13

#define DRAM_LIMIT   (DRAM_BASE + DRAM_SIZE)

.text
.align

.global start
.global _start
.global _stext
.global __start
.global __stext

start:
_start:
_stext:
_entry:
__stext:
__entry:
#ifdef CONFIG_EBI
	bl config_ebi
#endif

	bl config_aic
	
	mov	r0, #(ARM_MODE_SVC | I_BIT | F_BIT )
	msr	cpsr, r0
        ldr	r13, =DRAM_LIMIT	/* DRAM_LIMIT */

	adr	r5, LC0
	ldmia	r5, {r5, r6, r8, sp}	@ Setup stack
	mov	r4, #0
1:	cmp	r5, r8			@ Clear BSS
	strcc	r4, [r5],#4
	bcc	1b

	ldr	r2,=0x41007000		/* Hmm...???  What's this? */
	str	r2, [r6]

	mov	fp, #0
	b	start_kernel

LC0:	.long edata
	.long arm_id
	.long end
	.long init_kernel_stack + 4096
/*
	configure EBI
*/
config_ebi:
	ldr	r1,=EBI_MCR		/* EBI_MCR */
	ldr	r2,=0x6			/* 16M */
	str	r2, [r1]

	/* External Memory 0 : Flash ROM 0x01000000 16M 16bit 6waite */
	ldr	r1,=EBI_CSR0		/* EBI_CSR0 */
	ldr	r2,=FLASH_MEM_BASE
	str	r2, [r1]

	/* External Memory 1 : SRAM      0x02000000 16M 16bit 0waite */
	ldr	r1,=EBI_CSR1		/* EBI_CSR1 */
	ldr	r2,=DRAM_BASE
	str	r2, [r1]

#if 0
	ldr	r1,=EBI_RCR		/* EBI_RCR */
	ldr	r2,=0x1			/* Remap Cancel */
	str	r2, [r1]
#endif
	mov	pc, lr


/* 
	configure AIC,  disable all interrupt sources
*/
config_aic:
	ldr	r1,=AIC_IDCR		/* AIC_IDCR */
	ldr	r2,=0xFFFFFFFF
	str	r2, [r1]
	mov	pc, lr

