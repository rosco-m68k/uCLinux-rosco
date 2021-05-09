#include <asm/spr_defs.h>
#include <asm/board.h>

	.text
/*
 * Enable interrupts
 *      sti()
 */
	.global _sti
_sti:
	l.mfspr r3,r0,SPR_SR
	l.ori r3,r3,(SPR_SR_IEE | SPR_SR_TEE)
	l.mtspr r0,r3,SPR_SR
	l.jr	r9
	l.nop

/*
 * Disable interrupts
 *      cli()
 */
	.global _cli
_cli:
	l.addi r4,r0,-1
	l.xori r4,r4,(SPR_SR_IEE | SPR_SR_TEE)
	l.mfspr r3,r0,SPR_SR
	l.and r3,r3,r4
	l.mtspr r0,r3,SPR_SR
	l.jr	r9
	l.nop

/*
 * Get 'flags' (aka status register)
 *      __save_flags(long *ptr)
 */
	.global ___save_flags
___save_flags:
	l.mfspr r4,r0,SPR_SR
	l.sw	0(r3),r4
	l.jr	r9
	l.nop

/*
 * Restore 'flags'
 *      __restore_flags(long val)
 */
	.global ___restore_flags
___restore_flags:
	l.psync
	l.mtspr r0,r3,SPR_SR
	l.jr	r9
	l.nop

/*
 * SPR write
 *      mtspr(long add, long val)
 */
	.global _mtspr
_mtspr:
	l.mtspr	r3,r4,0
	l.jr	r9
	l.nop

/*
 * SPR read
 *      mtspr(long add)
 */
	.global _mfspr
_mfspr:
	l.mfspr	r11,r3,0
	l.jr	r9
	l.nop

/*
 * Instruction cache enable
 *      ic_enable()
 */
	.global _ic_enable
_ic_enable:
  /* Disable IC */
  l.mfspr r13,r0,SPR_SR
  l.addi  r11,r0,-1
  l.xori  r11,r11,SPR_SR_ICE
  l.and   r11,r13,r11
  l.mtspr r0,r11,SPR_SR

  /* Invalidate IC */
  l.addi  r13,r0,0
  l.addi  r11,r0,IC_SIZE
1:
  l.mtspr r0,r13,SPR_ICBIR
  l.sfne  r13,r11
  l.bf    1b
  l.addi  r13,r13,IC_LINE

  /* Enable IC */
  l.mfspr r13,r0,SPR_SR
  l.ori   r13,r13,SPR_SR_ICE
  l.mtspr r0,r13,SPR_SR
  l.nop
  l.nop
  l.nop
  l.nop
  l.nop

  l.jr    r9
  l.nop
 
/*
 * Instruction cache disable
 *      ic_disable()
 */
	.global _ic_disable
_ic_disable:
  /* Disable IC */
  l.mfspr r13,r0,SPR_SR
  l.addi  r11,r0,-1
  l.xori  r11,r11,SPR_SR_ICE
  l.and   r11,r13,r11
  l.mtspr r0,r11,SPR_SR

  l.jr    r9
  l.nop

/*
 * Instruction cache invalidate
 *      ic_flush()
 */
	.global _ic_invalidate
_ic_invalidate:
  /* Disable IC */
  l.mfspr r13,r0,SPR_SR
  l.addi  r11,r0,-1
  l.xori  r11,r11,SPR_SR_ICE
  l.and   r11,r13,r11
  l.mtspr r0,r11,SPR_SR

  /* Invalidate IC */
  l.addi  r13,r0,0
  l.addi  r11,r0,IC_SIZE
1:
  l.mtspr r0,r13,SPR_ICBIR
  l.sfne  r13,r11
  l.bf    1b
  l.addi  r13,r13,IC_LINE

  /* Enable IC */
  l.mfspr r13,r0,SPR_SR
  l.ori   r13,r13,SPR_SR_ICE
  l.mtspr r0,r13,SPR_SR
  l.nop
  l.nop
  l.nop
  l.nop
  l.nop

  l.jr    r9
  l.nop

/*
 * Data cache enable
 *      dc_enable()
 */
	.global _dc_enable
_dc_enable:
  /* Disable DC */
  l.mfspr r13,r0,SPR_SR
  l.addi  r11,r0,-1
  l.xori  r11,r11,SPR_SR_DCE
  l.and   r11,r13,r11
  l.mtspr r0,r11,SPR_SR

  /* Flush DC */
  l.addi  r13,r0,0
  l.addi  r11,r0,DC_SIZE
1:
  l.mtspr r0,r13,SPR_DCBIR
  l.sfne  r13,r11
  l.bf    1b
  l.addi  r13,r13,DC_LINE

  /* Enable DC */
  l.mfspr r13,r0,SPR_SR
  l.ori   r13,r13,SPR_SR_DCE
  l.mtspr r0,r13,SPR_SR

  l.jr    r9
  l.nop
 
/*
 * Data cache disable
 *      dc_disable()
 */
	.global _dc_disable
_dc_disable:
  /* Disable DC */
  l.mfspr r13,r0,SPR_SR
  l.addi  r11,r0,-1
  l.xori  r11,r11,SPR_SR_DCE
  l.and   r11,r13,r11
  l.mtspr r0,r11,SPR_SR

  l.jr    r9
  l.nop
 
/*
 * Invalidate data cache line
 *      dc_line_invalidate(long ph_add)
 */
	.global _dc_line_invalidate
_dc_line_invalidate:
  l.mfspr r4,r0,SPR_SR
  l.addi  r5,r0,-1
  l.xori  r5,r5,SPR_SR_DCE
  l.and   r5,r4,r5
  l.mtspr r0,r5,SPR_SR
  l.mtspr r0,r3,SPR_DCBIR
  l.mtspr r0,r4,SPR_SR
  l.jr    r9
  l.nop

/*
 * Data MMU enable
 *      dmmu_enable()
 */
	.global _dmmu_enable
_dmmu_enable:
  /* Invalidate all sets */
  l.addi  r11,r0,DMMU_SET_NB
  l.addi  r13,r0,0
1:
  l.mtspr r13,r0,SPR_DTLBMR_BASE(0)
  l.addi  r11,r11,-1
  l.sfeqi r11,0
  l.bnf   1b
  l.addi  r13,r13,1
  l.mfspr r11,r0,SPR_SR
  l.ori   r11,r11,SPR_SR_DME
  l.mtspr r0,r11,SPR_SR
  l.jr    r9
  l.nop

/*
 * Instruction MMU enable
 *      immu_enable()
 */
	.global _immu_enable
_immu_enable:
  /* Invalidate all sets */
  l.addi  r11,r0,IMMU_SET_NB
  l.addi  r13,r0,0
1:
  l.mtspr r13,r0,SPR_ITLBMR_BASE(0)
  l.addi  r11,r11,-1
  l.sfeqi r11,0
  l.bnf   1b
  l.addi  r13,r13,1
  l.mfspr r11,r0,SPR_SR
  l.ori   r11,r11,SPR_SR_IME
  l.mtspr r0,r11,SPR_SR
  l.nop
  l.nop
  l.nop
  l.nop
  l.jr    r9
  l.nop

 /*
 * Print utility
 *      __print(const char *fmt, ...)
 */
	.global __print
__print:
	l.lwz	r3,0(r1)
	l.addi	r4,r1,4
#	l.sys	202
  l.nop 3
	l.jr	r9
	l.nop

