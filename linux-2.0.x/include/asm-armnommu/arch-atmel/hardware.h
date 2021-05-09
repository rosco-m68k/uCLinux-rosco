/*
 * uclinux/include/asm-armnommu/arch-atmel/hardware.h
 *
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/config.h>

#ifndef __ASSEMBLER__

typedef unsigned long u_32;

typedef volatile unsigned long at91_reg ;

/* ARM asynchronous clock */
#define ARM_CLK ((u_32)(CONFIG_ARM_CLK))	/*(24000000))*/
#else
#define ARM_CLK CONFIG_ARM_CLK
#endif

#ifndef __ASSEMBLER__
/*
 * RAM definitions
 */
#define MAPTOPHYS(a)      ((unsigned long)a)
#define KERNTOPHYS(a)     ((unsigned long)(&a))
#define GET_MEMORY_END(p) ((p->u1.s.page_size) * (p->u1.s.nr_pages))

#define PARAMS_BASE       0x7000

#define HARD_RESET_NOW()  { arch_hard_reset(); }

#endif

#define IO_BASE  0

/* EBI -- the External Bus Interface */
#define EBI_BASE 0xFFE00000

#define EBI_CSR0 (EBI_BASE + 0x00) /* Chip Select Register 0 */
#define EBI_CSR1 (EBI_BASE + 0x04) /* Chip Select Register 1 */
#define EBI_RCR  (EBI_BASE + 0x20) /* Remap Control Register */
#define EBI_MCR  (EBI_BASE + 0x24) /* Memory Control Register */

/* USART0/1 */

#ifdef CONFIG_ARCH_ATMEL_EB55
#define USART0_BASE 0XFFFC0000
#define USART1_BASE 0XFFFC4000
#else
/* USART0_BASE and USART1_BASE are set in .config
 *
#define USART0_BASE 0xFFFD0000
#define USART1_BASE 0xFFFCC000
*/
#endif

