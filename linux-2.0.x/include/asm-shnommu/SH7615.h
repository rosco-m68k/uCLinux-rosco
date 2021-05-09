
/* include/asm- sh/SH7615.H
 *
 * Copyright (C) 2002  Ushustech pvt ltd www.ushustech.com
 *                   
 *  include/asm-shnommu/SH7615.H
 * 
 * 13-08-2002 BINOJ.G.S
 *
 */

#ifndef _SH7615_H_
#define _SH7615_H_

#include  "SH7615serial.h"

#define BYTE_REF(addr) (*((volatile unsigned char*)addr))
#define WORD_REF(addr) (*((volatile unsigned short*)addr))
#define LONG_REF(addr) (*((volatile unsigned long*)addr))


#define PUT_FIELD(field, val) (((val) << field##_SHIFT) & field##_MASK)
#define GET_FIELD(reg, field) (((reg) & field##_MASK) >> field##_SHIFT)
 

/* System  Registers */


#define BASE_ADDR	0xfffff000
 
/***************************************BSC*************************************************/

#define BCR1_ADDR     (BASE_ADDR+0xfe0)
#define BCR1           LONG_REF(BCR1_ADDR)
#define INIT_BCR1      0xA55A4031

#define BCR2_ADDR     (BASE_ADDR+0xfe4)   
#define BCR2           LONG_REF(BCR2_ADDR)
#define INIT_BCR2      0xA55A00F8


#define BCR3_ADDR     (BASE_ADDR+0xffc)   
#define BCR3           LONG_REF(BCR3_ADDR)
#define INIT_BCR3      0xA55A0100

#define WCR1_ADDR     (BASE_ADDR+0xfe8)
#define WCR1           LONG_REF(WCR1_ADDR)
#define INIT_WCR1      0xA55A55DF
 
#define WCR2_ADDR     (BASE_ADDR+0xfc0)
#define WCR2           LONG_REF(WCR2_ADDR)
#define INIT_WCR2      0xA55A0B06

#define WCR3_ADDR     (BASE_ADDR+0xfc4)   
#define WCR3           LONG_REF(WCR3_ADDR)
#define INIT_WCR3      0xA55A0000

#define MCR_ADDR      (BASE_ADDR+0xfec)   
#define MCR            LONG_REF(MCR_ADDR)
#define INIT_MCR       0xA55AC1C8

#define RTCSR_ADDR    (BASE_ADDR+0xff0)   
#define RTCSR          LONG_REF(RTCSR_ADDR)
#define INIT_RTCSR     0xA55A0090

#define RTCNT_ADDR    (BASE_ADDR+0xff4)   
#define RTCNT          LONG_REF(RTCNT_ADDR) /*SET A VALUE*/

#define RTCOR_ADDR    (BASE_ADDR+0xff8)   
#define RTCOR          LONG_REF(RTCOR_ADDR)
#define INIT_RTCOR     0xA55A0019



/************************************************SDRAM************************************************/

#define MRS_ADDR       0xffff8880   
#define MRS            LONG_REF(MRS_ADDR)
#define INIT_MRS       0xFFFF0880

/************************************************CCR**************************************************/

#define CCR_ADDR       (BASE_ADDR+0xe92)
#define CCR             BYTE_REF(CCR_ADDR)

/************************************************PFC**************************************************/
#define PBCR_ADDR      (BASE_ADDR+0xc88)
#define PBCR            WORD_REF(PBCR_ADDR)
#define INIT_PBCR       0x2A80

#define PBCR2_ADDR     (BASE_ADDR+0xc8e)
#define PBCR2           WORD_REF(PBCR2_ADDR)
#define INIT_PBCR2      0x0A00

#define PBIOR_ADDR     (BASE_ADDR+0xc8e)
#define PBIOR           WORD_REF(PBIOR_ADDR)
#define INIT_PBIOR      0x0000

#define PBDR_ADDR      (BASE_ADDR+0xc8c)
#define PBDR            WORD_REF(PBDR_ADDR)
#define INIT_PBDR       0x0000

/************************************************FMR**************************************************/

#define FMR_ADDR       (BASE_ADDR+0xe90)
#define FMR             LONG_REF(CCR_ADDR)/* SET A VALUE*/

/************************************************INTC*************************************************/

#define IPRA_ADDR      (BASE_ADDR+0xee2)
#define IPRA            WORD_REF(IPRA_ADDR)
#define INIT_IPRA       0x00F0

