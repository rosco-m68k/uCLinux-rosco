/*---------------------------------------------------------------------------*/
/* mc68328digi.c,v 1.5 2001/07/19 13:58:58 pney Exp
 *
 * linux/drivers/char/mc68328digi.c - Touch screen driver.
 *
 * Author: Philippe Ney <philippe.ney@smartdata.ch>
 * Copyright (C) 2001 SMARTDATA <www.smartdata.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Thanks to:
 *    Kenneth Albanowski for is first work on a touch screen driver.
 *    Alessandro Rubini for is "Linux device drivers" book.
 *    Ori Pomerantz for is "Linux Kernel Module Programming" guide.
 *
 * Updates:
 *   2001-03-07 Pascal bauermeister <pascal.bauermeister@smartdata.ch>
 *              - Adapted to work ok on xcopilot; yet untested on real Palm.
 *              - Added check for version in ioctl()
 *
 *   2001-03-21 Pascal bauermeister <pascal.bauermeister@smartdata.ch>
 *              - bypass real hw uninits for xcopilot. Now xcopilot is
 *                happy. It even no longer generates pen irq while this
 *                irq is disabled (I'd like to understand why, so that
 *                I can do the same in mc68328digi_init() !).
 *
 *
 * Hardware:      Motorola MC68EZ328 DragonBall-EZ
 *                Burr-Brown ADS7843 Touch Screen Controller
 *                Rikei REW-A1 Touch Screen
 *
 * OS:            uClinux version 2.0.38.1pre3
 *
 * Connectivity:  Burr-Brown        DragonBall
 *                PENIRQ     ---->  PF1  &  PD4 (with a 100k resistor)
 *                BUSY       ---->  PF5
 *                CS         ---->  PB4
 *                DIN        ---->  PE0
 *                DOUT       ---->  PE1
 *                DCLK       ---->  PE2
 *
 *     ADS7843: /PENIRQ ----+----[100k resistor]---- EZ328: PD4
 *                          |
 *                          +----------------------- EZ328: /IRQ5
 *
 * States graph:
 *
 *         ELAPSED & PEN_UP
 *         (re-init)             +------+
 *     +------------------------>| IDLE |<------+
 *     |                         +------+       |
 *     |                           |            | ELAPSED
 *     |  +------------------------+            | (re-init)
 *     |  |       PEN_IRQ                       |
 *     |  |       (disable pen_irq)             | 
 *     |  |       (enable & set timer)          | 
 *     |  |                                     |
 *     | \/                                     |
 *   +------+                                   |
 *   | WAIT |                                   |
 *   +------+                                   |
 *   /\   |                                     | 
 *   |    |             +---------------+-------+------+---------------+
 *   |    |             |               |              |               |
 *   |    |             |               |              |               |
 *   |    |        +--------+      +-------+      +--------+      +--------+
 *   |    +------->| ASK X  |      | ASK Y |      | READ X |      | READ Y |
 *   |  ELAPSED &  +--------+      +-------+      +--------+      +--------+
 *   |  PEN_DOWN       |           /\    |         /\    |         /\    |
 *   |  (init SPIM)    +-----------'     +---------'     +---------'     |
 *   |  (set timer)    XCH_COMPLETE      XCH_COMPLETE    XCH_COMPLETE    |
 *   |                                                                   |
 *   |                                                                   |
 *   |                                                                   |
 *   +-------------------------------------------------------------------+
 *                               XCH_COMPLETE
 *                               (release SPIM)
 *                               (set timer)
 *
 *
 * Remarks: I added some stuff for port on 2.2 and 2.4 kernels but currently
 *          this version works only on 2.0 because not tested on 2.4 yet.
 *          Someone interested?
 *
 */
/*---------------------------------------------------------------------------*/

#include <linux/kernel.h>     /* We're doing kernel work */
#include <linux/module.h>     /* Specifically, a module */
#include <linux/interrupt.h>  /* We want interrupts */

#include <linux/miscdevice.h> /* for misc_register() and misc_deregister() */

#include <linux/fs.h>         /* for struct 'file_operations' */

#include <linux/timer.h>     /* for timeout interrupts */
#include <linux/param.h>     /* for HZ. HZ = 100 and the timer step is 1/100 */
#include <linux/sched.h>     /* for jiffies definition. jiffies is incremented
                              * once for each clock tick; thus it's incremented
			      * HZ times per secondes.*/
#include <linux/mm.h>        /* for verify_area */
#include <linux/malloc.h>    /* for kmalloc */

#include <asm/irq.h>         /* For IRQ_MACHSPEC */

/*---------------------------------------------------------------------------*/

#ifdef CONFIG_M68328
/* These are missing in MC68328.h. I took them from MC68EZ328.h temporarily,
 * but did not dare modifying MC68328.h yet (don't want to break things and
 * have no time now to do regression testings)
 *   - Pascal Bauermeister
 */
#define ICR_ADDR        0xfffff302
#define ICR_POL5        0x0080  /* Polarity Control for IRQ5 */
#define IMR_MIRQ5       (1 << IRQ5_IRQ_NUM)     /* Mask IRQ5 */
#define IRQ5_IRQ_NUM    20      /* IRQ5 */
#define PBPUEN          BYTE_REF(PBPUEN_ADDR)
#define PBPUEN_ADDR     0xfffff40a              /* Port B Pull-Up enable reg */
#define PB_CSD0_CAS0    0x10    /* Use CSD0/CAS0 as PB[4] */
#define PB_CSD0_CAS0    0x10    /* Use CSD0/CAS0 as PB[4] */
#define PDSEL           BYTE_REF(PDSEL_ADDR)
#define PDSEL_ADDR      0xfffff41b              /* Port D Select Register */
#define PD_IRQ1         0x10    /* Use IRQ1 as PD[4] */
#define PF_A22          0x20    /* Use A22  as PF[5] */
#define PF_IRQ5         0x02    /* Use IRQ5 as PF[1] */
#define SPIMCONT        WORD_REF(SPIMCONT_ADDR)
#define SPIMCONT_ADDR   0xfffff802
#define SPIMCONT_SPIMEN         SPIMCONT_RSPIMEN

#elif defined(CONFIG_M68EZ328)
# include <asm/MC68EZ328.h> /* bus, port and irq definition of DragonBall EZ */

#elif defined(CONFIG_M68VZ328)
# include <asm/MC68VZ328.h> /* bus, port and irq definition of DragonBall VZ */
#endif

#include <linux/mc68328digi.h>

/*---------------------------------------------------------------------------*/
/*
 * Kernel compatibility.
 * Kernel > 2.2.3, include/linux/version.h contain a macro for KERNEL_VERSION
 */
#include <linux/version.h>
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c)  (((a) << 16) + ((b) << 8) + (c))
#endif

/*
 * Conditional compilation. LINUX_VERSION_CODE is the code of this version
 * (as per KERNEL_VERSION)
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
#include <asm/uaccess.h>    /* for put_user     */
#include <linux/poll.h>     /* for polling fnct */
#endif

/*---------------------------------------------------------------------------*/
/* Backward compatibility ---------------------------------------------------*/

//#define CONFIG_M68EZ328_CHIPSLICE    /* uncomment for backward comp. */

/*---------------------------------------------------------------------------*/
/* Debug --------------------------------------------------------------------*/

#define INIT_DEBUG       0
#define EVENT_DEBUG      0
#define POSITION_DEBUG   0
#define CORRUPT_DEBUG    0
#define SEQU_DEBUG       0

/*---------------------------------------------------------------------------*/
/* definitions --------------------------------------------------------------*/
/*
 * time limit 
 * Used for a whole conversion (x & y) and to clock the Burr-Brown to fall the
 * BUSY signal down (otherwise, pen cannot generate irq anymore).
 * If this limit run out, an error is generated. In the first case, the driver
 * could recover his normal state. In the second, the BUSY signal cannot be
 * fallen down and the device have to be reseted.
 * This limit is defined as approximatively the double of the deglitch one.
 */
#define CONV_TIME_LIMIT    50      /* ms */

/* 
 * Length of the data structure
 */
#define DATA_LENGTH       sizeof(struct ts_pen_info)

/*
 * Size of the buffer for the event queue
 */
#define TS_BUF_SIZE        32*DATA_LENGTH

/*
 * Minor number for the touch screen device. Major is 10 because of the misc 
 * type of the driver.
 */
#define MC68328DIGI_MINOR    9

/*
 * SPIMCONT fields for this app (not defined in asm/MC68EZ328.h).
 */
#define SPIMCONT_DATARATE ( 2<<SPIMCONT_DATA_RATE_SHIFT)/* SPICLK = CLK / 16 */
#define SPIMCONT_BITCOUNT (15<< 0)                      /* 16 bits transfert */

