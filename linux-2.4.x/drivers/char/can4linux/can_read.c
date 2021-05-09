/*
 * can_read - can4linux CAN driver module
 *
 * can4linux -- LINUX CAN device driver source
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * 
 * Copyright (c) 2001 port GmbH Halle/Saale
 * (c) 2001 Heinz-J�rgen Oertel (oe@port.de)
 *          Claus Schroeter (clausi@chemie.fu-berlin.de)
 *------------------------------------------------------------------
 * $Header: /cvs/sw/linux-2.4.x/drivers/char/can4linux/can_read.c,v 1.1.1.2 2003/08/29 01:04:37 davidm Exp $
 *
 *--------------------------------------------------------------------------
 *
 *
 * modification history
 * --------------------
 * $Log: can_read.c,v $
 * Revision 1.1.1.2  2003/08/29 01:04:37  davidm
 * Import of uClinux-2.4.22-uc0
 *
 * Revision 1.1  2003/07/18 00:11:46  gerg
 * I followed as much rules as possible (I hope) and generated a patch for the
 * uClinux distribution. It contains an additional driver, the CAN driver, first
 * for an SJA1000 CAN controller:
 *   uClinux-dist/linux-2.4.x/drivers/char/can4linux
 * In the "user" section two entries
 *   uClinux-dist/user/can4linux     some very simple test examples
 *   uClinux-dist/user/horch         more sophisticated CAN analyzer example
 *
 * Patch submitted by Heinz-Juergen Oertel <oe@port.de>.
 *
 *
 *
 *
 *--------------------------------------------------------------------------
 */


/**
* \file can_read.c
* \author Heinz-J�rgen Oertel, port GmbH
* $Revision: 1.1.1.2 $
* $Date: 2003/08/29 01:04:37 $
*
* Module Description 
* see Doxygen Doc for all possibilities
*
*
*
*/


/* header of standard C - libraries */

/* header of common types */

/* shared common header */

/* header of project specific types */

/* project headers */
#include "can_defs.h"

/* local header */

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
/* static char _rcsid[] = "$Id: can_read.c,v 1.1.1.2 2003/08/29 01:04:37 davidm Exp $"; */



/***************************************************************************/
/**
*
* \brief ssize_t read(int fd, void *buf, size_t count);
* the read system call
* \param fd The descriptor to read from.
* \param buf The destination data buffer (array of CAN canmsg_t).
* \param count The number of bytes to read.
*
* read() attempts to read up to \a count CAN messages from file
* descriptor fd into the buffer starting at buf.
* buf must be large enough to hold count times the size of 
* one CAN message structure \b canmsg_t.
* 
* \code
int got;
canmsg_t rx[80];			// receive buffer for read()

    got = read(can_fd, rx , 80 );
    if( got > 0) {
      ...
    } else {
	// read returned with error
	fprintf(stderr, "- Received got = %d\n", got);
	fflush(stderr);
    }


* \endcode
*
* \par ERRORS
*
* the following errors can occur
*
* \arg \c EINVAL \b buf points not to an large enough area,
*
* \returns
* On success, the number of bytes read is returned
* (zero indicates end of file).
* It is not an  error if this number is
* smaller than the number of bytes requested;
* this may happen for example
* because fewer bytes are actually available right now,
* or because read() was interrupted by a signal.
* On error, -1 is returned, and errno is set  appropriately.
*
* \internal
*/

int can_read( __LDDK_READ_PARAM )
{
  int retval=0;

  DBGin("can_read");
  {
	unsigned int minor = __LDDK_MINOR;
	msg_fifo_t *RxFifo = &Rx_Buf[minor];
	canmsg_t *addr; 
	int written = 0;

	addr = (canmsg_t *)buffer;
	
	if( verify_area( VERIFY_WRITE, buffer, count * sizeof(canmsg_t) )) {
	   DBGout();
	   return -EINVAL;
	}
	while( written < count && RxFifo->status == BUF_OK ) {
	    if( RxFifo->tail == RxFifo->head ) {
		RxFifo->status = BUF_EMPTY;
		break;
	    }	

	    __lddk_copy_to_user( (canmsg_t *) &addr[written], 
			(canmsg_t *) &(RxFifo->data[RxFifo->tail]),
			sizeof(canmsg_t) );
	    written++;
	    RxFifo->tail = ++(RxFifo->tail) % MAX_BUFSIZE;
	}
	DBGout();
	return(written);
	
    } 
    DBGout();
    return retval;
}

