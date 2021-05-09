/*****************************************************************************/
/* 
 *  sm2010_ser_conf.h, v1.0 <2003-07-30 11:57:30 gc>
 * 
 *  linux/arch/m68knommu/platform/68000/SM2010/sm2010_ser_conf.h
 *
 *  hardware configuration for pd72001 serial driver for our SM2010 board
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

#ifndef __SM2010_SER_CONF_H
#define __SM2010_SER_CONF_H

#include "sm2010_hw.h"

#define MPSC_SIO_CLOCK_SYS      SM2010_SIO_CLOCK_SYS
#define MPSC_SIO_CLOCK_XTAL     SM2010_SIO_CLOCK_XTAL


/* definition of the available chip (each with 2 serial channels) */
static const struct mpsc_chip_info mpsc_chips[] = {
        {{ &SM2010_SIO1_A_CONTROL, &SM2010_SIO1_A_DATA }, 
         { &SM2010_SIO1_B_CONTROL, &SM2010_SIO1_B_DATA }, 
         SM2010_INT_NUM_SIO1 },
        {{ &SM2010_SIO2_A_CONTROL, &SM2010_SIO2_A_DATA }, 
         { &SM2010_SIO2_B_CONTROL, &SM2010_SIO2_B_DATA }, 
         SM2010_INT_NUM_SIO2 },
        {{ &SM2010_SIO3_A_CONTROL, &SM2010_SIO3_A_DATA }, 
         { &SM2010_SIO3_B_CONTROL, &SM2010_SIO3_B_DATA }, 
         SM2010_INT_NUM_SIO3 },
};

#define CHIP_SIO1               0
#define CHIP_SIO2               1
#define CHIP_SIO3               2

/* definition of the available channels */
const struct mpsc_channel_info mpsc_channels[] = {
        /* COM 5 => /dev/ttyS0 (console) */
        { CHIP_SIO1, 0, 1<<11, 1<<10 }, 
        
        /* COM 1 => /dev/ttyS1 */
        { CHIP_SIO2, 0, 1<<1, 1<<0 },
        
        /* COM 2 => /dev/ttyS2 */
        { CHIP_SIO2, 1, 1<<3, 1<<2 }, 
        
        /* COM 3 => /dev/ttyS3 */
        { CHIP_SIO1, 1, 1<<5, 1<<4 }, 
        
        /* COM 4 => /dev/ttyS4 */
        { CHIP_SIO3, 0, 1<<7, 1<<6 }, 
        
        /* COM 6 => /dev/ttyS5 */
        { CHIP_SIO3, 1, 1<<11, 1<<10 }

};

#define NUM_SERIAL              3     /* 3 chips on board. */

extern u16 panel_led_state;


/* turn on one ore more LEDs on the front panel.
 * @param led LED's corresponding to 1 bits in led are turned on, all
 *            other LED's are unchanged
 */
static inline void panel_led_on(u16 led)
{
  panel_led_state |= led;
  SM2010_LED_PORT = panel_led_state;
}


/* turn off one ore more LEDs on the front panel.
 * @param led LED's corresponding to 1 bits in led are turned off, all
 *            other LED's are unchanged
 */
static inline void panel_led_off(u16 led)
{
  panel_led_state &= ~led;
  SM2010_LED_PORT = panel_led_state;  
}

#endif /* __SM2010_SER_CONF_H */

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