/*
 * SPIMCONT used values.
 */
#define SPIMCONT_INIT (SPIMCONT_DATARATE    | \
                       SPIMCONT_ENABLE      | \
                       SPIMCONT_XCH      *0 | \
                       SPIMCONT_IRQ      *0 | \
                       SPIMCONT_IRQEN       | \
                       SPIMCONT_PHA         | \
                       SPIMCONT_POL         | \
                       SPIMCONT_BITCOUNT      )

/*
 * ADS7843 fields (BURR-BROWN Touch Screen Controller).
 */
#define ADS7843_START_BIT (1<<7)
#define ADS7843_A2        (1<<6)
#define ADS7843_A1        (1<<5)
#define ADS7843_A0        (1<<4)
#define ADS7843_MODE      (1<<3)  /* HIGH = 8, LOW = 12 bits conversion */
#define ADS7843_SER_DFR   (1<<2)  /* LOW = differential mode            */
#define ADS7843_PD1       (1<<1)  /* PD1,PD0 = 11  PENIRQ disable       */
#define ADS7843_PD0       (1<<0)  /* PD1,PD0 = 00  PENIRQ enable        */

/*
 * SPIMDATA used values.
 */
/*
 * Ask for X conversion. Disable PENIRQ.
 */
#define SPIMDATA_ASKX   (ADS7843_START_BIT   | \
                         ADS7843_A2          | \
			 ADS7843_A1        *0| \
			 ADS7843_A0          | \
                         ADS7843_MODE      *0| \
                         ADS7843_SER_DFR   *0| \
                         ADS7843_PD1         | \
                         ADS7843_PD0            )
/*
 * Ask for Y conversion. Disable PENIRQ.
 */
#define SPIMDATA_ASKY   (ADS7843_START_BIT   | \
                         ADS7843_A2        *0| \
 			 ADS7843_A1        *0| \
	 	 	 ADS7843_A0          | \
                         ADS7843_MODE      *0| \
                         ADS7843_SER_DFR   *0| \
                         ADS7843_PD1         | \
                         ADS7843_PD0            )
/*
 * Ask for conversion on auxiliary input IN3.
 * Single-ended reference mode. Disable PENIRQ.
 */
#define SPIMDATA_ASKAUX_IN3   (ADS7843_START_BIT   | \
                               ADS7843_A2        *0| \
                               ADS7843_A1          | \
                               ADS7843_A0        *0| \
                               ADS7843_MODE      *0| \
                               ADS7843_SER_DFR     | \
                               ADS7843_PD1         | \
                               ADS7843_PD0            )
/*
 * Ask for conversion on auxiliary input IN4.
 * Single-ended reference mode. Disable PENIRQ.
 */
#define SPIMDATA_ASKAUX_IN4   (ADS7843_START_BIT   | \
                               ADS7843_A2          | \
                               ADS7843_A1          | \
                               ADS7843_A0        *0| \
                               ADS7843_MODE      *0| \
                               ADS7843_SER_DFR     | \
                               ADS7843_PD1         | \
                               ADS7843_PD0            )
/*
 * Enable PENIRQ.
 */
#define SPIMDATA_NOP    (ADS7843_START_BIT   | \
                         ADS7843_A2        *0| \
 			 ADS7843_A1        *0| \
	 	 	 ADS7843_A0        *0| \
                         ADS7843_MODE      *0| \
                         ADS7843_SER_DFR   *0| \
                         ADS7843_PD1       *0| \
                         ADS7843_PD0       *0   )
/*
 * Generate clock to pull down BUSY signal.
 * No start bit to avoid generating a BUSY at end of the transfert.
 */
#define SPIMDATA_NULL    0

/*
 * Touch screen driver states.
 */
#define TS_DRV_ERROR       -1
#define TS_DRV_IDLE         0
#define TS_DRV_WAIT         1
#define TS_DRV_ASKX         2
#define TS_DRV_ASKY         3
#define TS_DRV_READX        4
#define TS_DRV_READY        5

/*
 * Conversion status.
 */
#define CONV_ERROR      -1   /* error during conversion flow        */    
#define CONV_END         0   /* conversion ended (= pen is up)      */
#define CONV_LOOP        1   /* conversion continue (= pen is down) */


/* HW specifics -------------------------------------------------------------*/
/*
 * Config ChipSlice for DB VZ
 */
#ifdef CONFIG_M68VZ328_CHIPSLICE
/*
 * 1- Ports config for VZ_CHIPSLICE
 */
/* Burr-Brown Chip Select connected to port M2 */
# define CS_DIR   PMDIR
# define CS_DATA  PMDATA
# define CS_PUEN  PMPUEN
# define CS_SEL   PMSEL
/* Burr-Brown AD Busy connected to port D0 */
# define BUSY_DIR   PDDIR
# define BUSY_DATA  PDDATA
# define BUSY_PUEN  PDPUEN
# define BUSY_SEL   PDSEL
# define BUSY_POL   PDPOL
# define BUSY_IRQEN PDIRQEN
# define BUSY_KBEN  PDKBEN
# define BUSY_IRQEG PDIRQEG
/*
 * 2- Useful mask for VZ_CHIPSLICE
 */
# define PEN_MASK    PF_IRQ5  /* pen irq signal from the Burr-Brown is
                                 connected to IRQ5 of DragonBall */
/* The 3 LSB of port E are multiplexed with the SPIM       */
# define PESEL_MASK  0x07
/* Due to Burr-Brown connection */
# define BUSY_MASK   PD_INT0
# define CS_MASK     PM_DQMH
/* Because the conversion is done with 12 bit accuracy */
# define CONV_MASK   0x0FFF

/*
 * Config ChipSlice for DB EZ = old config
 * This config is valid for Copilot too
 */
#elif defined(CONFIG_M68EZ328_CHIPSLICE) || \
      defined(CONFIG_PILOT)
/*
 * 1- Ports config for EZ_CHIPSLICE
 */
/* Burr-Brown Chip Select connected to port B4 */
# define CS_DIR   PBDIR
# define CS_DATA  PBDATA
# define CS_PUEN  PBPUEN
# define CS_SEL   PBSEL
/* Burr-Brown AD Busy connected to port F5 */
# define BUSY_DIR   PFDIR
# define BUSY_DATA  PFDATA
# define BUSY_PUEN  PFPUEN
# define BUSY_SEL   PFSEL
/*
 * 2- Useful mask for EZ_CHIPSLICE
 */
/* pen irq signal from the Burr-Brown is connected to IRQ5 (port F) and
 * to bit 4 port D through a 100k resistor */
# define PEN_MASK    PF_IRQ5
# define PEN_MASK_2  PD_IRQ1
/* The 3 LSB of port E are multiplexed with the SPIM       */
# define PESEL_MASK  0x07
/* Due to Burr-Brown connection */
# define BUSY_MASK   PF_A22
# define CS_MASK     PB_CSD0_CAS0
/* Because the conversion is done with 12 bit accuracy */
# define CONV_MASK   0x0FFF
/*
 * No config defined.
 * You have to define your own config or set backward compatibility by
 * uncommenting the related define at the top of this file.
 */
#else
# error -->> You have to define your hw configuration (definitions) <<--
#endif

/*---------------------------------------------------------------------------*/
/* macros -------------------------------------------------------------------*/

#define TICKS(nbms)        ((HZ*(nbms))/1000)

/*---------------------------------------------------------------------------*/

/*
 * Look in the PF register if it is on interrupt state.
 */
#ifdef CONFIG_XCOPILOT_BUGS
# define IS_PEN_DOWN    ((IPR&IPR_PEN)!=0)
#else
/*
 * to know if the pen is down or not, we have to read the PENIRQ port. But as
 * we don't know the state of the PF register before reading it, we have to
 * save it, set it for reading and then reset it at its initial state.
 */
# define IS_PEN_DOWN    is_pen_down()
int is_pen_down(void)
{
  int pf_dir=0,pf_sel=0;
  int down;

  /* save PF register settings */
  pf_dir = PFDIR;
  pf_sel = PFSEL;
  /* set PF register to read its state */
  PFDIR  &= ~PEN_MASK;                       /* I/O 1 of PENIRQ as input */
  PFSEL  |=  PEN_MASK;                       /* PF1 as I/O               */
  down = ((PFDATA & PF_IRQ5)==ST_PEN_DOWN);
  /* reset PF register with inital settings */
  PFDIR = pf_dir;
  PFSEL = pf_sel;
  return down;
}
#endif

/*
 * Test if the exchange with spim is ended. Used for sequential conversion.
 */
