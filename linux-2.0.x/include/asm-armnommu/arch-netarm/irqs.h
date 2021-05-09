/*
 * linux/include/asm-arm/arch-netarm/irqs.h
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

#ifndef	_NETARM_IRQS_H_INCLUDED
#define	_NETARM_IRQS_H_INCLUDED

#define IRQ_PORTC0	0
#define IRQ_PORTC1	1
#define IRQ_PORTC2	2
#define IRQ_PORTC3	3
#define IRQ_TC2		4
#define IRQ_TC1		5
#define IRQ_WDOG	6
#define IRQ_SER2_TX	12
#define IRQ_SER2_RX	13
#define IRQ_SER1_TX	14
#define IRQ_SER1_RX	15
#define IRQ_ENET_TX	16
#define IRQ_ENET_RX	17
#define IRQ_ENIP4	18
#define IRQ_ENIP3	19
#define IRQ_ENIP2	20
#define IRQ_ENIP1	21
#define IRQ_DMA10	22	
#define IRQ_DMA09	23	
#define IRQ_DMA08	24	
#define IRQ_DMA07	25	
#define IRQ_DMA06	26	
#define IRQ_DMA05	27	
#define IRQ_DMA04	28	
#define IRQ_DMA03	29	
#define IRQ_DMA02	30	
#define IRQ_DMA01	31	

#endif
