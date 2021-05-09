/* arch/m68knommu/platform/68360/uCquicc/crt0_rom.S
 *
 * Startup code for Motorola 68360
 *
 * Copyright (C) Michael Leslie <mleslie@lineo.com>
 * Based on: arch/m68knommu/platform/68EZ328/ucsimm/crt0_rom.S
 * Copyright (C) 1998  D. Jeff Dionne <jeff@uclinux.org>,
 *
 */
#include "linux/autoconf.h"
        .global _stext
        .global _start
        .global __bss_start
        .global _quicc_base
        .global _periph_base
        .global _dprbase
        .global _boot_stack
        .global __kernel_image_start
        .global __kernel_image_end

#define REGB                        0x1000
#define PEPAR                       (_dprbase + REGB + 0x0016)
#define GMR                         (_dprbase + REGB + 0x0040)
#define BR0                         (_dprbase + REGB + 0x0050)
#define OR0                         (_dprbase + REGB + 0x0054)
#define BR1                         (_dprbase + REGB + 0x0060)
#define OR1                         (_dprbase + REGB + 0x0064)
#define BR2                         (_dprbase + REGB + 0x0070)
#define OR2                         (_dprbase + REGB + 0x0074)
#define BR3                         (_dprbase + REGB + 0x0080)
#define OR3                         (_dprbase + REGB + 0x0084)
#define BR4                         (_dprbase + REGB + 0x0090)
#define OR4                         (_dprbase + REGB + 0x0094)
#define BR5                         (_dprbase + REGB + 0x00A0)
#define OR5                         (_dprbase + REGB + 0x00A4)
#define BR6                         (_dprbase + REGB + 0x00b0)
#define OR6                         (_dprbase + REGB + 0x00b4)
#define BR7                         (_dprbase + REGB + 0x00c0)
#define OR7                         (_dprbase + REGB + 0x00c4)

#define MCR                         (_dprbase + REGB + 0x0000)
#define AVR                         (_dprbase + REGB + 0x0008)

#define SYPCR                       (_dprbase + REGB + 0x0022)

#define PLLCR                       (_dprbase + REGB + 0x0010)
#define CLKOCR                      (_dprbase + REGB + 0x000C)
#define CDVCR                       (_dprbase + REGB + 0x0014)

#define BKAR                        (_dprbase + REGB + 0x0030)
#define BKCR                        (_dprbase + REGB + 0x0034)
#define SWIV                        (_dprbase + REGB + 0x0023)
#define SWSR                        (_dprbase + REGB + 0x002F)
#define PICR                        (_dprbase + REGB + 0x0026)
#define PITR                        (_dprbase + REGB + 0x002A)

#define PBODR                       (_dprbase + REGB + 0x06C2)
#define PBPAR                       (_dprbase + REGB + 0x06BC)
#define PBDIR                       (_dprbase + REGB + 0x06B8)
#define PBDAT                       (_dprbase + REGB + 0x06C4)

/* Define for all memory configuration */
#define MCU_SIM_GMR                 0x00000000


#define MCU_SIM_MBAR_ADRS           0x0003ff00
#define MCU_SIM_MBAR_BA_MASK        0xfffff000
#define MCU_SIM_MBAR_AS_MASK        0x00000001

#define MCU_SIM_PEPAR               0x00B4
    
#define MCU_DISABLE_INTRPTS         0x2700
#define MCU_SIM_AVR                 0x50
    
#define MCU_SIM_MCR                 0x00005cff

#define MCU_SIM_CLKOCR              0x8f
#define MCU_SIM_PLLCR               0x8000
#define MCU_SIM_CDVCR               0x0000

#define MCU_SIM_SYPCR               0xEC
/*Watchdog  - 16s / 31.4ms
                 - Watchdog enabled
                 - Watchdog resets the system -needed for hard reset to work.
                 - Double bus fault monitor enabled
                 - Bus monitor external enabled, 1K system clocks */
#define MCU_SIM_SWIV                0x00
#define MCU_SIM_PICR                0x0000
#define MCU_SIM_PITR                0x0200
    /* Select 16s on watchdog. */