#define IS_XCHG_ENDED   ((SPIMCONT & SPIMCONT_XCH)==0)

/*
 * State of the BUSY signal of the SPIM.
 */
#define IS_BUSY_ENDED   ((BUSY_DATA & BUSY_MASK)==0)

/*
 * Write the SPIM (data and control).
 */
#define SET_SPIMDATA(x) { SPIMDATA = x; }
#define SET_SPIMCONT(x) { SPIMCONT = x; }

/*---------------------------------------------------------------------------*/

#define SPIM_ENABLE  { SET_SPIMCONT(SPIMCONT_ENABLE); }  /* enable SPIM      */
#define SPIM_INIT    { SPIMCONT |= SPIMCONT_INIT; }      /* init SPIM        */
#define SPIM_DISABLE { SET_SPIMCONT(0x0000); }           /* disable SPIM     */

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_XCOPILOT_BUGS
# define CARD_SELECT   
# define CARD_DESELECT 
#else
# define CARD_SELECT   { CS_DATA &= ~CS_MASK; }    /* PB4 active (=low)      */
# define CARD_DESELECT { CS_DATA |=  CS_MASK; }    /* PB4 inactive (=high)   */
#endif
/*---------------------------------------------------------------------------*/

#define ENABLE_PEN_IRQ    { IMR &= ~IMR_MIRQ5; }
#define ENABLE_SPIM_IRQ   { IMR &= ~IMR_MSPI; SPIMCONT |= SPIMCONT_IRQEN; }

#define DISABLE_PEN_IRQ   { IMR |= IMR_MIRQ5; }
#define DISABLE_SPIM_IRQ  { SPIMCONT &= ~SPIMCONT_IRQEN; IMR |= IMR_MSPI; }

#define CLEAR_SPIM_IRQ    { SPIMCONT &= ~SPIMCONT_IRQ; }

/*---------------------------------------------------------------------------*/
/* inline functions ---------------------------------------------------------*/

static inline void init_spim_hw(void) {
  /* Init stuff for port E. The 3 LSB are multiplexed with the SPIM */
  PESEL &= ~PESEL_MASK;
}
static inline void release_spim_hw(void){
  PESEL  |= PESEL_MASK;     /* default: 1 */
}

/* HW specifics -------------------------------------------------------------*/
static inline void init_pen_irq_hw(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
  /* Set bit 2 of port F for interrupt (IRQ5 for pen_irq)                 */
  PFPUEN |= PEN_MASK;     /* Port F as pull-up                            */
  PFDIR  &= ~PEN_MASK;    /* PF1 as input                                 */
  PFSEL  &= ~PEN_MASK;    /* PF1 internal chip function is connected      */
  /* Set polarity of IRQ5 as low (interrupt occure when signal is low)    */
  ICR &= ~ICR_POL5;

# if defined(CONFIG_M68EZ328_CHIPSLICE) || \
     defined(CONFIG_PILOT)
  /* Set bit 4 of port D for the pen irq pull-up of ADS7843 as output.    */
  PDDIR  |= PEN_MASK_2;      /* PD4 as output                             */
  PDDATA |= PEN_MASK_2;      /* PD4 up (pull-up)                          */
  PDPUEN |= PEN_MASK_2;      /* PD4 as pull-up                            */
#  ifndef CONFIG_XCOPILOT_BUGS
  PDSEL  |= PEN_MASK_2;      /* PD4 I/O port function pin is connected    */
#  endif
# endif
#else
# error -->> You have to define your hw configuration (inline functions) <<--
#endif
}

static inline void init_AD_converter_hw(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
  /* Set CARD_SELECT signal of ADS7843 as output.                         */
# ifndef CONFIG_XCOPILOT_BUGS
  CS_PUEN |= CS_MASK;      /* Port B as pull-up                           */
  CS_DIR  |= CS_MASK;      /* PB4 as output                               */
  CS_DATA |= CS_MASK;      /* PB4 inactive (=high)                        */
  CS_SEL  |= CS_MASK;      /* PB4 as I/O not dedicated                    */
# endif
  
  /*
   * Set BUSY signal of ADS7843 as input for EZ and as level sensitive irq
   * for the VZ.
   * From the spec of EZ about port D:
   * "Because there are no SELx bits associated with bits 3-0, the I/O function
   *  is always enabled for DIRx"
   * As I don't found anything comparable in the VZ spec, I think that with
   * a little bit of luck, the 3-0 bits of the D port will have the same
   * behavior... ;-)
   */
  BUSY_PUEN |= BUSY_MASK;    /* Port F as pull-up                        */
  BUSY_DIR  &= ~BUSY_MASK;   /* PF5 as input                             */
# ifdef CONFIG_M68VZ328_CHIPSLICE
  BUSY_POL &= ~BUSY_MASK;    /* Data is unchanged (not inverted)         */
  BUSY_IRQEN &= ~BUSY_MASK;  /* Interrupt is disable                     */
  BUSY_KBEN &= ~BUSY_MASK;   /* Keyboard interrupt is disable            */
  BUSY_IRQEG &= ~BUSY_MASK;  /* Level sensitive                          */
# else
  BUSY_SEL  |= BUSY_MASK;    /* PF5 I/O port function pin is connected   */
# endif
#else
# error -->> You have to define your hw configuration (inline functions) <<--
#endif
}

static inline void release_pen_irq_hw(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
# ifndef CONFIG_XCOPILOT_BUGS
#  ifdef CONFIG_M68EZ328_CHIPSLICE
  PDDIR  |= PEN_MASK_2;     /* default: 1 */
  PDDATA |= PEN_MASK_2;     /* default: 1 */
  PDPUEN |= PEN_MASK_2;     /* default: 1 */
  PDSEL  |= PEN_MASK_2;     /* default: 1 */
#  endif
  PFPUEN |= PEN_MASK;       /* default: 1 */
  PFDIR  &= ~PEN_MASK;      /* default: 0 */
  PFSEL  |= PEN_MASK;       /* default: 1 */
# endif
  ICR    &= ~PEN_MASK;      /* default: 0 */
  IMR    |= IMR_MIRQ5;      /* default: 1 */
#else
# error -->> You have to define your hw configuration (inline functions) <<--
#endif
}

static inline void release_AD_converter_hw(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
# ifndef CONFIG_XCOPILOT_BUGS
  CS_DIR  &= ~CS_MASK;      /* default: 0 */
  CS_DATA &= ~CS_MASK;      /* default: 0 */
  CS_PUEN |= CS_MASK;       /* default: 1 */
  CS_SEL  |= CS_MASK;       /* default: 1 */
  BUSY_PUEN |= BUSY_MASK;   /* default: 1 */
  BUSY_DIR  &= ~BUSY_MASK;  /* default: 0 */
#  ifdef CONFIG_M68VZ328_CHIPSLICE
  BUSY_POL &= ~BUSY_MASK;   /* default: 0 */
  BUSY_IRQEN &= ~BUSY_MASK; /* default: 0 */
  BUSY_KBEN &= ~BUSY_MASK;  /* default: 0 */
  BUSY_IRQEG &= ~BUSY_MASK; /* default: 0 */
#  else
  BUSY_SEL  |= BUSY_MASK;   /* default: 1 */
#  endif
# endif
#else
# error -->> You have to define your hw configuration (inline functions) <<--
#endif
}

/*
 * Toggle the pen irq port to I/O as output and to low (0) to reverse bias the
 * PENIRQ diode
 */
static inline void toggle_PEN_IRQ_2_io(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
  PFPUEN &= ~PEN_MASK;     /* pull-up disable                        */
  PFDIR  |= PEN_MASK;      /* I/O 1 of PENIRQ as output              */
  PFDATA &= ~PEN_MASK;     /* I/O 1 at 0                             */
  PFSEL  |= PEN_MASK;      /* PF1 I/O port function pin is connected */
#endif
}

/*
 * Toggle the pen irq port to dedicated (irq) function to catch irq from pen
 * when driver is in idle state
 */
static inline void toggle_PEN_IRQ_2_dedicated(void) {
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
  PFPUEN |= PEN_MASK;      /* Port F as pull-up                       */
  PFDIR  &= ~PEN_MASK;     /* I/O 1 of PENIRQ as input                */
  PFSEL  &= ~PEN_MASK;     /* PF1 dedicated function pin is connected */
#endif
}

/*
 * Set ADS7843 and SPIM parameters for transfer.
 */
