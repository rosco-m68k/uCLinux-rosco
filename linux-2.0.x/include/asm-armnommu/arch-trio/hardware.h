/*
 * linux/include/asm-arm/arch-trio/hardware.h
 *
 * Copyright (C) 1996 Russell King.
 *
 * This file contains the hardware definitions of the APLIO TRIO series machines.
 */

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

/*
 * What hardware must be present
 */
#ifndef __ASSEMBLER__

typedef unsigned long u_32;
/* ARM asynchronous clock */
#define ARM_CLK		((u_32)(24000000))
/* ARM synchronous with OAK clock */
#define A_O_CLK		((u_32)(20000000))

#else

#define ARM_CLK 24000000
#define A_O_CLK 20000000

#endif

#ifndef __ASSEMBLER__
/*
 * RAM definitions
 */
#define MAPTOPHYS(a)		((unsigned long)a)
#define KERNTOPHYS(a)		((unsigned long)(&a))
#define GET_MEMORY_END(p)	((p->u1.s.page_size) * (p->u1.s.nr_pages))
#define PARAMS_BASE			0x1000
//#define KERNEL_BASE		(PAGE_OFFSET + 0x80000)
#endif

#define IO_BASE		0
#define PERIPH_BASE 0xff000000
#define OAKA_PRAM	0xfd000000
#define OAKB_PRAM	0xfe000000

#define SRAM_BASE	0xfc000000
#define SRAM_SIZE	0x1000

#define DPMBA_BASE  0xfa000000
#define DPMBB_BASE  0xfb000000
#define DPMB_SIZE	0x800

#define BOOTROM_BASE 0xfb000000
#define BOOTROM_SIZE 0x400

/*
	Peripherials
*/	
#define SIAP_BASE	(PERIPH_BASE+0)
#define SMC_BASE	(PERIPH_BASE+0x4000)
#define DMC_BASE	(PERIPH_BASE+0x8000)
#define PIOA_BASE	(PERIPH_BASE+0xc000)
#define PIOB_BASE	(PERIPH_BASE+0x10000)
#define KB_BASE		(PERIPH_BASE+0x10000)
#define TMC_BASE	(PERIPH_BASE+0x14000)
#define USART0_BASE	(PERIPH_BASE+0x18000)
#define USART1_BASE	(PERIPH_BASE+0x1c000)
#define SPI_BASE	(PERIPH_BASE+0x20000)
#define WDG_BASE	(PERIPH_BASE+0x28000)
#define AIC_BASE	(PERIPH_BASE+0x30000)
/*
	SIAP registers
*/	
#define SIAP_MD   (SIAP_BASE)
#define SIAP_ID   (SIAP_BASE+4)
#define SIAP_RST   (SIAP_BASE+8)

/* SIAP mode register */
#define SIAP_SW2	(1<<11)
#define SIAP_SW1	(1<<10)
#define SIAP_LPCS(x)(x<<8 & 0x300)
#define SIAP_LP		(1<<6)
#define SIAP_CS		(1<<5)
#define SIAP_IB		(1<<4)
#define SIAP_IA		(1<<3)
#define SIAP_RB		(1<<2)
#define SIAP_RA		(1<<1)
#define SIAP_RM		(1)

/* SIAP ID register */
#define SIAP_PKG	(1<<31)
#define SIAP_VERS	(1)	

/* SIAP reset register */
#define	SIAP_RESET	(7)	

/*
	DRAM Memory controller registers
*/	
#define DMR0	(DMC_BASE + 0)		
#define DMR1	(DMC_BASE + 4)
#define DMC_CR	(DMC_BASE + 8)

/* DMRx registers */
#define DMR_EMR		1
#define DMR_PS(x)	(x<<1 & 6)
#define DMR_SZ(x)	(x<<3 & 0x18)

/* DMR memory control register */
#define DMR_ROR		(1<<2)
#define DMR_BBR		(1<<1)
#define DMR_DBW		1

/*
	Static Memory controller registers
*/	
#define SMC_CSR0	(SMC_BASE + 0)		
#define SMC_CSR1	(SMC_BASE + 4)		
#define SMC_CSR2	(SMC_BASE + 8)		
#define SMC_CSR3	(SMC_BASE + 0xc)		
#define SMC_MCR		(SMC_BASE + 0x24)		