#define MCU_WD_KICK1                0x55
#define MCU_WD_KICK2                0xAA
#define MCU_KICK_WATCHDOG   \
    move.w  #MCU_WD_KICK1, SWSR ; \
    move.w  #MCU_WD_KICK2, SWSR

#define SIM_OR0 0x2FF80004
#define SIM_BR0 0x00000001

#define SIM_OR4 0x2F000000
#define SIM_BR4 0x01000001

#define SIM_BRE 0x00000000
#define SIM_ORE 0xF0000000

/* By the time this RAM specific code begins to execute, DPRAM
 * and DRAM should already be mapped and accessible. */

	.text
.section ".text.bootrom","awx"
_start:
_stext:
    nop
    ori.w   #MCU_DISABLE_INTRPTS,   %sr     /* disable interrupts: */
    movea.l	#_boot_stack,           %sp     /*set up stack at the end of DRAM:*/


    moveq.l #0x07,                  %d1     /* Setup MBAR */
    movec	%d1,                    %dfc

    lea.l   MCU_SIM_MBAR_ADRS,      %a0
    move.l  #_dprbase,              %d0
    andi.l  #MCU_SIM_MBAR_BA_MASK,  %d0
    ori.l   #MCU_SIM_MBAR_AS_MASK,  %d0
    moves.l %d0,                    %a0@

    moveq.l #0x05,                  %d1
    movec.l %d1,                    %dfc

    /* Set Module Configuration Register */
    move.l  #MCU_SIM_MCR,           MCR
    move.b  #MCU_SIM_AVR,           AVR

/* TODO: Determine cause of reset */


    /*
    *       configure system clock MC68360 p. 6-40
    *       (value +1)*osc/128 = system clock
    */
set_sim_clock:
    move.w  #MCU_SIM_PLLCR,         PLLCR
    move.b  #MCU_SIM_CLKOCR,        CLKOCR
    move.w  #MCU_SIM_CDVCR,         CDVCR

/*
    // Wait for the PLL to settle - only needed if you change the pll setting
    move.w      #16384, %d0
pll_settle_wait:
    subi.w       #1, %d0
    bne	pll_settle_wait
*/

    /* Setup the system protection register, and watchdog timer register */
    MCU_KICK_WATCHDOG
    move.b      #MCU_SIM_SWIV,      SWIV
    move.w      #MCU_SIM_PICR,      PICR
    move.w      #MCU_SIM_PITR,      PITR
    move.b      #MCU_SIM_SYPCR,     SYPCR

/* Clear DPRAM */
    move.l  #_dprbase,              %a0
    move.l  #0x0cbf,                %d0
    lsr.l   #2,                     %d0
    subq.l  #1,                     %d0
clear_dpram_1_move:
    move.l  #0,                     %a0@+
    dbf     %d0,                    clear_dpram_1_move

    move.l  #_dprbase,              %d0
    add.l   #0x0D00,                %d0
clear_dpram_3:
    move.l  %d0,                    %a0
    move.l  #0x02E,                 %d0 /* (BF >> 2) - 1 = 2E */
clear_dpram_2:
    move.l  #0,                     %a0@+
    dbf     %d0,                    clear_dpram_2

    move.l  #_dprbase,              %d0
    add.l   #0x0E00,                %d0
    cmp.l   %a0,                    %d0
    bls     clear_dpram_3
    add.l   #0x0100,                %d0
    cmp.l   %a0,                    %d0
    bls     clear_dpram_3

/* Set up memory controller: */
    /*
    *   Set up Global Memory Register (GMR)
    */
    move.l      #MCU_SIM_GMR,   %d0
    move.l      %d0, GMR

    move.l      #SIM_OR0,       OR0
    move.l      #SIM_BR0,       BR0

    move.l      #SIM_ORE,       OR1
    move.l      #SIM_BRE,       BR1
    
    move.l      #SIM_ORE,       OR2
    move.l      #SIM_BRE,       BR2

    move.l      #SIM_ORE,       OR3
    move.l      #SIM_BRE,       BR3

    move.l      #SIM_OR4,       OR4
    move.l      #SIM_BR4,       BR4

    move.l      #SIM_ORE,       OR5
    move.l      #SIM_BRE,       BR5

    move.l      #SIM_ORE,       OR6
    move.l      #SIM_BRE,       BR6

    move.l      #SIM_ORE,       OR7
    move.l      #SIM_BRE,       BR7

    move.w      #MCU_SIM_PEPAR, PEPAR 

    MCU_KICK_WATCHDOG