static inline void set_SPIM_transfert(void) {
  /* DragonBall drive PENIRQ port LOW to reverse bias the PENIRQ diode
   * of the ADS7843
   */
#if defined(CONFIG_M68VZ328_CHIPSLICE) || \
    defined(CONFIG_M68EZ328_CHIPSLICE) || \
    defined(CONFIG_PILOT)
  PFDATA &= ~PEN_MASK;     /* Pull down I/O 1 of PENIRQ */
  PFDIR |= PEN_MASK;       /* I/O 1 of PENIRQ as output */
# ifdef CONFIG_M68EZ328_CHIPSLICE
  PDDATA &= ~PEN_MASK_2;   /* Pull down I/0 2 of PENIRQ */
# endif
#endif

  /* Initiate selection and parameters for using the Burr-Brown ADS7843
   * and the DragonBall SPIM.
   */
  CARD_SELECT;          /* Select ADS7843.     */
  SPIM_ENABLE;          /* Enable SPIM         */
  SPIM_INIT;            /* Set SPIM parameters */
}

/*
 * Release transfer settings.
 */
static inline void release_SPIM_transfert(void) {
  SPIM_DISABLE;
  CARD_DESELECT;

  PFPUEN |= PEN_MASK;       /* Port F as pull-up        */
  PFDIR &= ~PEN_MASK;       /* I/O 1 of PENIRQ as input */
#ifdef CONFIG_M68EZ328_CHIPSLICE
  PDDATA |= PEN_MASK_2;  /* Pull up I/0 2 of PENIRQ   */
#endif
}

/*---------------------------------------------------------------------------*/
/* predefinitions -----------------------------------------------------------*/

static void handle_timeout(void);
static void handle_pen_irq(int irq, void *dev_id, struct pt_regs *regs);
static void handle_spi_irq(void);

/*---------------------------------------------------------------------------*/
/* structure ----------------------------------------------------------------*/

struct ts_pen_position { int x,y; };

struct ts_queue {
  unsigned long head;
  unsigned long tail;
  struct wait_queue *proc_list;
  struct fasync_struct *fasync;
  unsigned char buf[TS_BUF_SIZE];
};

/*---------------------------------------------------------------------------*/
/* Variables ----------------------------------------------------------------*/

/*
 * driver state variable.
 */
static int ts_drv_state;

/*
 * first tick initiated at ts_open.
 */
static struct timeval   first_tick;

/*
 * pen state.
 */
static int new_pen_state;

/*
 * event counter.
 */
static int ev_counter;

/*
 * counter to differentiate a click from a move.
 */
static int state_counter;

static struct timer_list         ts_wake_time;

/*
 * drv parameters.
 */
static struct ts_drv_params      current_params;
static int                       sample_ticks;
static int                       deglitch_ticks;

/*
 * pen info variables.
 */
static struct ts_pen_info        ts_pen;
static struct ts_pen_info        ts_pen_prev;
static struct ts_pen_position    current_pos;

/*
 * driver management.
 */
static struct ts_queue *queue;
static int    device_open = 0; /* number of open device to prevent concurrent
                                * access to the same device
				*/
static int    device_locked = 0; /* state of the device.
				  * locked = 0; the driver doesn't do anything
				  * locked = 1; conversion are done either in
				  * 'state graph' or sequential mode
				  */



/*---------------------------------------------------------------------------*/
/* Init & release functions -------------------------------------------------*/

static void init_ts_state(void) {
  DISABLE_SPIM_IRQ;      /* Disable SPIM interrupt */

  ts_drv_state  = TS_DRV_IDLE;
  state_counter = 0;
  device_locked = 0;

  ENABLE_PEN_IRQ;        /* Enable interrupt IRQ5  */
}

static void init_ts_pen_values(void) {
  ts_pen.x = 0;
  ts_pen.y = 0;
  ts_pen.dx = 0;
  ts_pen.dy = 0;
  ts_pen.event = EV_PEN_UP;
  ts_pen.state &= 0;
  ts_pen.state |= ST_PEN_UP;

  ts_pen_prev.x = 0;
  ts_pen_prev.y = 0;
  ts_pen_prev.dx = 0;
  ts_pen_prev.dy = 0;
  ts_pen_prev.event = EV_PEN_UP;
  ts_pen_prev.state &= 0;
  ts_pen_prev.state |= ST_PEN_UP;
  
  new_pen_state = 0;
  ev_counter = 0;
  do_gettimeofday(&first_tick);
}

static void init_ts_timer(struct timer_list *timer) {
  init_timer(timer);
  timer->function = (void *)handle_timeout;
}

static void release_ts_timer(struct timer_list *timer) {
  del_timer(timer);
}

/*
 * Set default values for the params of the driver.
 */
static void init_ts_settings(void) {
  current_params.version = MC68328DIGI_VERSION;
  current_params.version_req = 0;
  current_params.x_ratio_num = 1;
  current_params.x_ratio_den = 1;
  current_params.y_ratio_num = 1;
  current_params.y_ratio_den = 1;
  current_params.x_offset = 0;
  current_params.y_offset = 0;
  current_params.xy_swap = 0;
  current_params.x_min = 0;
  current_params.x_max = 4095;
  current_params.y_min = 0;
  current_params.y_max = 4095;
  current_params.mv_thrs = 50;
  current_params.follow_thrs = 5;    /* to eliminate the conversion noise */
  current_params.sample_ms = 20;
  current_params.deglitch_ms = 20;
  current_params.event_queue_on = 1;
  sample_ticks = TICKS(current_params.sample_ms);
  deglitch_ticks = TICKS(current_params.deglitch_ms);
}

/*
 * hardware initialization and release
 */
static void init_ts_hardware(void) {
  init_pen_irq_hw();
  init_spim_hw();
  init_AD_converter_hw();
}
/*
 * reset bits used in each register to their default value.
 */
static void release_ts_hardware(void) {
  release_spim_hw();
  release_AD_converter_hw();
  release_pen_irq_hw();
}

/*
 * driver initialization and release
 */
static void init_ts_drv(void) {
  init_ts_state();
  init_ts_pen_values();
  init_ts_timer(&ts_wake_time);
  init_ts_hardware();
}
static void release_ts_drv(void) {
  release_ts_timer(&ts_wake_time);
  release_ts_hardware();
}


/*---------------------------------------------------------------------------*/
/* scaling functions --------------------------------------------------------*/

static inline void rescale_xpos(int *x) {
  *x &= CONV_MASK;

  *x *= current_params.x_ratio_num;
  *x /= current_params.x_ratio_den;
  *x += current_params.x_offset;
  *x = (*x > current_params.x_max ? current_params.x_max :
	(*x < current_params.x_min ? current_params.x_min :
	 *x));
}

static inline void rescale_ypos(int *y) {
  *y &= CONV_MASK;

  *y *= current_params.y_ratio_num;
  *y /= current_params.y_ratio_den;
  *y += current_params.y_offset;
  *y = (*y > current_params.y_max ? current_params.y_max :
	(*y < current_params.y_min ? current_params.y_min :
	 *y));
}

static inline void swap_xy(int *x, int *y)
{
  if(current_params.xy_swap) {
    int t = *x;
    *x = *y;
    *y = t;
  }
}

/*---------------------------------------------------------------------------*/
/* xcopilot compatibility hacks ---------------------------------------------*/
#ifdef CONFIG_XCOPILOT_BUGS

/* xcopilot has the following bugs:
 *
 * - Disabling the penirq has no effect; we keep on getting irqs even when
 *   penirq is disabled; this is not too annoying: we just trash pen irq events
 *   that come when disabled.
 *
 * - SPIM interrupt is not simulated; this is not too annoying: we just read
 *   SPI data immediately and bypass a couple of states related to SPI irq.
 *
 * - We do not get mouse drag events; this is a known xcopilot bug.
 *   This is annoying: we miss all moves ! You should patch and recompile
 *   your xcopilot.
 *
 *   This has been reported as Debian bug #64367, and there is a patch there:
 *     http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=64367&repeatmerged=yes
 *   which is:
 *   +-----------------------------------------------------------------------+
 *   | --- display.c   Tue Aug 25 14:56:02 1998
 *   | +++ display2.c  Fri May 19 16:07:52 2000
 *   | @@ -439,7 +439,7 @@
 *   | static void HandleDigitizer(Widget w, XtPointer client_data, XEvent\
 *   | *event, Boolean *continue_to_dispatch)
 *   | {
 *   | -  Time start;
 *   | +  static Time start;
 *   | 
 *   | *continue_to_dispatch = True;
 *   +-----------------------------------------------------------------------+
 *   In short, add 'static' to the declaration of 'start' in HandleDigitizer()
 *   in display.c
 *
 *   With this patch, all works perfectly !
 *
 * - The pen state can be read only in IPR, not in port D; again, we're doing
 *   the workaround.
 *
 * With all these workarounds in this file, and with the patch on xcopilot,
 * things go perfectly smoothly.
 *
 */

