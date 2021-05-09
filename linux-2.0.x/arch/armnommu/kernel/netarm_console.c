/*
 * linux/arch/armnommu/kernel/netarm_console.c
 *
 * Copyright (C) 2000, 2001 Red Hat, Inc.
 * Copyright (C) 2000, 2001 NETsilicon, Inc.
 *
 * This software is copyrighted by Red Hat. LICENSEE agrees that
 * it will not delete this copyright notice, trademarks or protective
 * notices from any copy made by LICENSEE.
 *
 * This software is provided "AS-IS" and any express or implied 
 * warranties or conditions, including but not limited to any
 * implied warranties of merchantability and fitness for a particular
 * purpose regarding this software. In no event shall Red Hat
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

void	netarm_console_print(const char *b)
{
	volatile unsigned long *ser_port_status;
	volatile unsigned long *ser_port_fifo;
	
	unsigned long *bPtr = (unsigned long *)b ;
	unsigned char *zPtr = (unsigned char *)b ;
	unsigned long ctmp;
	
	int count = 0;
	int acount;
	int scount;
	
	/* can't really handle errors at this low level so just die */
	if (0 == ((unsigned long)b)) _netarm_led_FAIL() ;
	
	ser_port_status = (volatile unsigned long*)(NETARM_SER_MODULE_BASE + 
		NETARM_SER_CH1_STATUS_A);
	ser_port_fifo   = (volatile unsigned char*)(NETARM_SER_MODULE_BASE + 
		NETARM_SER_CH1_FIFO);
	
	/* count the length of the string */
	while ('\0' != *zPtr)
	{
		zPtr ++ ;
		count ++ ;
	}

  	      zPtr = (unsigned char *)bPtr;
	while ( count > 0 )
	{
	      ctmp = 0 ;
	      scount = 0 ;
  	      while ( ( count > 0 ) && ( scount < 4 ) )
	      {
		      ctmp += *zPtr << ( scount << 3 ) ;
		      if (*zPtr == '\n')
		      {
		      	if ( scount < 3 )
			{
				scount++;
				ctmp += '\r' << (scount << 3) ; 
			}
			else
			{
				while ( (*ser_port_status & NETARM_SER_STATA_TX_RDY ) == 0)
				{
				}
				*ser_port_fifo = ctmp;
				scount = 0 ;
				ctmp = '\r' ;	
			}
		      }
		      zPtr++;
		      count -- ;
		      scount ++ ;
	      }
	      while ( (*ser_port_status & NETARM_SER_STATA_TX_RDY ) == 0)
	      {
	      }
	      *ser_port_fifo = ctmp;	

	}

}

void	netarm_dump_hex8(unsigned long int val)
{
	int i;
	char	buf[9];
	
	buf[8] = 0 ;
	for ( i = 0 ; i < 8 ; i++ )
	{
		buf[i] = (( val >> ( 28 - ( 4 * i ) ) ) & 0x0F) + '0' ;
		if ( buf[i] > '9' ) 
			buf[i] = (( val >> ( 28 - ( 4 * i ) ) ) & 0x0F) + ('A' - 10) ;
	}
	netarm_console_print(buf);
}

void	netarm_dump_int_stats(void)
{
	volatile unsigned long int *netarm_ie = 
		(volatile unsigned long int *)(NETARM_GEN_MODULE_BASE +
		NETARM_GEN_INTR_ENABLE );
	volatile unsigned long int *netarm_i_stat = 
		(volatile unsigned long int *)(NETARM_GEN_MODULE_BASE +
		NETARM_GEN_INTR_STATUS_RAW );

	netarm_console_print("interrupt enable = ");
	netarm_dump_hex8(*netarm_ie);
	netarm_console_print("\ninterrupt status = ");
	netarm_dump_hex8(*netarm_i_stat);
	netarm_console_print("\n");
}

#if 0
void	test_recurse(int depth)
{
	volatile int x;
	
	char	buffer[20];
	
	x = 1 ;

	if (depth > 0) test_recurse(depth-1);

	buffer[0] = 'H' ;
	buffer[1] = 'e' ;
	buffer[2] = 'l' ;
	buffer[3] = 'l' ;
	buffer[4] = 'o' ;
	buffer[5] = ' ' ;
	
	buffer[8] = '0' + (depth % 10) ;
	buffer[7] = '0' + ((depth/10) % 10) ;
	buffer[6] = '0' + ((depth/100) % 10) ;
	
	buffer[9] = '\n' ;
	buffer[10] = '\r' ;
	buffer[11] = '\0' ;

	netarm_console_print(buffer);
	
	x = 0 ;

}

void	test_stack()
{
	test_recurse(50);
}
#endif