/* SMC chip select registers */
#define SMC_CSEN	(1<<13)
#define SMC_BAT		(1<<12)
#define SMC_TDF(x)	(x<<9 & 0xe000)
#define SMC_PGS(x)	(x<<7 & 0x1800)
#define SMC_WSE		(1<<5)
#define SMC_NWS(x)	(x<<2 & 0x1c)
#define SMC_DBW(x)	(x & 3)

/* SMC memory control register */
#define SMC_DRP		(1<<4)

/*
	Dual Port Memory A
*/
#define DPMBA_S0	(DPMBA_BASE + 0x200)
#define DPMBA_S1	(DPMBA_BASE + 0x204)
#define DPMBA_S2	(DPMBA_BASE + 0x208)
#define DPMBA_S3	(DPMBA_BASE + 0x20c)
#define DPMBA_S4	(DPMBA_BASE + 0x210)
#define DPMBA_S5	(DPMBA_BASE + 0x214)
#define DPMBA_S6	(DPMBA_BASE + 0x218)
#define DPMBA_S7	(DPMBA_BASE + 0x21c)
#define DPMBA_CC	(DPMBA_BASE + 0x220)
/*
	Dual Port Memory B
*/	
#define DPMBB_S0	(DPMBB_BASE + 0x200)
#define DPMBB_S1	(DPMBB_BASE + 0x204)
#define DPMBB_S2	(DPMBB_BASE + 0x208)
#define DPMBB_S3	(DPMBB_BASE + 0x20c)
#define DPMBB_S4	(DPMBB_BASE + 0x210)
#define DPMBB_S5	(DPMBB_BASE + 0x214)
#define DPMBB_S6	(DPMBB_BASE + 0x218)
#define DPMBB_S7	(DPMBB_BASE + 0x21c)
#define DPMBB_CC	(DPMBB_BASE + 0x220)
/*
	Timer  registers
*/
#define TC0_BASE	(TMC_BASE + 0)
#define TC1_BASE	(TMC_BASE + 0x40)
#define TC2_BASE	(TMC_BASE + 0x80)

#define TC_OFFSET 0x14000
#define TC_BASE(i) (PERIPH_BASE+TC_OFFSET+(i)*0x40)

#define TC_BCR		(TMC_BASE + 0xC0)
#define TC_BMR		(TMC_BASE + 0xC4)

#ifndef __ASSEMBLER__
struct trio_timer_channel
{
	unsigned long ccr;				// channel control register		(WO)
	unsigned long cmr;				// channel mode register		(RW)
	unsigned long reserved[2];		
	unsigned long cv;				// counter value				(RW)
	unsigned long ra;				// register A					(RW)
	unsigned long rb;				// register B					(RW)
	unsigned long rc;				// register C					(RW)
	unsigned long sr;				// status register				(RO)
	unsigned long ier;				// interrupt enable register	(WO)
	unsigned long idr;				// interrupt disable register	(WO)
	unsigned long imr;				// interrupt mask register		(RO)
};

struct trio_timers
{
	struct {
		struct trio_timer_channel ch;
		unsigned char padding[0x40-sizeof(struct trio_timer_channel)];
	} chans[3];
	unsigned  long bcr;				// block control register		(WO)
	unsigned  long bmr;				// block mode	 register		(RW)
};

#endif

/*  TC control register */
#define TC_SYNC	(1)

/*  TC mode register */
#define TC2XC2S(x)	(x & 0x3)
#define TC1XC1S(x)	(x<<2 & 0xc)
#define TC0XC0S(x)	(x<<4 & 0x30)

/* TC channel control */
#define TC_CLKEN	(1)			
#define TC_CLKDIS	(1<<1)			
#define TC_SWTRG	(1<<2)			

/* TC interrupts enable/disable/mask and status registers */
#define TC_MTIOB	(1<<18)
#define TC_MTIOA	(1<<17)
#define TC_CLKSTA	(1<<16)

#define TC_ETRGS	(1<<7)
#define TC_LDRBS	(1<<6)
#define TC_LDRAS	(1<<5)
#define TC_CPCS		(1<<4)
#define TC_CPBS		(1<<3)
#define TC_CPAS		(1<<2)
#define TC_LOVRS	(1<<1)
#define TC_COVFS	(1)

