/* Test whether RT signals are really available.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include <limits.h>
#include <string.h>
#include <sys/utsname.h>

static int
kernel_has_rtsig (void)
{
  return 0; /* hacked to test old uClibc that doesn't know about RT signals. 0.9.9 should work with RT signals. Make this proper in the end! */
#ifdef RTSIG_MAX
  return 1;
#else
  return 0;
#endif

/*
  struct utsname name;

  return uname (&name) == 0 && __strverscmp (name.release, "2.1.70") >= 0;
*/
}
