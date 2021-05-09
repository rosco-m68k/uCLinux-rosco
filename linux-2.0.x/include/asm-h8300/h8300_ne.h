/****************************************************************************/

/*
 *	h8300_ne.h -- NE2000 in H8/300H Evalution Board.
 *      
 *	(C) Copyright 2002, Yoshinori Sato <qzb04471@nifty.ne.jp>
 */

/****************************************************************************/
#ifndef	h8300ne_h
#define	h8300ne_h
/****************************************************************************/

#include <linux/config.h>

#if defined(CONFIG_BOARD_AKI3068NET)
#define NE2000_ADDR		CONFIG_NE_BASE
#define NE2000_IRQ_VECTOR	(12 + CONFIG_NE_IRQ)
#define	NE2000_BYTE		volatile unsigned short

#define IER                     0xfee015
#define ISR			0xfee016
#define IRQ_MASK		(1 << CONFIG_NE_IRQ)
#endif

/****************************************************************************/
#endif	/* h8300ne_h */