/*
 * Ugly stuff to read the mouse position on xcopilot
 *
 * We get the position of the last mouse up/down only, we get no moves !!! :-(
 */
static void xcopilot_read_now(void) {
    PFDATA &= ~0xf; PFDATA |= 6;
    SPIMCONT |= SPIMCONT_XCH | SPIMCONT_IRQEN;
    current_pos.x = SPIMDATA;
    rescale_xpos(&current_pos.x);

    PFDATA &= ~0xf; PFDATA |= 9;
    SPIMCONT |= SPIMCONT_XCH | SPIMCONT_IRQEN;
    current_pos.y = SPIMDATA;
    rescale_ypos(&current_pos.y);

    swap_xy(&current_pos.x, &current_pos.y);
}

#endif

/*---------------------------------------------------------------------------*/
/* flow functions -----------------------------------------------------------*/

/*
 * Set timer irq to now + delay ticks.
 */
static void set_timer_irq(struct timer_list *timer,int delay) {
  del_timer(timer);
  timer->expires = jiffies + delay;
  add_timer(timer);
}

/*
 * Clock the Burr-Brown to fall the AD_BUSY. 
 * With a 'start' bit and PD1,PD0 = 00 to enable PENIRQ.
 * Used for the first pen irq after booting. And when error occures during
 * conversion that need an initialisation.
 */
static void fall_BUSY_enable_PENIRQ(void) {
  SET_SPIMDATA(SPIMDATA_NOP);
  SPIMCONT |= SPIMCONT_XCH;     /* initiate exchange */
}

/*
 * Ask for an X conversion.
 */
static void ask_x_conv(void) {
  SET_SPIMDATA(SPIMDATA_ASKX);
  SPIMCONT |= SPIMCONT_XCH;     /* initiate exchange */
}

/*
 * Ask for an Y conversion.
 */
static void ask_y_conv(void) {
  SET_SPIMDATA(SPIMDATA_ASKY);
  SPIMCONT |= SPIMCONT_XCH;     /* initiate exchange */
}

/*
 * Get the X conversion. Re-enable the PENIRQ.
 */
static void read_x_conv(void) {
  /*
   * The first clock is used to fall the BUSY line and the following 15 clks
   * to transfert the 12 bits of the conversion, MSB first.
   * The result will then be in the SPIM register in the bits 14 to 3.
   */
  current_pos.x = (SPIMDATA & 0x7FF8) >> 3;

  SET_SPIMDATA(SPIMDATA_NOP);   /* nop with a start bit to re-enable */
  SPIMCONT |= SPIMCONT_XCH;     /* the pen irq.                      */

#if POSITION_DEBUG
  printk("%3i",current_pos.x);
#endif

  rescale_xpos(&current_pos.x);
}

/*
 * Get the Y result. Clock the Burr-Brown to fall the BUSY.
 */
static void read_y_conv(void) {
  /* Same remark as upper. */
  current_pos.y = (SPIMDATA & 0x7FF8) >> 3;

  SET_SPIMDATA(SPIMDATA_NULL);  /* No start bit to fall the BUSY without */
  SPIMCONT |= SPIMCONT_XCH;     /* initiating another BUSY...            */

#if POSITION_DEBUG
  printk(" %3i\n",current_pos.y);
#endif

  rescale_ypos(&current_pos.y);
}

/*---------------------------------------------------------------------------*/
/* queue functions ----------------------------------------------------------*/

/*
 * Get one char from the queue buffer.
 * AND the head with 'TS_BUF_SIZE -1' to have the loopback
 */
static unsigned char get_char_from_queue(void) {
  unsigned int result;

  result = queue->buf[queue->tail];
  queue->tail = (queue->tail + 1) & (TS_BUF_SIZE - 1);
  return result;
}

/*
 * Write one event in the queue buffer.
 * Test if there is place for an event = the head cannot be just one event
 * length after the queue.
 */
static void put_in_queue(char *in, int len) {
  unsigned long head    = queue->head;
  unsigned long maxhead = (queue->tail - len) & (TS_BUF_SIZE - 1);
  int i;
  
  if(head != maxhead)              /* There is place for the event */
    for(i=0;i<len;i++) {
      queue->buf[head] = in[i];
      head++;
      head &= (TS_BUF_SIZE - 1);
    }
#if EVENT_DEBUG
  else printk("%s: Queue is full !!!!\n", __FILE__);
#endif
  queue->head = head;
}

/*
 * Test if queue is empty.
 */
static inline int queue_empty(void) {
  return queue->head == queue->tail;
}

/*---------------------------------------------------------------------------*/
/* event generation functions -----------------------------------------------*/

/*
 * Test if the delta move of the pen is enough big to say that this is a really
 * move and not due to noise.
 */
static int is_moving(void) {
  int threshold;
  int dx, dy;
  
  threshold=((ts_pen_prev.event & EV_PEN_MOVE) > 0 ?
             current_params.follow_thrs : current_params.mv_thrs);
  dx = current_pos.x-ts_pen_prev.x;
  dy = current_pos.y-ts_pen_prev.y;
  return (abs(dx) > threshold ? 1 :
	  (abs(dy) > threshold ? 1 : 0));
}

static void copy_info(void) {
  ts_pen_prev.x = ts_pen.x;
  ts_pen_prev.y = ts_pen.y;
  ts_pen_prev.dx = ts_pen.dx;
  ts_pen_prev.dy = ts_pen.dy;
  ts_pen_prev.event = ts_pen.event;
  ts_pen_prev.state = ts_pen.state;
  ts_pen_prev.ev_no = ts_pen.ev_no;
  ts_pen_prev.ev_time = ts_pen.ev_time;
}

unsigned long set_ev_time(void) {
  struct timeval now;

  do_gettimeofday(&now);
  return (now.tv_sec -first_tick.tv_sec )*1000 +
         (now.tv_usec-first_tick.tv_usec)/1000;
}

