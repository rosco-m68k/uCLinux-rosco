/* arch/m68knommu/platform/68EZ328/ads_flash-head.S
 * 
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on: arch/m68knommu/platform/68EZ328/pilot-head.S
 * Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 */

	.global _stext
	.global _start
	.global __bss_start
	.global _ramend

#define ASSEMBLY

#include <linux/config.h>
#include <asm/MC68EZ328.h>

	.text
_start:
_stext:
        /*
         * Program the basic registers. Only those registers that
         * are required at startup should be initialized with the
         * values required for startup. All other registers should
         * be initialized by related components.
         */
/*
 * Use dual mapping of the CPU register space and allow access to them
 * from the user mode (this is the default, anyway). Also enable bus
 * timeout watchdog.
 */
#define SCR_INIT (SCR_DMAP | SCR_BETEN)

/*
 * Disable the watchdog timer for now
 */
#define WATCHDOG_INIT 0

/*
 * Connect internal chip functions to those pins that are required for
 * startup. Device drivers will do the rest
 */

/*
 * Port B. We need:
 *       CSC[01]/RAS[01] and CSD[01]/CAS[01] for our 2 DRAM  chips
 */      
#define PBSEL_INIT (~(PB_CSC0_RAS0 | \
                      PB_CSC1_RAS1 | \
                      PB_CSD0_CAS0 | \
                      PB_CSD1_CAS1))

/*
 * Port E. We need
 *	DWE    for DRAM Write Enable
 *      RXD    RS-232 receive
 *      TXD    RS-232 Transmit
 *      RTS    RS-232 RTS
 *      CTS    RS-232 CTS
 */
#define PESEL_INIT (~(PE_DWE | PE_RXD | PE_TXD | PE_RTS | PE_CTS))

/*
 * Port F. We need
 *	CSA1	to address the second FLASH chip
 */
#define PFSEL_INIT (~(PF_CSA1))

/*
 * In-circuit emulation should be disabled
 */
#define ICEMCR_INIT (ICEMCR_HMDIS)

        moveb   #SCR_INIT,      SCR_ADDR 
        movew   #WATCHDOG_INIT, WATCHDOG_ADDR
        moveb   #PBSEL_INIT,    PBSEL_ADDR
        moveb   #PESEL_INIT,    PESEL_ADDR
        moveb   #PFSEL_INIT,    PFSEL_ADDR
        movew   #ICEMCR_INIT,   ICEMCR_ADDR
        moveb   #0x40,          IVR_ADDR    /* Use 0x40-0x47 for IRQ 0-7 */
        movel   #0x00FFFFFF,    IMR_ADDR    /* Mask all h/w interrupts */

        /*
         * Program Chip selects & DRAM controller
         *
         * CSA[01]           FLASH      10200000 -- 10200000
         * CAS[01]/RAS[01]   DRAM       00000000 -- 00400000
         */

#define CSGBA_INIT (0x10000000 >> 13)      /* FLASH starts at 0x10000000 */
#define CSGBD_INIT (0x00000000 >> 13)      /* DRAM  starts at 0x00000000 */

#define CSA_INIT \
        (CSA_FLASH              |       /* CSA controls FLASH memory */ \
         CSA_BSW                |       /* 16-bit bus Width */          \
         PUT_FIELD(CSA_WS, 2)   |       /* 2 Wait States */             \
         PUT_FIELD(CSA_SIZ, 3)  |       /* Total FLASH size is 2*1M */  \
         CSA_EN)                        /* Enable it */

#define CSD_INIT \
        (CSD_DRAM               |       /* CSD Controls DRAM */         \
         CSD_BSW                |       /* 16-bit bus width */          \
         PUT_FIELD(CSD_WS, 1)   |       /* 1 Wait State */              \
         PUT_FIELD(CSD_SIZ, 6)  |       /* The board has 2*2MB DRAM*/   \
         CSD_EN)                        /* Enable it */