#ifndef __ASSEMBLER__
/*
	Atmel USART registers
*/
struct atmel_usart_regs{
	at91_reg cr;		// control 
	at91_reg mr;		// mode
	at91_reg ier;		// interrupt enable
	at91_reg idr;		// interrupt disable
	at91_reg imr;		// interrupt mask
	at91_reg csr;		// channel status
	at91_reg rhr;		// receive holding 
	at91_reg thr;		// tramsmit holding		
	at91_reg brgr;		// baud rate generator		
	at91_reg rtor;		// rx time-out
	at91_reg ttgr;		// tx time-guard
	at91_reg res1;
	at91_reg rpr;		// rx pointer
	at91_reg rcr;		// rx counter
	at91_reg tpr;		// tx pointer
	at91_reg tcr;		// tx counter
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

/* PIO -- Parallel IO controller */

#ifndef CONFIG_ARCH_ATMEL_EB55

#define PIO_BASE                       0xffff0000

#define PIO_ENABLE_REGISTER            (PIO_BASE+0x00)
#define PIO_DISABLE_REGISTER           (PIO_BASE+0x04)
#define PIO_OUTPUT_ENABLE_REGISTER     (PIO_BASE+0x10)
#define PIO_SET_OUTPUT_DATA_REGISTER   (PIO_BASE+0x30)
#define PIO_CLEAR_OUTPUT_DATA_REGISTER (PIO_BASE+0x34)

#define LED_OUTPUT_PIN                 (PIO_BASE+0x01)  /* LED is attached to PIO pin 1 */
#endif

#ifdef CONFIG_ARCH_ATMEL_EB55

#ifndef __ASSEMBLER__
typedef struct
{
    at91_reg        PIO_PER ;           /* PIO Enable Register */
    at91_reg        PIO_PDR ;           /* PIO Disable Register */
    at91_reg        PIO_PSR ;           /* PIO Status Register */
    at91_reg        Reserved0 ;
    at91_reg        PIO_OER ;           /* Output Enable Register */
    at91_reg        PIO_ODR ;           /* Output Disable Register */
    at91_reg        PIO_OSR ;           /* Output Status Register */
    at91_reg        Reserved1 ;
    at91_reg        PIO_IFER ;          /* Input Filter Enable Register */
    at91_reg        PIO_IFDR ;          /* Input Filter Disable Register */
    at91_reg        PIO_IFSR ;          /* Input Filter Status Register */
    at91_reg        Reserved2 ;
    at91_reg        PIO_SODR ;          /* Set Output Data Register */
    at91_reg        PIO_CODR ;          /* Clear Output Data Register */
    at91_reg        PIO_ODSR ;          /* Output Data Status Register */
    at91_reg        PIO_PDSR ;          /* Pin Data Status Register */
    at91_reg        PIO_IER ;           /* Interrupt Enable Register */
    at91_reg        PIO_IDR ;           /* Interrupt Disable Register */
    at91_reg        PIO_IMR ;           /* Interrupt Mask Register */
    at91_reg        PIO_ISR ;           /* Interrupt Status Register */
    at91_reg        PIO_MDER ;          /* Multi Driver Enable Register */
    at91_reg        PIO_MDDR ;          /* Multi Driver Disable Register */
    at91_reg        PIO_MDSR ;          /* Multi Driver Status Register */
} StructPIO ;

#define PIOA_BASE                       (( StructPIO *) 0xfffec000)
#define PIOB_BASE                       (( StructPIO *) 0xffff0000)

#else

#define PIOA_BASE                       (0xfffec000)
#define PIOB_BASE                       (0xffff0000)

#endif

#define PIOB_ENABLE_REGISTER            (PIOB_BASE+0x00)
#define PIOB_DISABLE_REGISTER           (PIOB_BASE+0x04)
#define PIOB_OUTPUT_ENABLE_REGISTER     (PIOB_BASE+0x10)
#define PIOB_SET_OUTPUT_DATA_REGISTER   (PIOB_BASE+0x30)
#define PIOB_CLEAR_OUTPUT_DATA_REGISTER (PIOB_BASE+0x34)

#define PIOB_ENABLE_REGISTER            (PIOB_BASE+0x00)
#define PIOB_DISABLE_REGISTER           (PIOB_BASE+0x04)
#define PIOB_OUTPUT_ENABLE_REGISTER     (PIOB_BASE+0x10)
#define PIOB_SET_OUTPUT_DATA_REGISTER   (PIOB_BASE+0x30)
#define PIOB_CLEAR_OUTPUT_DATA_REGISTER (PIOB_BASE+0x34)

#define PIOTXD0       15            /* USART 0 transmit data */
#define PIORXD0       16            /* USART 0 receive data  */

#endif

/* SF -- the special function stuff */
#define SF_BASE 0xFFF00000

#define SF_CHIP_ID			(SF_BASE+0x00)
#define SF_CHIP_ID_EXT			(SF_BASE+0x00)
#define SF_CHIP_ID_EXT			(SF_BASE+0x00)
#define SF_PROTEXT_MODE			(SF_BASE+0x00)

/* Timer */
#ifdef CONFIG_ARCH_ATMEL_EB55
#define TIMER_BASE 0xFFFD0000
#else
#define TIMER_BASE 0xFFFE0000
#endif

/* The Atmel CPUs have 3 internel timers, TC0, TC1, and TC2.
 * One of these muct be used to drive the kernel's internal
 * timer (the thing that updates jiffies).  Pick a timer channel
 * here.  */
#define	KERNEL_TIMER	1

#define CH0_OFFSET		0x00
#define CH1_OFFSET		0x40
#define CH2_OFFSET		0x80
#define BLOCK_CONTROL_OFFSET	0xC0
#define BLOCK_MODE_OFFSET	0xC4

#define TIMER_CONTROL_CH0		(TIMER_BASE+CH0_OFFSET)
#define TIMER_CONTROL_CH1		(TIMER_BASE+CH1_OFFSET)
#define TIMER_CONTROL_CH2		(TIMER_BASE+CH2_OFFSET)
#define TIMER_CONTROL_BLOCK_CONTROL	(TIMER_BASE+BLOCK_CONTROL_OFFSET)
#define TIMER_CONTROL_BLOCK_MODE	(TIMER_BASE+BLOCK_MODE_OFFSET)

/*  Timer control registers */
#define TC_CCR(x)   (x + 0x00)
#define TC_CMR(x)   (x + 0x04)
#define TC_CV(x)    (x + 0x10)
#define TC_RA(x)    (x + 0x14)
#define TC_RB(x)    (x + 0x18)
#define TC_RC(x)    (x + 0x1C)
#define TC_SR(x)    (x + 0x20)
#define TC_IER(x)   (x + 0x24)
#define TC_IDR(x)   (x + 0x28)
#define TC_IMR(x)   (x + 0x2C)

/*  TC mode register */
#define TC2XC2S(x) (x & 0x3)
#define TC1XC1S(x) (x<<2 & 0xc)
#define TC0XC0S(x) (x<<4 & 0x30)

/* TC channel control */
#define TC_CLKEN  (1)      
#define TC_CLKDIS (1<<1)      
#define TC_SWTRG  (1<<2)      

/* TC interrupts enable/disable/mask and status registers */
#define TC_MTIOB  (1<<18)
#define TC_MTIOA  (1<<17)
#define TC_CLKSTA (1<<16)

#define TC_ETRGS  (1<<7)
#define TC_LDRBS  (1<<6)
#define TC_LDRAS  (1<<5)
#define TC_CPCS   (1<<4)
#define TC_CPBS   (1<<3)
#define TC_CPAS   (1<<2)
#define TC_LOVRS  (1<<1)
#define TC_COVFS  (1)

#ifndef __ASSEMBLER__
struct atmel_timer_channel
{
  at91_reg ccr;        // channel control register   (WO)
  at91_reg cmr;        // channel mode register      (RW)
  at91_reg reserved[2];    
  at91_reg cv;         // counter value              (RW)
  at91_reg ra;         // register A                 (RW)
  at91_reg rb;         // register B                 (RW)
  at91_reg rc;         // register C                 (RW)
  at91_reg sr;         // status register            (RO)
  at91_reg ier;        // interrupt enable register  (WO)
  at91_reg idr;        // interrupt disable register (WO)
  at91_reg imr;        // interrupt mask register    (RO)
  at91_reg reserved1[4];    
};

struct atmel_timers
{
  struct {
    struct atmel_timer_channel ch;
  } chans[3];
  at91_reg bcr;        // block control register   (WO)
  at91_reg bmr;        // block mode   register    (RW)
};

#endif

/* Advanced Interrupt Controller */
#define AIC_BASE 0xFFFFF000

#define AIC_SMR(i) (AIC_BASE+i*4)      /* Source Mode Register */
#define AIC_IVR    (AIC_BASE+0x100)    /* IRQ Vector Register */
#define AIC_FVR    (AIC_BASE+0x104)    /* FIQ Vector Register */
#define AIC_ISR    (AIC_BASE+0x108)    /* Interrupt Status Register */
#define AIC_IPR    (AIC_BASE+0x10C)    /* Interrupt Pending Register */
#define AIC_IMR    (AIC_BASE+0x110)    /* Interrupt Mask Register */
#define AIC_CISR   (AIC_BASE+0x114)    /* Core Interrupt Status Register */
#define AIC_IECR   (AIC_BASE+0x120)    /* Interrupt Enable Command Register */
#define AIC_IDCR   (AIC_BASE+0x124)    /* Interrupt Disable Command Register */
#define AIC_ICCR   (AIC_BASE+0x128)    /* Interrupt Clear Command Register */
#define AIC_ISCR   (AIC_BASE+0x12C)    /* Interrupt Set Command Register */
#define AIC_EOICR  (AIC_BASE+0x130)    /* End of Interrupt Command Register */
#define AIC_SPU    (AIC_BASE+0x134)    /* Spurious Vector Register */

#define AIC_SVR(i) (AIC_BASE+0x80+i*4) /* Source Vector Register */

#ifdef CONFIG_ARCH_ATMEL_EB55

#define AIC_FIQ  (1<<0)
#define AIC_SW   (1<<1)
#define AIC_URT0 (1<<2)
#define AIC_URT1 (1<<3)
#define AIC_URT2 (1<<4)
#define AIC_SPI  (1<<5)
#define AIC_TC0  (1<<6)
#define AIC_TC1  (1<<7)
#define AIC_TC2  (1<<8)
#define AIC_TC3  (1<<9)
#define AIC_TC4  (1<<10)
#define AIC_TC5  (1<<11)
#define AIC_WD   (1<<12)
#define AIC_PIOA (1<<13)
#define AIC_PIOB (1<<14)
#define AIC_AD0  (1<<15)
#define AIC_AD1  (1<<16)
#define AIC_DA0  (1<<17)
#define AIC_DA1  (1<<18)
#define AIC_RTC  (1<<19)
#define AIC_APMC (1<<20)

#define AIC_IRQ6 (1<<23)
#define AIC_IRQ5 (1<<24)
#define AIC_IRQ4 (1<<25)
#define AIC_IRQ3 (1<<26)
#define AIC_IRQ2 (1<<27)
#define AIC_IRQ1 (1<<28)
#define AIC_IRQ0 (1<<29)

#else

#define AIC_FIQ  (1<<0)
#define AIC_SW   (1<<1)
#define AIC_URT0 (1<<2)
#define AIC_URT1 (1<<3)
#define AIC_TC0  (1<<4)
#define AIC_TC1  (1<<5)
#define AIC_TC2  (1<<6)
#define AIC_WD   (1<<7)
#define AIC_PIO  (1<<8)
#define AIC_IRQ0 (1<<16)
#define AIC_IRQ1 (1<<17)
#define AIC_IRQ2 (1<<18)

#endif

#ifdef CONFIG_ARCH_ATMEL_EB55

#ifndef __ASSEMBLER__
typedef struct
{
    at91_reg    APMC_SCER ;             /* System Clock Enable  Register */
    at91_reg    APMC_SCDR ;             /* System Clock Disable Register */
    at91_reg    APMC_SCSR ;             /* System Clock Status  Register */
    at91_reg    Reserved0 ;
    at91_reg    APMC_PCER ;             /* Peripheral Clock Enable  Register */
    at91_reg    APMC_PCDR ;             /* Peripheral Clock Disable Register */
    at91_reg    APMC_PCSR ;             /* Peripheral Clock Status  Register */
        at91_reg    Reserved1 ;
        at91_reg    APMC_CGMR ;                 /* Clock Generator Mode Register */
        at91_reg    Reserved2 ;
        at91_reg    APMC_PCR ;                  /* Power Control Register */
        at91_reg    APMC_PMR ;                  /* Power Mode Register */
        at91_reg    APMC_SR ;                   /* Status Register */
        at91_reg    APMC_IER ;                  /* Interrupt Enable Register */
        at91_reg    APMC_IDR ;                  /* Interrupt Disable Register */
        at91_reg    APMC_IMR ;                  /* Interrupt Mask Register */   
} StructAPMC ;

#define APMC_BASE        (( StructAPMC *) 0xFFFF4000)

#endif

#define FIQ_ID      0       /* Fast Interrupt */

#define SW_ID       1       /* Soft Interrupt (generated by the AIC) */

#define US0_ID      2       /* USART Channel 0 */
#define US1_ID      3       /* USART Channel 1 */
#define US2_ID      4       /* USART Channel 2 */

#define SPI_ID      5       /* SPI Channel */

#define TC0_ID      6       /* Timer Channel 0 */
#define TC1_ID      7       /* Timer Channel 1 */
#define TC2_ID      8       /* Timer Channel 2 */
#define TC3_ID      9       /* Timer Channel 3 */
#define TC4_ID      10      /* Timer Channel 4 */
#define TC5_ID      11      /* Timer Channel 5 */

#define WD_ID       12      /* Watchdog interrupt */

#define PIOA_ID     13      /* Parallel I/O Controller A interrupt */
#define PIOB_ID     14      /* Parallel I/O Controller B interrupt */

#define AD0_ID      15      /* Analog to Digital Converter Channel 0 interrupt */
#define AD1_ID      16      /* Analog to Digital Converter Channel 1 interrupt */

#define DAC0_ID     17      /* Digital to Analog Converter Channel 0 interrupt */
#define DAC1_ID     18      /* Digital to Analog Converter Channel 1 interrupt */

#define RTC_ID      19      /* Real Time Clock interrupt */

#define APMC_ID     20      /* Advanced Power Management Controller interrupt */

#define IRQ6_ID     23      /* External interrupt 6 */
#define IRQ5_ID     24      /* External interrupt 5 */
#define IRQ4_ID     25      /* External interrupt 4 */
#define IRQ3_ID     26      /* External interrupt 3 */
#define IRQ2_ID     27      /* External interrupt 2 */
#define IRQ1_ID     28      /* External interrupt 1 */
#define IRQ0_ID     29      /* External interrupt 0 */

#define COMMRX_ID   30      /* RX Debug Communication Channel interrupt */
#define COMMTX_ID   31      /* TX Debug Communication Channel interrupt */

#endif


#endif