#ifndef __ASSEMBLER__
/*
	UART registers
*/
struct uart_regs{
	u_32 cr;		// control 
	u_32 mr;		// mode
	u_32 ier;		// interrupt enable
	u_32 idr;		// interrupt disable
	u_32 imr;		// interrupt mask
	u_32 csr;		// channel status
	u_32 rhr;		// receive holding 
	u_32 thr;		// tramsmit holding		
	u_32 brgr;		// baud rate generator		
	u_32 rtor;		// rx time-out
	u_32 ttgr;		// tx time-guard
	u_32 res1;
	u_32 rpr;		// rx pointer
	u_32 rcr;		// rx counter
	u_32 tpr;		// tx pointer
	u_32 tcr;		// tx counter
	u_32 mc;		// modem control
	u_32 ms;		// modem status
};
#endif

/*  US control register */
#define US_SENDA	(1<<12)
#define US_STTO		(1<<11)
#define US_STPBRK	(1<<10)
#define US_STTBRK	(1<<9)
#define US_RSTSTA	(1<<8)
#define US_TXDIS	(1<<7)
#define US_TXEN		(1<<6)
#define US_RXDIS	(1<<5)
#define US_RXEN		(1<<4)
#define US_RSTTX	(1<<3)
#define US_RSTRX	(1<<2)

/* US mode register */
#define US_CLK0		(1<<18)
#define US_MODE9	(1<<17)
#define US_CHMODE(x)(x<<14 & 0xc000)
#define US_NBSTOP(x)(x<<12 & 0x3000)
#define US_PAR(x)	(x<<9 & 0xe00)
#define US_SYNC		(1<<8)
#define US_CHRL(x)	(x<<6 & 0xc0)
#define US_USCLKS(x)(x<<4 & 0x30)

/* US interrupts enable/disable/mask and status register */
#define US_DMSI		(1<<10)
#define US_TXEMPTY	(1<<9)
#define US_TIMEOUT	(1<<8)
#define US_PARE		(1<<7)
#define US_FRAME	(1<<6)
#define US_OVRE		(1<<5)
#define US_ENDTX	(1<<4)
#define US_ENDRX	(1<<3)
#define US_RXBRK	(1<<2)
#define US_TXRDY	(1<<1)
#define US_RXRDY	(1)

#define US_ALL_INTS (US_DMSI|US_TXEMPTY|US_TIMEOUT|US_PARE|US_FRAME|US_OVRE|US_ENDTX|US_ENDRX|US_RXBRK|US_TXRDY|US_RXRDY)


/* US modem control register */
#define US_FCM	(1<<5)
#define US_RTS	(1<<1)
#define US_DTR	(1)

/* US modem status register */
#define US_FCMS	(1<<8)
#define US_DCD	(1<<7)
#define US_RI	(1<<6)
#define US_DSR	(1<<5)
#define US_CTS	(1<<4)
#define US_DDCD	(1<<3)
#define US_TERI	(1<<2)
#define US_DDSR	(1<<1)
#define US_DCTS	(1)

/*
	Advanced Interrupt Controller registers
*/
#define AIC_SMR(i)  (AIC_BASE+i*4)
#define AIC_IVR	    (AIC_BASE+0x100)
#define AIC_FVR	    (AIC_BASE+0x104)
#define AIC_ISR	    (AIC_BASE+0x108)
#define AIC_IPR	    (AIC_BASE+0x10C)
#define AIC_IMR	    (AIC_BASE+0x110)
#define AIC_CISR	(AIC_BASE+0x114)
#define AIC_IECR	(AIC_BASE+0x120)
#define AIC_IDCR	(AIC_BASE+0x124)
#define AIC_ICCR	(AIC_BASE+0x128)
#define AIC_ISCR	(AIC_BASE+0x12C)
#define AIC_EOICR   (AIC_BASE+0x130)

/* AIC enable/disable/mask/pending registers */