#define DRAMMC_INIT \
        (DRAMMC_ROW12_PA23      |       /* PA23 -> MD12 */      \
         DRAMMC_ROW11           |       /* PA22 -> MD11 */      \
         DRAMMC_ROW10           |       /* PA21 -> MD10 */      \
         DRAMMC_ROW9            |       /* PA19 -> MD9  */      \
         DRAMMC_ROW8)                   /* PA20 -> MD8  */
 
#define DRAMC_INIT \
        (DRAMC_EN               |       /* Enable DRAM controller */    \
         DRAMC_EDO              |       /* We have EDO DRAM */          \
         PUT_FIELD(DRAMC_WS, 1) |       /* 1 Wait State */              \
         DRAMC_LPR              |       /* Low-Power Refresh Enable */  \
         DRAMC_RST              |       /* Reset Burst Refresh Enable */\
         DRAMC_DWE)                     /* DRAM Write Enable */

        movew   #CSGBA_INIT,  CSGBA_ADDR        /* FLASH */
        movew   #CSA_INIT,    CSA_ADDR

        movew   #DRAMMC_INIT, DRAMMC_ADDR       /* DRAM */
        movew   #DRAMC_INIT,  DRAMC_ADDR
        movew   #CSGBD_INIT,  CSGBD_ADDR
        movew   #CSD_INIT,    CSD_ADDR

        /*
         * Program the UART (115200 cs8 -parenb -cstopb -crtscts */
         */
#define PLLCR_INIT \
        (PUT_FIELD(PLLCR_SYSCLK_SEL, 4) | PUT_FIELD(PLLCR_LCDCLK_SEL, 4))
 
#define USTCNT_INIT \
        (USTCNT_UEN | USTCNT_RXEN | USTCNT_TXEN | USTCNT_8_7)
 
#define UBAUD_INIT \
        (PUT_FIELD(UBAUD_DIVIDE, 0) | PUT_FIELD(UBAUD_PRESCALER, 0x38))
 
#define UTX_INIT \
        (UTX_NOCTS)
 
        movew   #(USTCNT_UEN | USTCNT_RXEN), USTCNT_ADDR
        tstw    USTCNT_ADDR
        movew   #USTCNT_INIT, USTCNT_ADDR
        movew   #PLLCR_INIT,  PLLCR_ADDR
        movew   #UBAUD_INIT,  UBAUD_ADDR
        moveb   #(UTX_INIT>>8),    UTX_ADDR                                     

	movew	#0x2700, %sr

	/*
	 * Here we start doing some real things
	 */
	moveq	#13, %d7		/* '\r' */
	jsr putc

	moveq	#10, %d7		/* '\n' */
	jsr putc

	moveq	#65, %d7		/* 'A' */
	jsr putc

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	moveq	#66, %d7		/* 'B' */
	jsr	putc

	moveal	#__bss_start, %a0
	moveal	#end, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	moveq	#67, %d7		/* 'C' */
	jsr	putc

        /* Copy data segment from ROM to RAM */
        moveal  #__data_rom_start, %a0
        moveal  #__data_start, %a1
        moveal  #__data_end, %a2
        moveq   #68, %d7
        jsr     putc                    /* 'D' */
 
        /* Copy %a0 to %a1 until %a1 == %a2 */
L2:
        movel   %a0@+, %d0
        movel   %d0, %a1@+
        cmpal   %a1, %a2
        bhi     L2

        moveq   #69, %d7                /* 'E' */
        jsr     putc                                                            

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveq	#70, %d7		/* 'F' */
	jsr	putc


lp:
	jsr	start_kernel
        jmp lp
_exit:

	jmp	_exit


putc:
	moveb	%d7,UTX_TXDATA_ADDR
pclp:
	movew	UTX_ADDR, %d7
	andw	#UTX_TX_AVAIL, %d7
	beq	pclp
	rts

	.data
env:
	.long	0
	.text