static int cause_event(int conv) {
  int trash_UP_event = 0;

#if EVENT_DEBUG
  int print_debug = 0;
#endif

  switch(conv) {

  case CONV_ERROR: /* error occure during conversion */
    ts_pen.state &= 0;            /* clear                                   */
    ts_pen.state |= ST_PEN_UP;    /* recover a stable state for the drv.     */
    ts_pen.state |= ST_PEN_ERROR; /* tell that the state is due to an error  */
    ts_pen.event = EV_PEN_UP;     /* event = pen go to up                    */
    ts_pen.x = ts_pen_prev.x;     /* get previous coord as current to by-pas */
    ts_pen.y = ts_pen_prev.y;     /* possible errors                         */
    ts_pen.dx = 0;
    ts_pen.dy = 0;
    ts_pen.ev_no = ev_counter++;    /* get no of event   */
    ts_pen.ev_time = set_ev_time(); /* get time of event */
    copy_info();                    /* remember info     */
    if(current_params.event_queue_on)
      put_in_queue((char *)&ts_pen,DATA_LENGTH);  /* queue event */

    /* tell user space progs that a new state occure */
    new_pen_state = 1;
    /* signal asynchronous readers that data arrives */
    if(queue->fasync)
      kill_fasync(queue->fasync,SIGIO);
    wake_up_interruptible(&queue->proc_list);  /* tell user space progs */
#if EVENT_DEBUG
    print_debug = 1;
#endif
    break;
    
  case CONV_LOOP:  /* pen is down, conversion continue */    
    ts_pen.state &= 0;          /* clear */
    ts_pen.state &= ST_PEN_DOWN;

    switch(state_counter) {       /* It could be a move   */
	    
    case 1:    /* the pen just went down, it's a new state */
      /*
       * If the conversion is corrupt, we don't propagate this event and
       * even queue it. We dont take the last coord as the valid because the
       * last coord doesn't exist as this conversion is the first of the cycle.
       * And as this is the DOWN event, if we trash it, we have to trash the
       * following UP event too.
       */
      if(!(IS_PEN_DOWN)) {
        trash_UP_event = 1;
#if CORRUPT_DEBUG
        printk("D_CRPT\n");
#endif
	/* don't propagate event */
        break;
      }
      ts_pen.x = current_pos.x;
      ts_pen.y = current_pos.y;
      ts_pen.dx = 0;
      ts_pen.dy = 0;
      ts_pen.event = EV_PEN_DOWN;      /* event = pen go to down */
      ts_pen.ev_no = ev_counter++;     /* get no of event        */
      ts_pen.ev_time = set_ev_time();  /* get time of event */
      copy_info();                     /* remember info */
      if(current_params.event_queue_on)
        put_in_queue((char *)&ts_pen,DATA_LENGTH);  /* queue event */

      /* tell user space progs that a new state occure */
      new_pen_state = 1;
      /* signal asynchronous readers that data arrives */
      if(queue->fasync)
        kill_fasync(queue->fasync,SIGIO);
      wake_up_interruptible(&queue->proc_list);  /* tell user space progs */
#if EVENT_DEBUG
      print_debug = 1;
#endif
      break;

    case 2:    /* the pen is down for at least 2 loop of the state machine */
      if(is_moving()) {
	/*
	 * If the conversion is corrupt, we take the last coord as the valid
	 * ones. Maybe this could be useful in the future.
	 * But anyway, we don't propagate this event and even queue it.
	 */
        if(!(IS_PEN_DOWN)) {
	  current_pos.x = ts_pen_prev.x;
	  current_pos.y = ts_pen_prev.y;
#if CORRUPT_DEBUG
          printk("M_CRPT\n");
#endif
	/* don't propagate event */
	  break;
	}
        ts_pen.event = EV_PEN_MOVE;
        ts_pen.x = current_pos.x;
        ts_pen.y = current_pos.y;
        ts_pen.dx = ts_pen.x - ts_pen_prev.x;
        ts_pen.dy = ts_pen.y - ts_pen_prev.y;
        ts_pen.ev_no = ev_counter++;    /* get no of event   */
        ts_pen.ev_time = set_ev_time(); /* get time of event */
        copy_info();                    /* remember info */
        if(current_params.event_queue_on)
          put_in_queue((char *)&ts_pen,DATA_LENGTH);  /* queue event */

        /*
	 * pen is moving, then it's anyway a good reason to tell it to
	 * user space progs
	 */
        new_pen_state = 1;
        /* signal asynchronous readers that data arrives */
        if(queue->fasync)
          kill_fasync(queue->fasync,SIGIO);
        wake_up_interruptible(&queue->proc_list);
#if EVENT_DEBUG
        print_debug = 1;
#endif
      }
      else {
        if(ts_pen_prev.event == EV_PEN_MOVE) /* if previous state was moving */
	  ts_pen.event = EV_PEN_MOVE;        /* -> keep it!                  */
        ts_pen.x = ts_pen_prev.x;       /* No coord passing to     */
        ts_pen.y = ts_pen_prev.y;       /* avoid Parkinson effects */
        ts_pen.dx = 0;
	ts_pen.dy = 0;  /* no wake-up interruptible because nothing new */
        copy_info();    /* remember info */
      }
      break;
    }
    break;

  case CONV_END:   /* pen is up, conversion ends */
    ts_pen.state &= 0;         /* clear */
    ts_pen.state |= ST_PEN_UP;
    ts_pen.event = EV_PEN_UP;
    ts_pen.x = current_pos.x;
    ts_pen.y = current_pos.y;
    ts_pen.dx = ts_pen.x - ts_pen_prev.x;
    ts_pen.dy = ts_pen.y - ts_pen_prev.y;
    ts_pen.ev_time = set_ev_time();  /* get time of event */
    ts_pen.ev_no = ev_counter++;     /* get no of event   */
    copy_info();                     /* remember info */
    if(current_params.event_queue_on)
      put_in_queue((char *)&ts_pen,DATA_LENGTH);  /* queue event */

    /* tell user space progs that a new state occure */
    new_pen_state = 1;
    /* signal asynchronous readers that data arrives */
    if(queue->fasync)
      kill_fasync(queue->fasync,SIGIO);
    wake_up_interruptible(&queue->proc_list);
#if EVENT_DEBUG
    print_debug = 1;
#endif
    break;
  }

#if EVENT_DEBUG
if(print_debug)
  printk("%d%c%3d%3d%c\n", ts_pen.ev_no%10,
                         ts_pen.event == EV_PEN_UP ? 'U' :
                         ts_pen.event == EV_PEN_DOWN ? 'D' :
                         ts_pen.event == EV_PEN_MOVE ? 'M' :
                         '?',
                         ts_pen.x,ts_pen.y,
			 IS_PEN_DOWN ? '_' : '°');
#endif

  return trash_UP_event;
}

/*---------------------------------------------------------------------------*/
/* Interrupt functions ------------------------------------------------------*/

/*
 * pen irq.
 */
static void handle_pen_irq(int irq, void *dev_id, struct pt_regs *regs) {
  unsigned long flags;
  int bypass_initial_timer = 0;

  /* if unwanted interrupts come, trash them */ 
  if(!device_open)
    return;
  if(device_locked)
    return;         /* as we cannot schedule in an interrupt handler, we just
		     * return if the device is locked but this my normally not
		     * happend
		     */

#if EVENT_DEBUG
  printk("PIRQ\n");
#endif

#ifdef CONFIG_XCOPILOT_BUGS
  PFDATA &= ~PF_IRQ5;

  if(IMR&IMR_MIRQ5) {
    return;
  }
#endif
  save_flags(flags);     /* disable interrupts */
  cli();

  switch(ts_drv_state) {

  case TS_DRV_IDLE:  
    DISABLE_PEN_IRQ;
    device_locked = 1;   /* lock device         */
    ts_drv_state++;      /* update driver state */
    if(current_params.deglitch_ms)
      set_timer_irq(&ts_wake_time,deglitch_ticks);
    else
      bypass_initial_timer = 1;
    break;

  default: /* Error */
    /* PENIRQ is enable then just init the driver 
     * Its not necessary to pull down the busy signal
     */
    init_ts_state();
    break;
  }    
  restore_flags(flags);  /* Enable interrupts */

  /* if deglitching is off, we haven't started the deglitching timeout
   * so we do the inital sampling immediately:
   */
  if(bypass_initial_timer)
    handle_timeout();
}

/*
 * timer irq.
 */
static void handle_timeout(void) {
  unsigned long flags;
  int           trash_UP_event = 0;

  /* if unwanted interrupts come, trash them */ 
  if(!device_open)
    return;
  
  save_flags(flags);     /* disable interrupts */
  cli();
  
  switch(ts_drv_state) {

  case TS_DRV_IDLE:  /* Error in the idle state of the driver */
    printk("%s: Error in the idle state of the driver\n", __FILE__);
    goto treat_error;
  case TS_DRV_ASKX:  /* Error in release SPIM   */
    printk("%s: Error in release SPIM\n", __FILE__);
    goto treat_error;
  case TS_DRV_ASKY:  /* Error in ask Y coord    */
    printk("%s: Error in ask Y coord\n", __FILE__);
    goto treat_error;
  case TS_DRV_READX: /* Error in read X coord   */
    printk("%s: Error in read X coord\n", __FILE__);
    goto treat_error;
  case TS_DRV_READY: /* Error in read Y coord   */
    printk("%s: Error in read Y coord\n", __FILE__);
    goto treat_error;
  treat_error:
    /* Force re-enable of PENIRQ because of an abnormal termination. Limit this
     * initialization with a timer to define a more FATAL error ...
     */
    set_timer_irq(&ts_wake_time,TICKS(CONV_TIME_LIMIT));
    ts_drv_state = TS_DRV_ERROR;
    CLEAR_SPIM_IRQ;              /* Consume residual irq from spim */
    ENABLE_SPIM_IRQ;
    fall_BUSY_enable_PENIRQ();
    break;
    
  case TS_DRV_WAIT:
    if(state_counter) {
      trash_UP_event = cause_event(CONV_LOOP);  /* only after one loop */
    }
    if(IS_PEN_DOWN) {
      if(state_counter < 2) state_counter++;
      set_timer_irq(&ts_wake_time,TICKS(CONV_TIME_LIMIT));
      ts_drv_state++;
      set_SPIM_transfert();
      toggle_PEN_IRQ_2_io();
      CLEAR_SPIM_IRQ;            /* Consume residual irq from spim */
      ENABLE_SPIM_IRQ;
      fall_BUSY_enable_PENIRQ();
    }
    else {
      if(state_counter) {
#ifdef CONFIG_XCOPILOT_BUGS
	/* on xcopilot, we read the last mouse position now, because we
	 * missed the moves, during which the position should have been
	 * read
	 */
	xcopilot_read_now();
#endif
        /* if the previous down event was trashed, we have to trash the up one
	 * too, to avoid odd behavior of the propagated events
	 */
	if(!trash_UP_event)  cause_event(CONV_END);
#if CORRUPT_DEBUG
        else printk("U_TRSHD\n");
#endif
      }
      init_ts_state();
    }
    break;
    
  case TS_DRV_ERROR:  /* Busy doesn't want to fall down -> reboot? */
    panic(__FILE__": cannot pull down BUSY signal\n");

  default:
    init_ts_state();
  }
  restore_flags(flags);  /* Enable interrupts */

#ifdef CONFIG_XCOPILOT_BUGS
  if (ts_drv_state==TS_DRV_ASKX) {
    handle_spi_irq();
  }
  PFDATA |= PF_IRQ5;
#endif
}