/* point to vector table: */
    move.l  #_romvec,   %a0
    move.l	#_ramvec,   %a1
    move.l  #256,       %d0
    bra     LD_VECTORS_END
LD_VECTORS:
    move.l  %a0@+,      %a1@+
LD_VECTORS_END:
    dbf     %d0,        LD_VECTORS

    move.l	#_ramvec, %a1
    movec	%a1, %vbr



    MCU_KICK_WATCHDOG
    /* Copy ROM Kernel Image to RAM so the kernel can run from RAM */
    move.l  #__kernel_image_start,  %a0
    move.l  #__kernel_image_end,    %d0
    subi.l  #__kernel_image_start,  %d0
    lsr.l   #2,                     %d0
    move.l  #text_start,            %a1
    bra     LD_KERNEL_END
LD_KERNEL:
    move.l  %a0@+,                  %a1@+
LD_KERNEL_END:
    dbf     %d0,                    LD_KERNEL
    sub.l   #0x00010000,            %d0
    bhs     LD_KERNEL

    MCU_KICK_WATCHDOG
    /* Copy data segment from ROM to RAM */
    move.l  #__data_rom_start,  %a0
    move.l  #__data_end,        %d0
    subi.l  #__data_start,      %d0
    lsr.l   #2,                 %d0
    move.l  #__data_start,      %a1
    bra     LD_DATA_END
LD_DATA:
    move.l  %a0@+,              %a1@+
LD_DATA_END:
    dbf     %d0,                LD_DATA
    sub.l   #0x00010000,        %d0
    bhs     LD_DATA

    move.l  #end,               %d0
    subi.l  #__bss_start,       %d0
    lsr.l   #2,                 %d0
    move.l  #__bss_start,       %a0
    bra     LD_BSS_END
LD_BSS:
    clr.l   %a0@+
LD_BSS_END:
    dbf     %d0,                LD_BSS
    sub.l   #0x00010000,        %d0
    bhs     LD_BSS

    /* Copy the command line from rom to ram */
    move.l  #_sios_command_line,        %a0
    move.l  #command_line,              %a1
    move.l  #_sios_command_line_end,    %d0
    sub.l   #_sios_command_line,        %d0
    bra     COPY_COMMAND_LINE_END
COPY_COMMAND_LINE:
    move.b  %a0@+,                  %a1@+
COPY_COMMAND_LINE_END:
    dbf     %d0,                    COPY_COMMAND_LINE
    sub.l   #0x00010000,            %d0
    bhs     COPY_COMMAND_LINE

load_quicc:
    move.l  #_dprbase,  _quicc_base
finished_load_quicc_base:

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

lp:
    jsr	start_kernel
    jmp lp

_exit:

	jmp	_exit

_sios_command_line:
    .ascii "root=/dev/flash1 console=/dev/ttyS0,19200\0"
_sios_command_line_end:

.data
env:
	.long	0
_quicc_base:
	.long	0
_periph_base:
	.long	0
	.text
	
    /*
     * These are the exception vectors at boot up, they are copied into RAM
     * and then overwritten as needed.
     */
 
