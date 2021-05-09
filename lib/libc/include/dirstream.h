/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the, 1992 Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/*
 *	POSIX Standard: 5.1.2 Directory Operations	<dirent.h>
 */

#ifndef	_DIRSTREAM_H

#define	_DIRSTREAM_H	1

#include <features.h>
#include <sys/types.h>
#ifdef _POSIX_THREADS
#include <pthread.h>
#endif


#ifdef __UCLIBC_HAVE_LFS__
#ifndef __USE_LARGEFILE64
# define __USE_LARGEFILE64
#endif
# define stuff_t    __off64_t
#else
# define stuff_t    __off_t
#endif


/* For now, syscall readdir () only supports one entry at a time. It
 * will be changed in the future.
#define NUMENT		3
*/
#ifndef NUMENT
#define NUMENT		1
#endif

#define SINGLE_READDIR	11
#define MULTI_READDIR	12
#define NEW_READDIR	13

/* Directory stream type.  */
struct __dirstream {
  /* file descriptor */
  int dd_fd;

  /* offset of the next dir entry in buffer */
  stuff_t dd_nextloc;

  /* bytes of valid entries in buffer */
  stuff_t dd_size;

  /* -> directory buffer */
  void *dd_buf;

  /* offset of the next dir entry in directory. */
  stuff_t dd_nextoff;

  /* total size of buffer */
  stuff_t dd_max;
 
  enum {unknown, have_getdents, no_getdents} dd_getdents;

  /* lock */
#ifdef _POSIX_THREADS
  pthread_mutex_t *dd_lock;
#else
  void *dd_lock;
#endif
};				/* stream data from opendir() */

#endif /* dirent.h  */