/*
 * spi irq.
 */
static void handle_spi_irq(void) {
  unsigned long flags;

  /* if unwanted interrupts come, trash them */ 
  if(!device_open)
    return;
  
  save_flags(flags);     /* disable interrupts */
  cli();
  
  CLEAR_SPIM_IRQ;       /* clear source of interrupt */

  switch(ts_drv_state) {

  case TS_DRV_ASKX:
#ifdef CONFIG_XCOPILOT_BUGS
    /* for xcopilot we bypass all SPI interrupt-related stuff
     * and read the pos immediately
     */
    xcopilot_read_now();
    ts_drv_state = TS_DRV_WAIT;
    DISABLE_SPIM_IRQ;
    release_SPIM_transfert();
    set_timer_irq(&ts_wake_time,sample_ticks);
    break;
#endif
    if(IS_BUSY_ENDED) {     /* If first BUSY down, then */
      ask_x_conv();         /* go on with conversion    */
      ts_drv_state++;
    }
    else  fall_BUSY_enable_PENIRQ();  /* else, re-loop  */
    break;

  case TS_DRV_ASKY:
    ask_y_conv();
    ts_drv_state++;
    break;

  case TS_DRV_READX:
    read_x_conv();
    ts_drv_state++;
    break;

  case TS_DRV_READY:
    read_y_conv();
    swap_xy(&current_pos.x, &current_pos.y);
    ts_drv_state = TS_DRV_WAIT;
    break;

  case TS_DRV_WAIT:
    DISABLE_SPIM_IRQ;
    release_SPIM_transfert();
    toggle_PEN_IRQ_2_dedicated();
    set_timer_irq(&ts_wake_time,sample_ticks);
    break;

  case TS_DRV_ERROR:
    if(IS_BUSY_ENDED) {              /* If BUSY down... */
      release_SPIM_transfert();
      cause_event(CONV_ERROR);
      init_ts_state();
    }
    else fall_BUSY_enable_PENIRQ();  /* else, re-loop */
    break;

  default:
    init_ts_state();
  }
  restore_flags(flags);  /* Enable interrupts */

#ifdef CONFIG_XCOPILOT_BUGS
  PFDATA |= PF_IRQ5;
#endif
}

/*---------------------------------------------------------------------------*/
/* sequential conversion functions ------------------------------------------*/

static int sequential_conversion(int io_channel) {
  int conv=0;
#if defined(CONFIG_M68VZ328_CHIPSLICE) || defined(CONFIG_M68EZ328_CHIPSLICE)
  int           sched_flag;
  unsigned long flags;

#if SEQU_DEBUG
  printk("SEQU_CONV\n");
#endif

  /* if ask for sequential conversion come when driver is in 'state graph'
   * conversion, schedule
   */
  while(device_locked)
    schedule();

  save_flags(flags);     /* disable interrupts */
  cli();

  /* disable irq of pen */
  DISABLE_PEN_IRQ;

  /* lock device */
  device_locked = 1;

  /* Initiate selection and parameters for using the Burr-Brown ADS7843
   * and the DragonBall SPIM.
   */
  CARD_SELECT;     /* Select ADS7843.     */
  SPIM_ENABLE;     /* Enable SPIM         */
  SPIM_INIT;       /* Set SPIM parameters */

  /* Clock the Burr-Brown to fall the AD_BUSY */
  while(!IS_BUSY_ENDED) {
    sched_flag = 0;      /* to schedule only once */
    SET_SPIMDATA(SPIMDATA_NOP);
    SPIMCONT |= SPIMCONT_XCH;
    while(!IS_XCHG_ENDED) {
      schedule();        /* let other process running */
      sched_flag = 1;
    }
    if(!sched_flag)
      schedule();        /* let other process running */
  }

  if(io_channel == 3) { SET_SPIMDATA(SPIMDATA_ASKAUX_IN3); }
  else                { SET_SPIMDATA(SPIMDATA_ASKAUX_IN4); }
  SPIMCONT |= SPIMCONT_XCH;
  while(!IS_XCHG_ENDED)
    schedule();          /* let other process running */

  SET_SPIMDATA(SPIMDATA_NOP);
  SPIMCONT |= SPIMCONT_XCH;
  while(!IS_XCHG_ENDED)
    schedule();          /* let other process running */

  /*
   * The first clock is used to fall the BUSY line and the following 15 clks
   * to transfert the 12 bits of the conversion, MSB first.
   * The result will then be in the SPIM register in the bits 14 to 3.
   */
  conv = (SPIMDATA & 0x7FF4) >> 3;

  SET_SPIMDATA(SPIMDATA_NULL);  /* No start bit to fall the BUSY without */
  SPIMCONT |= SPIMCONT_XCH;     /* initiate an other BUSY...             */
  while(!IS_XCHG_ENDED)
    schedule();          /* let other process running */

  SPIM_DISABLE;
  CARD_DESELECT;

  /* release device */
  device_locked = 0;

  ENABLE_PEN_IRQ;

  restore_flags(flags);  /* Enable interrupts */

  conv &= 0x0FFF;

# if SEQU_DEBUG
  printk("conv: %d\n",conv);
# endif

#else
  /* not a Chipslice arch then maybe nothing's connected to aux I/O of the
   * Burr-Brown, set result of conversion to maximum
   */
  conv = 4095;

# if SEQU_DEBUG
  printk("Not a ChipSlice arch. Maybe an emulator?\n");
  printk("Return conversion max value (4095)\n");
# endif

#endif

  return conv;
}

/*---------------------------------------------------------------------------*/
/* file_operations functions ------------------------------------------------*/

/*
 * Called whenever a process attempts to read the device it has already open.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
static ssize_t ts_read (struct file *file,
                        char *buff,       /* the buffer to fill with data */
			size_t len,       /* the length of the buffer.    */
			loff_t *offset) { /* Our offset in the file       */
#else
static int ts_read(struct inode *inode, struct file *file,
		   char *buff,            /* the buffer to fill with data */
		   int len) {             /* the length of the buffer.    */
		                          /* NO WRITE beyond that !!!     */
#endif

  char *p_buffer;
  char c;
  int  i;
  int  err;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
  int  is_sig=0;
#endif


  while(!new_pen_state) {    /* noting to read */
    if(file->f_flags & O_NONBLOCK)
      return -EAGAIN;
    interruptible_sleep_on(&queue->proc_list);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
    for(i=0;i<_NSIG_WORDS && !is_sig;i++)
      is_sig = current->signal.sig[i] & ~current->blocked.sig[i];
    if(is_sig)
#else
    if(current->signal & ~current->blocked)  /* a signal arrived */
#endif
      return -ERESTARTSYS;    /* tell the fs layer to handle it  */
    /* otherwise loop */
  }

  /* force length to the one available */
  len = DATA_LENGTH;

  /* verify that the user space address to write is valid */
  err = verify_area(VERIFY_WRITE,buff,len);
  if(err) return err;

  /*
   * The events are anyway put in the queue.
   * But the read could choose to get the last event or the next in queue.
   * This decision is controlled through the ioctl function.
   * When the read doesn't flush the queue, this last is always full.
   */
  if(current_params.event_queue_on) {
    i = len;
    while((i>0) && !queue_empty()) {
      c = get_char_from_queue();
      put_user(c,buff++);
      i--;
    }
  }
  else {
    p_buffer = (char *)&ts_pen;
    for(i=0;i<len;i++)   put_user(p_buffer[i],buff+i);
  }

  if(queue_empty() || !current_params.event_queue_on)
    new_pen_state = 0;  /* queue events fully consumned */
  else
    new_pen_state = 1;  /* queue not empty              */

  return i;   /* return the number of bytes read */
}

/*
 * select file operation seems to become poll since version 2.2 of the kernel
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
static unsigned int ts_poll(struct file *file, poll_table *table) {

  poll_wait(file,&queue->proc_list,table);
  if (new_pen_state)
    return POLLIN | POLLRDNORM;
  return 0;

}
#else
static int ts_select(struct inode *inode, struct file *file,
                     int mode, select_table *table) {
  if(mode != SEL_IN)
    return 0;                   /* the device is readable only */
  if(new_pen_state)
    return 1;                   /* there is some new stuff to read */
  select_wait(&queue->proc_list,table);    /* wait for data */
  return 0;
}
#endif

/*
 * Called whenever a process attempts to do an ioctl to the device it has
 * already open.
 */