#define AIC_PIOB	(1<<15)
#define AIC_URT1	(1<<14)
#define AIC_OAKB	(1<<13)
#define AIC_OAKA	(1<<12)
#define AIC_IRQ1	(1<<11)
#define AIC_IRQ0	(1<<10)
#define AIC_SPI		(1<<9)
#define AIC_LCD		(1<<8)
#define AIC_PIOA	(1<<7)
#define AIC_TC2		(1<<6)
#define AIC_TC1		(1<<5)
#define AIC_TC0		(1<<4)
#define AIC_URT0	(1<<3)
#define AIC_SWI		(1<<2)
#define AIC_WD		(1<<1)
#define AIC_FIQ		(1)
/*
	PIOA registers
*/	
#define PIOA_PER	(PIOA_BASE+0)			
#define PIOA_PDR	(PIOA_BASE+0x4)			
#define PIOA_PSR	(PIOA_BASE+0x8)			

#define PIOA_OER	(PIOA_BASE+0x10)		
#define PIOA_ODR	(PIOA_BASE+0x14)		
#define PIOA_OSR	(PIOA_BASE+0x18)		

#define PIOA_IFER	(PIOA_BASE+0x20)		
#define PIOA_IFDR	(PIOA_BASE+0x24)		
#define PIOA_IFSR	(PIOA_BASE+0x28)		

#define PIOA_SODR	(PIOA_BASE+0x30)		
#define PIOA_CODR	(PIOA_BASE+0x34)		
#define PIOA_ODSR	(PIOA_BASE+0x38)		

#define PIOA_PDSR	(PIOA_BASE+0x3C)		

#define PIOA_IER	(PIOA_BASE+0x40)		
#define PIOA_IDR	(PIOA_BASE+0x44)		
#define PIOA_IMR	(PIOA_BASE+0x48)		
#define PIOA_ISR	(PIOA_BASE+0x4C)		

/* PIOA bit allocation */
#define PIOA_TCLK0	(1<<8)					
#define PIOA_TI0A0	(1<<9)					
#define PIOA_TI0B0	(1<<10)					
#define PIOA_SCLKA	(1<<11)					
#define PIOA_NPCS1	(1<<12)					
#define PIOA_SCLKB	(1<<13)					
#define PIOA_NPCS2	(1<<14)					
#define PIOA_NPCS3	(1<<15)					
#define PIOA_TCLK2	(1<<16)					
#define PIOA_TIOA2	(1<<17)					
#define PIOA_TIOB2	(1<<18)					
#define PIOA_ACLK	(1<<19)					

/*
	PIOB registers
*/	
#define PIOB_PER	(PIOB_BASE+0)			
#define PIOB_PDR	(PIOB_BASE+0x4)			
#define PIOB_PSR	(PIOB_BASE+0x8)			

#define PIOB_OER	(PIOB_BASE+0x10)		
#define PIOB_ODR	(PIOB_BASE+0x14)		
#define PIOB_OSR	(PIOB_BASE+0x18)		

#define PIOB_IFER	(PIOB_BASE+0x20)		
#define PIOB_IFDR	(PIOB_BASE+0x24)		
#define PIOB_IFSR	(PIOB_BASE+0x28)		

#define PIOB_SODR	(PIOB_BASE+0x30)		
#define PIOB_CODR	(PIOB_BASE+0x34)		
#define PIOB_ODSR	(PIOB_BASE+0x38)		

#define PIOB_PDSR	(PIOB_BASE+0x3C)		

#define PIOB_IER	(PIOB_BASE+0x40)		
#define PIOB_IDR	(PIOB_BASE+0x44)		
#define PIOB_IMR	(PIOB_BASE+0x48)		
#define PIOB_ISR	(PIOB_BASE+0x4C)		

/* PIOB bit allocation */
#define PIOB_TCLK1	(1)					
#define PIOB_TIOA1	(1<<1)				
#define PIOB_TIOB1	(1<<2)				
#define PIOB_NCTSA	(1<<3)				

#define PIOB_NRIA	(1<<5)				
#define PIOB_NWDOVF	(1<<6)				
#define PIOB_NCE1	(1<<7)				
#define PIOB_NCE2	(1<<8)				

#ifndef __ASSEMBLER__
struct pio_regs{
	u_32 per;
	u_32 pdr;
	u_32 psr;
	u_32 res1;
	u_32 oer;
	u_32 odr;
	u_32 osr;
	u_32 res2;
	u_32 ifer;
	u_32 ifdr;
	u_32 ifsr;
	u_32 res3;
	u_32 sodr;
	u_32 codr;
	u_32 odsr;
	u_32 pdsr;
	u_32 ier;
	u_32 idr;
	u_32 imr;
	u_32 isr;
};
#endif