.section ".data.initvect","awx"
    .long   _boot_stack /* Reset: Initial Stack Pointer                 - 0.  */
    .long   _start      /* Reset: Initial Program Counter               - 1.  */
    .long   buserr      /* Bus Error                                    - 2.  */
    .long   trap        /* Address Error                                - 3.  */
    .long   trap        /* Illegal Instruction                          - 4.  */
    .long   trap        /* Divide by zero                               - 5.  */
    .long   trap        /* CHK, CHK2 Instructions                       - 6.  */
    .long   trap        /* TRAPcc, TRAPV Instructions                   - 7.  */
    .long   trap        /* Privilege Violation                          - 8.  */
    .long   trap        /* Trace                                        - 9.  */
    .long   trap        /* Line 1010 Emulator                           - 10. */
    .long   trap        /* Line 1111 Emualtor                           - 11. */
    .long   trap        /* Harware Breakpoint                           - 12. */
    .long   trap        /* (Reserved for Coprocessor Protocol Violation)- 13. */
    .long   trap        /* Format Error                                 - 14. */
    .long   trap        /* Uninitialized Interrupt                      - 15. */
    .long   trap        /* (Unassigned, Reserver)                       - 16. */
    .long   trap        /* (Unassigned, Reserver)                       - 17. */
    .long   trap        /* (Unassigned, Reserver)                       - 18. */
    .long   trap        /* (Unassigned, Reserver)                       - 19. */
    .long   trap        /* (Unassigned, Reserver)                       - 20. */
    .long   trap        /* (Unassigned, Reserver)                       - 21. */
    .long   trap        /* (Unassigned, Reserver)                       - 22. */
    .long   trap        /* (Unassigned, Reserver)                       - 23. */
    .long   trap        /* Spurious Interrupt                           - 24. */
    .long   trap        /* Level 1 Interrupt Autovector                 - 25. */
    .long   trap        /* Level 2 Interrupt Autovector                 - 26. */
    .long   trap        /* Level 3 Interrupt Autovector                 - 27. */
    .long   trap        /* Level 4 Interrupt Autovector                 - 28. */
    .long   trap        /* Level 5 Interrupt Autovector                 - 29. */
    .long   trap        /* Level 6 Interrupt Autovector                 - 30. */
    .long   trap        /* Level 7 Interrupt Autovector                 - 31. */
    .long   system_call /* Trap Instruction Vectors 0                   - 32. */
    .long   trap        /* Trap Instruction Vectors 1                   - 33. */
    .long   trap        /* Trap Instruction Vectors 2                   - 34. */
    .long   trap        /* Trap Instruction Vectors 3                   - 35. */
    .long   trap        /* Trap Instruction Vectors 4                   - 36. */
    .long   trap        /* Trap Instruction Vectors 5                   - 37. */
    .long   trap        /* Trap Instruction Vectors 6                   - 38. */
    .long   trap        /* Trap Instruction Vectors 7                   - 39. */
    .long   trap        /* Trap Instruction Vectors 8                   - 40. */
    .long   trap        /* Trap Instruction Vectors 9                   - 41. */
    .long   trap        /* Trap Instruction Vectors 10                  - 42. */
    .long   trap        /* Trap Instruction Vectors 11                  - 43. */
    .long   trap        /* Trap Instruction Vectors 12                  - 44. */
    .long   trap        /* Trap Instruction Vectors 13                  - 45. */
    .long   trap        /* Trap Instruction Vectors 14                  - 46. */
    .long   trap        /* Trap Instruction Vectors 15                  - 47. */
    .long   0           /* (Reserved for Coprocessor)                   - 48. */
    .long   0           /* (Reserved for Coprocessor)                   - 49. */
    .long   0           /* (Reserved for Coprocessor)                   - 50. */
    .long   0           /* (Reserved for Coprocessor)                   - 51. */
    .long   0           /* (Reserved for Coprocessor)                   - 52. */
    .long   0           /* (Reserved for Coprocessor)                   - 53. */
    .long   0           /* (Reserved for Coprocessor)                   - 54. */
    .long   0           /* (Reserved for Coprocessor)                   - 55. */
    .long   0           /* (Reserved for Coprocessor)                   - 56. */
    .long   0           /* (Reserved for Coprocessor)                   - 57. */
    .long   0           /* (Reserved for Coprocessor)                   - 58. */
    .long   0           /* (Unassigned, Reserved)                       - 59. */
    .long   0           /* (Unassigned, Reserved)                       - 60. */
    .long   0           /* (Unassigned, Reserved)                       - 61. */
    .long   0           /* (Unassigned, Reserved)                       - 62. */
    .long   0           /* (Unassigned, Reserved)                       - 63. */
    /*                  The assignment of these vectors to the CPM is         */
    /*                  dependant on the configuration of the CPM vba         */
    /*                          fields.                                       */
    .long   0           /* (User-Defined Vectors 1) CPM Error           - 64. */
    .long   0           /* (User-Defined Vectors 2) CPM Parallel IO PC11- 65. */
    .long   0           /* (User-Defined Vectors 3) CPM Parallel IO PC10- 66. */
    .long   0           /* (User-Defined Vectors 4) CPM SMC2 / PIP      - 67. */
    .long   0           /* (User-Defined Vectors 5) CPM SMC1            - 68. */
    .long   0           /* (User-Defined Vectors 6) CPM SPI             - 69. */
    .long   0           /* (User-Defined Vectors 7) CPM Parallel IO PC9 - 70. */
    .long   0           /* (User-Defined Vectors 8) CPM Timer 4         - 71. */
    .long   0           /* (User-Defined Vectors 9) CPM Reserved        - 72. */
    .long   0           /* (User-Defined Vectors 10) CPM Parallel IO PC8- 73. */
    .long   0           /* (User-Defined Vectors 11) CPM Parallel IO PC7- 74. */
    .long   0           /* (User-Defined Vectors 12) CPM Parallel IO PC6- 75. */
    .long   0           /* (User-Defined Vectors 13) CPM Timer 3        - 76. */
    .long   0           /* (User-Defined Vectors 14) CPM Reserved       - 77. */
    .long   0           /* (User-Defined Vectors 15) CPM Parallel IO PC5- 78. */
    .long   0           /* (User-Defined Vectors 16) CPM Parallel IO PC4- 79. */
    .long   0           /* (User-Defined Vectors 17) CPM Reserved       - 80. */
    .long   0           /* (User-Defined Vectors 18) CPM RISC Timer Tbl - 81. */
    .long   0           /* (User-Defined Vectors 19) CPM Timer 2        - 82. */
    .long   0           /* (User-Defined Vectors 21) CPM Reserved       - 83. */
    .long   0           /* (User-Defined Vectors 22) CPM IDMA2          - 84. */
    .long   0           /* (User-Defined Vectors 23) CPM IDMA1          - 85. */
    .long   0           /* (User-Defined Vectors 24) CPM SDMA Bus Err   - 86. */
    .long   0           /* (User-Defined Vectors 25) CPM Parallel IO PC3- 87. */
    .long   0           /* (User-Defined Vectors 26) CPM Parallel IO PC2- 88. */
    .long   0           /* (User-Defined Vectors 27) CPM Timer 1        - 89. */
    .long   0           /* (User-Defined Vectors 28) CPM Parallel IO PC1- 90. */
    .long   0           /* (User-Defined Vectors 29) CPM SCC 4          - 91. */
    .long   0           /* (User-Defined Vectors 30) CPM SCC 3          - 92. */
    .long   0           /* (User-Defined Vectors 31) CPM SCC 2          - 93. */
    .long   0           /* (User-Defined Vectors 32) CPM SCC 1          - 94. */
    .long   0           /* (User-Defined Vectors 33) CPM Parallel IO PC0- 95. */
    /*                  I don't think anything uses the vectors after here.   */
    .long   0           /* (User-Defined Vectors 34)                    - 96. */
    .long   0,0,0,0,0               /* (User-Defined Vectors 35  -  39). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 40  -  49). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 50  -  59). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 60  -  69). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 70  -  79). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 80  -  89). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 90  -  99). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 100 - 109). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 110 - 119). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 120 - 129). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 130 - 139). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 140 - 149). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 150 - 159). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 160 - 169). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 170 - 179). */
    .long   0,0,0,0,0,0,0,0,0,0     /* (User-Defined Vectors 180 - 189). */
    .long   0,0,0                   /* (User-Defined Vectors 190 - 192). */
.text
ignore: rte