static int ts_ioctl(struct inode *inode, struct file *file,
		    unsigned int cmd,     /* the command to the ioctl */
		    unsigned long arg) {  /* the parameter to it      */

  struct ts_drv_params       new_params;
  struct ts_sequ_conv_params sc_params;
  int  err;
  int  i;
  char *p_in;
  char *p_out;

  switch(cmd) {
  
  case TS_PARAMS_GET:  /* Get internal params. First check if user space area
                        * to write is valid.
			*/
    err = verify_area(VERIFY_WRITE,(char *)arg,sizeof(current_params));
    if(err) return err;
    p_in  = (char *)&current_params;
    p_out = (char *)arg;
    for(i=0;i<sizeof(current_params);i++)
      put_user(p_in[i],p_out+i);
    return 0;

  case TS_PARAMS_SET:  /* Set internal params. First check if user space area
                        * to read is valid.
			*/
    err = verify_area(VERIFY_READ,(char *)arg,sizeof(new_params));
    if(err) {
      return err;
    }

    /* ok */
    p_in  = (char *)&new_params;
    p_out = (char *)arg;
    for(i=0;i<sizeof(new_params);i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
      get_user(p_in[i],p_out+i);
#else
      p_in[i] = get_user(p_out+i);
#endif
    }
    
    /* All the params are set with minimal test. */
    memcpy((void*)&current_params, (void*)&new_params, sizeof(current_params));
    sample_ticks = TICKS(current_params.sample_ms);
    deglitch_ticks = TICKS(current_params.deglitch_ms);

    /* check version */
    if(new_params.version_req!=-1) {
      if(new_params.version_req!=MC68328DIGI_VERSION) {
	printk("%s: error: driver version is %d, requested version is %d\n",
	       __FILE__, MC68328DIGI_VERSION, new_params.version_req);
	return -EBADRQC;
      }
    }

    return 0;

  case TS_SEQU_CONV:  /* start a sequential conversion if the driver isn't busy
                       * Otherwise, schedule.
		       */
#if SEQU_DEBUG
    printk("sizeof sc_params: %d\n",sizeof(sc_params);
#endif

    err = verify_area(VERIFY_READ,(char *)arg,sizeof(sc_params));
    if(err) {
      return err;
    }

    /* ok */
    p_in  = (char *)&sc_params;
    p_out = (char *)arg;
    for(i=0;i<sizeof(sc_params);i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
      get_user(p_in[i],p_out+i);
#else
      p_in[i] = get_user(p_out+i);
#endif
#if SEQU_DEBUG
      printk("%i\n",p_in[i]);
#endif
    }

#if SEQU_DEBUG
    printk("ts_ioctl: I/O channel %d\n",sc_params.io_channel);
#endif

    /* process the sequential conversion */
    sc_params.ret_val = sequential_conversion(sc_params.io_channel);

#if SEQU_DEBUG
    printk("ts_ioctl: conv = %d\n",sc_params.ret_val);
#endif

    err = verify_area(VERIFY_WRITE,(char *)arg,sizeof(sc_params));
    if(err) return err;
    p_in  = (char *)&sc_params;
    p_out = (char *)arg;
    for(i=0;i<sizeof(sc_params);i++) {
      put_user(p_in[i],p_out+i);
#if SEQU_DEBUG
      printk("%i\n",p_in[i]);
#endif
    }

    return 0;

  default:  /* cmd is not an ioctl command */
    return -ENOIOCTLCMD;
  }
}
		    
/*
 * Called whenever a process attempts to open the device file.
 */
static int ts_open(struct inode *inode, struct file *file) {
  int err;

  /*
   * only one device open in read-mode at a time.
   * Open in write only is then authorized to access sequential conversion 
   * through ioctl
   */
  if(device_open && !(file->f_flags & O_WRONLY)) {
#if INIT_DEBUG
    printk("write-only authorized since second open !!\n");
#endif
    return -EBUSY;
  }

  /* Authorize multiple open but initialize only once */
  if(!device_open) {
    /* IRQ registration have to be done before the hardware is instructed to
     * generate interruptions.
     */
    /* IRQ for pen */
    err = request_irq(IRQ_MACHSPEC | IRQ5_IRQ_NUM,handle_pen_irq,
		      IRQ_FLG_STD,"touch_screen",NULL);
    if(err) {
      printk("%s: Error. Cannot attach IRQ %d to touch screen device\n",
	      __FILE__, IRQ5_IRQ_NUM);
      return err;
    }
#if INIT_DEBUG
    printk("%s: IRQ %d attached to touch screen\n",
	   __FILE__, IRQ5_IRQ_NUM);
#endif

    /* IRQ for SPIM */
    err = request_irq(IRQ_MACHSPEC | SPI_IRQ_NUM,(void *)handle_spi_irq,
		      IRQ_FLG_STD,"spi_irq",NULL);
    if(err) {
      printk("%s: Error. Cannot attach IRQ %d to SPIM device\n",
	     __FILE__, SPI_IRQ_NUM);
      return err;
    }
#if INIT_DEBUG
    printk("%s: IRQ %d attached to SPIM\n",
	   __FILE__, SPI_IRQ_NUM);
#endif

    /* Init hardware */
    init_ts_drv();
  
    /* Init the queue */
    queue = (struct ts_queue *) kmalloc(sizeof(*queue),GFP_KERNEL);
    memset(queue,0,sizeof(*queue));
    queue->head = queue->tail = 0;
    queue->proc_list = NULL;
  }

  /* Increment the usage count (number of opened references to the module)
   * to be sure the module couldn't be removed while the file is open.
   */
  MOD_INC_USE_COUNT;

  /* And my own counter too */
  device_open++;

  return 0;  /* open succeed */
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
static int ts_fasync(int inode, struct file *file, int mode) {
#else
static int ts_fasync(struct inode *inode, struct file *file, int mode) {
#endif

  int retval;
  
  retval = fasync_helper(inode,file,mode,&queue->fasync);
  if(retval < 0)
    return retval;
  return 0;
}

/*
 * Called whenever a process attempts to close the device file.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
static int ts_release(struct inode *inode, struct file *file) {
#else
static void ts_release(struct inode *inode, struct file *file) {
#endif

  /* Decrement the usage count, otherwise once the file is open we'll never
   * get rid of the module.
   */
  MOD_DEC_USE_COUNT;

  /* And my own counter too */
  device_open--;

  if(!device_open) {      /* we are about to close the last open device */

  /* Remove this file from the asynchronously notified file's */
  ts_fasync(inode,file,0);

  /* Release hardware */
  release_ts_drv();

  /* Free the allocated memory for the queue */
  kfree(queue);

  /* IRQ have to be freed after the hardware is instructed not to interrupt
   * the processor any more.
   */
  free_irq(IRQ_MACHSPEC | SPI_IRQ_NUM,NULL);
  free_irq(IRQ_MACHSPEC | IRQ5_IRQ_NUM,NULL);

  }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
  return 0;
#endif
}


static struct file_operations ts_fops = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
  owner:          THIS_MODULE,
  read:           ts_read,
  poll:           ts_poll,
  ioctl:          ts_ioctl,
  open:           ts_open,
  release:        ts_release,
  fasync:         ts_fasync,
#else

  NULL,       /* lseek              */
  ts_read,    /* read               */
  NULL,       /* write              */
  NULL,       /* readdir            */
# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
  ts_poll,    /* poll               */
# else
  ts_select,  /* select             */
# endif
  ts_ioctl,   /* ioctl              */
  NULL,       /* mmap               */
  ts_open,    /* open               */
# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0))
  NULL,       /* flush              */
# endif
  ts_release, /* release (close)    */
  NULL,       /* fsync              */
  ts_fasync   /* async notification */

#endif
};

/*
 * miscdevice structure for misc driver registration.
 */
static struct miscdevice mc68328_digi = {
  MC68328DIGI_MINOR,"mc68328 digitizer",&ts_fops
};

/*
 * Initialize the driver.
 */
int mc68328digi_init(void) {
  int err;

  printk("%s: MC68328DIGI touch screen driver\n",__FILE__);

  /* Register the misc device driver */
  err = misc_register(&mc68328_digi);
  if(err<0)
    printk("%s: Error registering the device\n", __FILE__);
  else
    printk("%s: Device register with name: %s and number: %d %d\n",
	   __FILE__, mc68328_digi.name,10,mc68328_digi.minor);

  /* Init prameters settings at boot time */
  init_ts_settings();

  return err;     /* A non zero value means that init_module failed */
}

/*
 * Cleanup - undid whatever mc68328digi_init did.
 */
void mc68328digi_cleanup(void) {
  /* Unregister device driver */
  misc_deregister(&mc68328_digi);
}
