/* linux/arch/m68knommu/kernel/head.S:  A startup file for the MC68332 
 *
 * Copyright (C) 2002  Mecel AB (www.mecel.se)
 *
 * This version is part of booting kernel from Compact Flash to RAM concept. 
 *
 *
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 *
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
 */

#define ASSEMBLY

#include <linux/config.h>
#include <asm/shglcore.h>
#include <asm/MC68332.h>

	.global _start
	.global _stext
	.global _romstart
	.global _romend
	.global _ramvec
	.global __bss_start
	.global _ramend
	.global main

	
	
	.text

_start:
_stext:

	move    #0x2700,%sr         		/* mask all interrupts during startup	*/

	move.b  #0x00, 0xfffa1f		 		/* Turn off some interupts enabled in bootcode */

	andi.w  #0xFFEF, 0xFFF438	 		/* Orange diod on*/

/* Configure Serial port */
	move.w  #0xd701,SYNCR_ADDR	
	move.b  #0x00,PEPAR_ADDR
	move.b  #0x72,DDRE_ADDR
	move.w  #0x42CF,SIMCR_ADDR
	move.b  #0x04,SYPCR_ADDR
	move.b  #0x70,PORTE_ADDR 			/* Set proper output, and activate the serial port */

	move.w  #0x00,QSMCR_ADDR 			/* Turn off supervisor mode */
	move.w	#0x0d,SCCR0_ADDR			/* 57600 BPS */

/*	move.w	#0x14,SCCR0_ADDR */			/* 38400 BPS */
/*	move.w	#0x4e,SCCR0_ADDR */			/* 9600 BPS */

	move.w	#0xc,SCCR1_ADDR 			/* Enable Read/Write */


/* Copy command line from beginning of RAM (+16) to end of bss */
/*	movel	#_ramvec, %d7
	addl	#16, %d7
	moveal	%d7, %a0
	moveal	#end, %a1
	lea	%a1@(512), %a2
*/
	/* Copy %a0 to %a1 until %a1 == %a2 */
/*L2:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	L2
*/
	/* Copy data segment from ROM to RAM */
/*	moveal	#__data_rom_start, %a0
	moveal	#__data_start, %a1
	moveal	#__data_end, %a2
*/
	/* Copy %a0 to %a1 until %a1 == %a2 */
/*LD1:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	LD1

*/
	moveal	#__bss_start, %a0
	moveal	#_ramend, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	/* Copy command line from end of bss to command line */
/*	moveal	#end, %a0
	moveal	#command_line, %a1
	lea	%a1@(512), %a2
*/
	/* Copy %a0 to %a1 until %a1 == %a2 */
/*L3:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	L3
*/

	/* Super stack pointer setup */

	movel	#_ramend,%d0
	subl	#32768,	%d0			/* Reserve 32K of super stack */

	/*subl	#4096,	%d0	*/		/* Reserve 4K of super stack */
	movel	%d0,	memory_end
	moveal	#_ramend, %ssp

	movel	#_romend,	%d0
	movel 	#_romstart,	%d1
	subl	%d0,	%d1
	movel	%d0,	rom_length
	
	pea		0
	pea		env
	pea		%sp@(4)
	pea		0

	move.l  #_ramvec,%d0        /* Set up vector base register        */
	movec.l %d0,%vbr         

	andi.w  #0xFFEF, 0xFFF440	 /* Red diod on*/
	move.l	0x900000, %d0
	move.l	0x900004, %d0


	jsr	start_kernel
_exit:

	jmp	_exit


putc:

#ifdef CONFIG_68332_SERIAL
	movew	%d7,0xfffc0e
LSTL:
	movew	0xfffc0c,%d7
	andw	#0x0100,%d7
	beq	LSTL
#endif
	rts

	.data
env:
	.long	0


