/* Copyright (C) 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
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
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef	_GLIBCSTAT_H
#define	_GLIBCSTAT_H	1

/* Versions of the `struct stat' data structure.  */
#define _STAT_VER_LINUX_OLD	1
#define _STAT_VER_SVR4		2
#define _STAT_VER_LINUX		3
#define _STAT_VER		_STAT_VER_LINUX	/* The one defined below.  */

/* Versions of the `xmknod' interface.  */
#define _MKNOD_VER_LINUX	1
#define _MKNOD_VER_SVR4		2
#define _MKNOD_VER		_MKNOD_VER_LINUX /* The bits defined below.  */

typedef unsigned long long int __glibc_dev_t;
typedef unsigned long int __glibc_ino_t;
typedef unsigned int __glibc_mode_t;
typedef unsigned int __glibc_nlink_t;
typedef unsigned int __glibc_uid_t;
typedef unsigned int __glibc_gid_t;
typedef long int __glibc_off_t;
typedef long int __glibc_time_t;

struct glibcstat
  {
    __glibc_dev_t st_dev;		/* Device.  */
    unsigned short int __pad1;
    __glibc_ino_t st_ino;		/* File serial number.	*/
    __glibc_mode_t st_mode;		/* File mode.  */
    __glibc_nlink_t st_nlink;		/* Link count.  */
    __glibc_uid_t st_uid;		/* User ID of the file's owner.	*/
    __glibc_gid_t st_gid;		/* Group ID of the file's group.*/
    __glibc_dev_t st_rdev;		/* Device number, if device.  */
    unsigned short int __pad2;
    __glibc_off_t st_size;		/* Size of file, in bytes.  */
    unsigned long int st_blksize;	/* Optimal block size for I/O.  */
#define	_STATBUF_ST_BLKSIZE		/* Tell code we have this member.  */

    unsigned long int st_blocks;	/* Number of 512-byte blocks allocated.  */
    __glibc_time_t st_atime;		/* Time of last access.  */
    unsigned long int __unused1;
    __glibc_time_t st_mtime;		/* Time of last modification.  */
    unsigned long int __unused2;
    __glibc_time_t st_ctime;			/* Time of last status change.  */
    unsigned long int __unused3;
    unsigned long int __unused4;
    unsigned long int __unused5;
  };

#endif	/* glibcstat.h */
