/****************************************************************************/

/*
 *	mcfmss.h -- MSS audio support for ColdFire environments.
 *
 *	(C) Copyright 2001, Greg Ungerer (gerg@snapgear.com)
 *	(C) Copyright 2001, Lineo Inc. (www.lineo.com) 
 */

/****************************************************************************/
#ifndef	mcfmss_h
#define	mcfmss_h
/****************************************************************************/

#include <linux/config.h>

/*
 *	Address defaults for MSS audio on Lineo/MP3 hardware.
 */
#define	MSS_BASE	0x30a00000
#define	MSS_DMA		0
#define	MSS_IRQ		25
#define	MSS_IRQDMA	120

#undef  DSP_BUFFSIZE
#define	DSP_BUFFSIZE	0x10000

/****************************************************************************/
#endif	/* mcfmss_h */