#define IPRB_ADDR      (BASE_ADDR+0xe60)
#define IPRB            WORD_REF(IPRB_ADDR)
#define INIT_IPRB       0xAD00

#define IPRD_ADDR      (BASE_ADDR+0xe40)
#define IPRD            WORD_REF(IPRD_ADDR)
#define INIT_IPRD       0x000A

#define IPRE_ADDR       (BASE_ADDR+0xec0)
#define IPRE            WORD_REF(IPRE_ADDR)
#define INIT_IPRE       0xA000

#define VCRA_ADDR      (BASE_ADDR+0xe62)
#define VCRA            WORD_REF(VCRA_ADDR)
#define INIT_VCRA       0x5300

#define VCRC_ADDR      (BASE_ADDR+0xe66)
#define VCRC            WORD_REF(VCRA_ADDR)
#define INIT_VCRC       0x5051

#define VCRD_ADDR      (BASE_ADDR+0xe68)
#define VCRD            WORD_REF(VCRD_ADDR)
#define INIT_VCRD       0x5200

#define VCRL_ADDR      (BASE_ADDR+0xe50)
#define VCRL            WORD_REF(VCRL_ADDR)
#define INIT_VCRL       0x4C4D

#define VCRM_ADDR      (BASE_ADDR+0xe52)
#define VCRM            WORD_REF(VCRM_ADDR)
#define INIT_VCRM       0x4E4E

#define VCRN_ADDR      (BASE_ADDR+0xe54)
#define VCRN            WORD_REF(VCRN_ADDR)
#define INIT_VCRN       0x5455

#define VCRO_ADDR      (BASE_ADDR+0xe56)
#define VCRO            WORD_REF(VCRO_ADDR)
#define INIT_VCRO       0x5656

/*********************************FRT******************************************************************/
#define TIER_ADDR      (BASE_ADDR+0xe10)
#define TIER            BYTE_REF(TIER_ADDR)
#define INIT_TIER       0x09

#define FTCSR_ADDR     (BASE_ADDR+0xe11)
#define FTCSR           BYTE_REF(FTCSR_ADDR)
#define INIT_FTCSR      0x01

#define FRC_H_ADDR     (BASE_ADDR+0xe12)
#define FRC_H           BYTE_REF(FRC_H_ADDR)
#define INIT_FRC_H      0x00

#define FRC_L_ADDR     (BASE_ADDR+0xe13)
#define FRC_L           BYTE_REF(FRC_L_ADDR)
#define INIT_FRC_L      0x00

#define OCRA_H_ADDR    (BASE_ADDR+0xe14)
#define OCRA_H          BYTE_REF(OCRA_H_ADDR)
#define INIT_OCRA_H     0x7A

#define OCRA_L_ADDR    (BASE_ADDR+0xe15)
#define OCRA_L          BYTE_REF(OCRA_L_ADDR)
#define INIT_OCRA_L     0x12

#define TCR_ADDR       (BASE_ADDR+0xe16)
#define TCR             BYTE_REF(TCR_ADDR)
#define INIT_TCR        0x00

#define TOCR_ADDR      (BASE_ADDR+0xe17)
#define TOCR            BYTE_REF(TOCR_ADDR)
#define INIT_TOCR       0xE0


#define INIT_OCIA_ENABLE 	0x09
#define INIT_OCFA_ENABLE 	0x01

#define CLR_FTCSR       0x01
/*****************************************WDT**********************************************************/
#define VCRWDT_ADDR    (BASE_ADDR+0xee4)
#define VCRWDT          WORD_REF(VCRWDT_ADDR)
#define INIT_VCRWDT     0x4A00

#define WTCSR_CNT_ADDR  (BASE_ADDR+0xe80)
#define WTCSR_CNT        WORD_REF(WTCSR_CNT_ADDR)
#define INIT_WTCSR       0xA51F

#define RSTCSR_ADDR     (BASE_ADDR+0xe82)
#define RSTCSR           WORD_REF(RSTCSR_ADDR)
#define INIT_RSTCSR      0x5ADF

#define CLOCK_SET        0XA50E   /*changed to approx 80 ms*/
#define TIMER_ENABLE     0xA520
#define TIMER_DISABLE    0xA53E  /*changed to approx 80 ms*/
#define OVERFLOW_CLEAR   0X00
#define COUNT_VALUE      0X5A0B


                                      /*ADD WTCNT REG HERE*/
#endif
/******************************************************************************************************/









