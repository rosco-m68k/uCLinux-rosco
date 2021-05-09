/*
 * uclinux/include/asm-armnommu/arch-atmel/irqs.h
 *
 */
#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#ifdef CONFIG_ARCH_ATMEL_EB55

#define IRQ_FIQ	    0
#define IRQ_SW	    1
#define IRQ_USART0  2
#define IRQ_USART1  3
#define IRQ_USART2  4
#define IRQ_SPI     5
#define IRQ_TC0	    6
#define IRQ_TC1	    7
#define IRQ_TC2	    8
#define IRQ_TC3	    8
#define IRQ_TC4	    10
#define IRQ_TC5	    11
#define IRQ_WD	    12
#define IRQ_PIOA    13
#define IRQ_PIOB    14
#define IRQ_AD0	    15
#define IRQ_AD1     16
#define IRQ_DA0	    17
#define IRQ_DA1     18
#define IRQ_RTC     19
#define IRQ_APMC    20

#define IRQ_IRQ6    23    
#define IRQ_IRQ5    24
#define IRQ_IRQ4    25
#define IRQ_IRQ3    26
#define IRQ_IRQ2    27
#define IRQ_IRQ1    28
#define IRQ_IRQ0    29

#else

#define IRQ_FIQ	    0
#define IRQ_SW	    1
#define IRQ_USART0  2
#define IRQ_USART1  3
#define IRQ_TC0	    4
#define IRQ_TC1	    5
#define IRQ_TC2	    6
#define IRQ_WD	    7
#define IRQ_PIO	    8

#define IRQ_IRQ0    16
#define IRQ_IRQ1    17
#define IRQ_IRQ2    18

#endif

/* Machine specific interrupt sources.
 *
 * Adding an interrupt service routine for a source with this bit
 * set indicates a special machine specific interrupt source.
 * The machine specific files define these sources.  
 */
#define IRQ_MACHSPEC     (0x10000000L)
#define IRQ_IDX(irq)    ((irq) & ~IRQ_MACHSPEC)

/* various flags for request_irq() */
#define IRQ_FLG_LOCK    (0x0001)        /* handler is not replaceable   */
#define IRQ_FLG_REPLACE (0x0002)        /* replace existing handler     */
#define IRQ_FLG_FAST    (0x0004)
#define IRQ_FLG_SLOW    (0x0008)
#define IRQ_FLG_STD     (0x8000)        /* internally used              */


#endif
