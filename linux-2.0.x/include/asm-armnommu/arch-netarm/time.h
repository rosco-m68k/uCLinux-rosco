/*
 * include/asm-armnommu/arch-netarm/time.h
 *
 * Copyright (C) 2000 NETsilicon, Inc.
 * Copyright (C) 2000 WireSpeed Communications Corporation
 *
 * This software is copyrighted by WireSpeed. LICENSEE agrees that
 * it will not delete this copyright notice, trademarks or protective
 * notices from any copy made by LICENSEE.
 *
 * This software is provided "AS-IS" and any express or implied 
 * warranties or conditions, including but not limited to any
 * implied warranties of merchantability and fitness for a particular
 * purpose regarding this software. In no event shall WireSpeed
 * be liable for any indirect, consequential, or incidental damages,
 * loss of profits or revenue, loss of use or data, or interruption
 * of business, whether the alleged damages are labeled in contract,
 * tort, or indemnity.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * author(s) : Joe deBlaquiere
 */

#include	<asm/arch/netarm_registers.h>

/* patched into do_gettimeoffset() - which uses microseconds */

extern __inline__ unsigned long gettimeoffset (void)
{
	volatile unsigned int *timer_status = 
		(volatile unsigned int *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_TIMER2_STATUS) ;

	/* convert milliseconds to microseconds */
	return  ( NETARM_GEN_TIMER_MSEC_P(*timer_status) * 1000 ) ;
}

/*
 * No need to reset the timer at every irq - timer rolls itself
 */
extern __inline__ int reset_timer()
{
#if 0
	setup_timer();
#endif	
	return (1);
}

/*
 * Updating of the RTC.  There is none, so do nothing :o)
 */
#define update_rtc()

/*
 * Set up timer interrupt, and return the current time in seconds.
 */

extern __inline__ unsigned long setup_timer (void)
{
	volatile unsigned int *timer_control = 
		(volatile unsigned int *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_TIMER2_CONTROL) ;

#if 1
	*timer_control = NETARM_GEN_TIMER_SET_HZ( HZ ) | 
		NETARM_GEN_TCTL_ENABLE | NETARM_GEN_TCTL_INT_ENABLE |
		NETARM_GEN_TCTL_USE_IRQ | NETARM_GEN_TCTL_INIT_COUNT(0) ;
#else
	*timer_control = NETARM_GEN_TIMER_SET_HZ( 4 ) | 
		NETARM_GEN_TCTL_ENABLE | NETARM_GEN_TCTL_INT_ENABLE |
		NETARM_GEN_TCTL_USE_IRQ | NETARM_GEN_TCTL_INIT_COUNT(0) ;
#endif

	printk("setup_timer : T2 CTL = %08X\n", ((unsigned long)*timer_control));
	
	/* probably need to store time in NVRAM at shutdown and restore old */
	/* but for now we just pick a time... 				    */

	return mktime(2000, 1, 7, 0, 0, 0);
}

