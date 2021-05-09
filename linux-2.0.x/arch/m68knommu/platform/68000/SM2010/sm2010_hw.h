/*****************************************************************************/
/* 
 *  sm2010_hw.h, v1.0 <2003-07-30 11:56:37 gc>
 * 
 *  linux/arch/m68knommu/platform/68000/SM2010/sm2010_hw.h
 *
 *  hardware defines for our SM2010 board
 *  you probably need an own version of this file for your board
 *
 *  Author:     Guido Classen (classeng@clagi.de)
 *              Weiss-Electronic GmbH
 *
 *  This program is free software;  you can redistribute it and/or modify it
 *  under the  terms of the GNU  General Public License as  published by the
 *  Free Software Foundation.  See the file COPYING in the main directory of
 *  this archive for more details.
 *
 *  This program  is distributed  in the  hope that it  will be  useful, but
 *  WITHOUT   ANY   WARRANTY;  without   even   the   implied  warranty   of
 *  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR  PURPOSE.   See the  GNU
 *  General Public License for more details.
 * *
 *  Change history:
 *       2003-07-28 gc: initial version
 *
 */
 /****************************************************************************/

#ifndef __SM2010_HW_H
#define __SM2010_HW_H

#include <asm/traps.h>

#define SM2010_INT_NUM_SIO1     VEC_INT3
#define SM2010_INT_NUM_SIO2     VEC_INT4
#define SM2010_INT_NUM_SIO3     VEC_INT5

#define SM2010_INT_NUM_TIMER1   VEC_INT2
#define SM2010_INT_NUM_TIMER2   VEC_INT1


#define SM2010_SIO1_A_CONTROL   (*(volatile u8 *)  0xff8023)
#define SM2010_SIO1_A_DATA      (*(volatile u8 *)  0xff8021)
#define SM2010_SIO1_B_CONTROL   (*(volatile u8 *)  0xff8027)
#define SM2010_SIO1_B_DATA      (*(volatile u8 *)  0xff8025)
#define SM2010_SIO2_A_CONTROL   (*(volatile u8 *)  0xff802b)
#define SM2010_SIO2_A_DATA      (*(volatile u8 *)  0xff8029)
#define SM2010_SIO2_B_CONTROL   (*(volatile u8 *)  0xff802f)
#define SM2010_SIO2_B_DATA      (*(volatile u8 *)  0xff802d)
#define SM2010_SIO3_A_CONTROL   (*(volatile u8 *)  0xff8033)
#define SM2010_SIO3_A_DATA      (*(volatile u8 *)  0xff8031)
#define SM2010_SIO3_B_CONTROL   (*(volatile u8 *)  0xff8037)
#define SM2010_SIO3_B_DATA      (*(volatile u8 *)  0xff8035)

/* frequency on sys clock input of sio */
#define SM2010_SIO_CLOCK_SYS    6144000UL
/* frequency on xtal input of sio */
#define SM2010_SIO_CLOCK_XTAL   6000000UL       

#define SM2010_IO_PORT          (*(volatile u8 *)  0xff8001)
#define SM2010_SIO_STATUS       (*(volatile u8 *)  0xff8002)
#define SM2010_LED_PORT         (*(volatile u16 *) 0xff8008)
#define SM2010_BOARD_CONTROL    (*(volatile u8 *)  0xff8011)
#define SM2010_RESET_TIMER_INT1 (*(volatile u8 *)  0xff801b)
#define SM2010_RESET_TIMER_INT2 (*(volatile u8 *)  0xff801d)


/* structure of timer ports */
struct _sm2010_Hw_Timer { 
        u8 counter0;
        u8 dum_1;
        u8 counter1;
        u8 dum_2;
        u8 counter2;
        u8 dum_3;
        u8 control;
} __attribute__ ((packed));

#define SM2010_TIMER  (*(volatile struct _sm2010_Hw_Timer *) 0xff8039)

#endif  /* __SM2010_HW_H */


/*
 *Local Variables:
 * mode: c
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * indent-tabs-mode: nil
 * c-file-style: "Linux"
 * fill-column: 76
 * tab-width: 8
 * time-stamp-pattern: "4/<%%>"
 * End:
 */