/*
	Serial Peripheral Interface
*/
#define	SP_CR		(SPI_BASE + 0)
#define	SP_MR		(SPI_BASE + 4)
#define	SP_RDR		(SPI_BASE + 8)
#define	SP_TDR		(SPI_BASE + 0xC)
#define	SP_SR		(SPI_BASE + 0x10)
#define	SP_IER		(SPI_BASE + 0x14)
#define	SP_IDR		(SPI_BASE + 0x18)
#define	SP_IMR		(SPI_BASE + 0x1C)
#define	SP_CSR0		(SPI_BASE + 0x30)
#define	SP_CSR1		(SPI_BASE + 0x34)
#define	SP_CSR2		(SPI_BASE + 0x38)
#define	SP_CSR3		(SPI_BASE + 0x3C)

#ifndef __ASSEMBLER__
struct spi_regs{
	u_32 cr;
	u_32 mr;
	u_32 rdr;
	u_32 tdr;
	u_32 sr;
	u_32 ier;
	u_32 idr;
	u_32 imr;
	u_32 res1;
	u_32 res2;
	u_32 res3;
	u_32 res4;
	u_32 csr0;
	u_32 csr1;
	u_32 csr2;
	u_32 csr3;
};
#endif

/* SPI control register */
#define SPI_SWRST		(1<<7)
#define SPI_SPIDIS		(1<<1)
#define SPI_SPIEN		(1)

/* SPI mode register */
#define SPI_MSTR		(1)
#define SPI_PS			(1<<1)
#define SPI_PCSDEC		(1<<2)
#define SPI_MCSK32		(1<<3)
#define SPI_LLB			(1<<7)
#define SPI_PCS(x)		(x<<16 & 0xF0000)
#define SPI_DLYBCS(x)	(x<<24 & 0xFF000000)

/* SPI Receive/Transmit Data Register */
#define SPI_PCS_MASK	(0xF0000)

/* SPI Interrupt enable/disable and Status registers */
#define SPI_OVRES		(1<<3)
#define SPI_MODF		(1<<2)
#define SPI_TDRE		(1<<1)
#define SPI_RDRF		(1)

/* SPI chip selects registers */
#define SPI_CPOL		(1)
#define SPI_NCPHA		(1<<1)

#define SPI_BITS_MASK	0xF0
#define SPI_SCBR_MASK	0xF00
#define SPI_DLYBS_MASK	0xF0000
#define SPI_DLYBCT_MASK	0xF000000

#define SPI_BITS(x)		(x<<4 & SPI_BITS_MASK)
#define SPI_SCBR(x)		(x<<8 & SPI_SCBR_MASK)
#define SPI_DLYBS(x)	(x<<16 & SPI_DLYBS_MASK)
#define SPI_DLYBCT(x)	(x<<24 & SPI_DLYBCT_MASK)

/*
	Watchdog registers
*/
#define WDG_OMR		(WD_BASE + 0)
#define WDG_CMR		(WD_BASE + 4)
#define WDG_CR		(WD_BASE + 8)
#define WDG_SR		(WD_BASE + 0xC)

/* Overflow Mode Register */
#define WDG_OKEY_MASK	0xFFF0
#define WDG_OKEY(x)		(x<<4 & WDG_OKEY_MASK)
#define WDG_EXTEN		(1<<3)
#define WDG_IRQEN		(1<<2)
#define WDG_RSTEN		(1<<1)
#define WDG_WDEN		(1)

/* Clock Mode Register */
#define WDG_CKEY_MASK	0xFF80
#define WDG_HPCV_MASK	0x3C
#define WDG_WDCLKS_MASK	0x3
#define WDG_CKEY(x)		(x<<7 & WDG_CKEY_MASK)
#define WDG_HPCV(x)		(x<<2 & WDG_HPCV_MASK)
#define WDG_WDCLKS(x)	(x & WDG_WDCLKS_MASK)

/* Control Register */
#define	WDG_RESTART_KEY		0xC071

/* Status Register */
#define WDG_WDOVF		(1)


#endif