.section .ramvec  /* This is the VBR definition */ 

    .long   _ramend			/* Reset: Initial Stack Pointer                 - 0.  */
    .long   _start      	/* Reset: Initial Program Counter               - 1.  */
    .long   buserr      	/* Bus Error                                    - 2.  */
    .long   trap        	/* Address Error                                - 3.  */
    .long   inthandler      /* Illegal Instruction                          - 4.  */
    .long   trap        	/* Divide by zero                               - 5.  */
    .long   trap        	/* CHK, CHK2 Instructions                       - 6.  */
    .long   trap        	/* TRAPcc, TRAPV Instructions                   - 7.  */
    .long   inthandler		/* Privilege Violation                          - 8.  */
    .long   trap        	/* Trace                                        - 9.  */
    .long   inthandler		/* Line 1010 Emulator                           - 10. */
    .long   inthandler		/* Line 1111 Emualtor                           - 11. */
    .long   trap        	/* Harware Breakpoint                           - 12. */
    .long   trap        	/* (Reserved for Coprocessor Protocol Violation)- 13. */
    .long   inthandler		/* Format Error                                 - 14. */
    .long   inthandler		/* Uninitialized Interrupt                      - 15. */
    .long   inthandler   	/* (Unassigned, Reserver)                       - 16. */
    .long   inthandler   	/* (Unassigned, Reserver)                       - 17. */
    .long   inthandler    	/* (Unassigned, Reserver)                       - 18. */
    .long   inthandler   	/* (Unassigned, Reserver)                       - 19. */
    .long   inthandler   	/* (Unassigned, Reserver)                       - 20. */
    .long   inthandler    	/* (Unassigned, Reserver)                       - 21. */
    .long   inthandler     	/* (Unassigned, Reserver)                       - 22. */
    .long   inthandler    	/* (Unassigned, Reserver)                       - 23. */
    .long   inthandler     	/* Spurious Interrupt                           - 24. */
    .long   inthandler    	/* Level 1 Interrupt Autovector                 - 25. */
    .long   inthandler     	/* Level 2 Interrupt Autovector                 - 26. */
    .long   inthandler   	/* Level 3 Interrupt Autovector                 - 27. */
    .long   inthandler    	/* Level 4 Interrupt Autovector                 - 28. */
    .long   inthandler    	/* Level 5 Interrupt Autovector                 - 29. */
    .long   inthandler   	/* Level 6 Interrupt Autovector                 - 30. */
    .long   inthandler   	/* Level 7 Interrupt Autovector                 - 31. */
    .long   system_call   	/* System Call (Trap Instruction Vector  0)     - 32. */
    .long   inthandler     	/* Trap Instruction Vector 1                    - 33. */
    .long   inthandler    	/* Trap Instruction Vector 2                    - 34. */
    .long   inthandler    	/* Trap Instruction Vector 3                    - 35. */
    .long   inthandler    	/* Trap Instruction Vector 4                    - 36. */
    .long   inthandler   	/* Trap Instruction Vector 5                    - 37. */
    .long   inthandler   	/* Trap Instruction Vector 6                    - 38. */
    .long   inthandler     	/* Trap Instruction Vector 7                    - 39. */
    .long   inthandler    	/* Trap Instruction Vector 8                    - 40. */
    .long   inthandler     	/* Trap Instruction Vector 9                    - 41. */
    .long   inthandler     	/* Trap Instruction Vector 10                   - 42. */
    .long   inthandler    	/* Trap Instruction Vector 11                   - 43. */
    .long   inthandler     	/* Trap Instruction Vector 12                   - 44. */
    .long   inthandler    	/* Trap Instruction Vector 13                   - 45. */
    .long   inthandler     	/* Trap Instruction Vector 14                   - 46. */
    .long   inthandler     	/* Trap Instruction Vector 15                   - 47. */
 	.long   inthandler		/* 49 */
	.long   inthandler		/* 50 */
	.long   inthandler		/* 51 */
	.long   inthandler		/* 52 */
	.long   inthandler		/* 53 */
	.long   inthandler		/* 54 */
	.long   inthandler		/* 55 */
	.long   inthandler		/* 56 */
	.long   inthandler		/* 57 */
	.long   inthandler		/* 58 */
	.long   inthandler		/* 59 */
	.long   inthandler		/* 60 */
	.long   inthandler		/* 61 */
	.long   inthandler		/* 62 */
	.long   inthandler		/* 63 */

	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* Bind all user interrupts to inthandler */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* 64 - 127 */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler
	  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* Bind all user interrupts to inthandler */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* 128 - 191 */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler
	  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* Bind all user interrupts to inthandler */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  /* 192 - 255 */
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	.long   inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler, inthandler  
	