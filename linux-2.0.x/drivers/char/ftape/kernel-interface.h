#ifndef _KERNEL_INTERFACE_H
#define _KERNEL_INTERFACE_H

/*
 * Copyright (C) 1993-1995 Bas Laarhoven.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *
 $Source: /cvs/sw/linux-2.0.x/drivers/char/ftape/kernel-interface.h,v $
 $Author: christ $
 *
 $Revision: 1.1.1.1 $
 $Date: 1999/11/22 03:47:17 $
 $State: Exp $
 *
 * ----Description----
 *
 */

#include <linux/linkage.h>
#include <linux/signal.h>

#define _S(nr) (1<<((nr)-1))
#define _NEVER_BLOCK    (_S(SIGKILL)|_S(SIGSTOP))
#define _DONT_BLOCK     (_NEVER_BLOCK|_S(SIGINT))
#define _DO_BLOCK       (_S(SIGPIPE))
#define _BLOCK_ALL      (0xffffffffL)


#ifndef QIC117_TAPE_MAJOR
#define QIC117_TAPE_MAJOR 27
#endif

#define FTAPE_NO_REWIND 4	/* mask for minor nr */

/*      kernel-interface.c defined global variables.
 */
extern byte *tape_buffer[];
extern char kernel_version[];

/*      kernel-interface.c defined global functions.
 */
asmlinkage extern int init_module(void);
asmlinkage extern void cleanup_module(void);

/*      kernel global functions not (yet) standard accessible
 *      (linked at load time by modules package).
 */
asmlinkage extern int sys_sgetmask(void);
asmlinkage extern int sys_ssetmask(int);

#endif
