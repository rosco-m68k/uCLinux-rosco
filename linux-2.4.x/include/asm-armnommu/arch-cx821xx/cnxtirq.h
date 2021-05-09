/****************************************************************************
*
*	Name:			cnxtirq.h
*
*	Description:	
*
*	Copyright:		(c) 2002 Conexant Systems Inc.
*
*****************************************************************************

  This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

  This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

  You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA 02111-1307 USA

****************************************************************************
*  $Author: Davidsdj $
*  $Revision: 1 $
*  $Modtime: 11/08/02 12:50p $
****************************************************************************/


#ifndef CNXTIRQ_H
#define CNXTIRQ_H

#define INTLOCK() intLock()
#define INTUNLOCK(a) intUnlock(a)

void intUnlock( int oldlevel );
UINT32 intLock( void );

void PICClearIntStatus(UINT32 IntSource);
BOOL intContext( void );

#endif
