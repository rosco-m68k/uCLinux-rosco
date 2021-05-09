/* Copyright (C) 1997, 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <signal.h>

#include <sysdep.h>
#include <sys/syscall.h>
#include <bp-checks.h>

#ifdef __NR_rt_sigtimedwait
extern int __syscall_rt_sigtimedwait (const sigset_t *__unbounded, siginfo_t *__unbounded,
				      const struct timespec *__unbounded, size_t);


/* Return any pending signal or wait for one for the given time.  */
int
__sigtimedwait (set, info, timeout)
     const sigset_t *set;
     siginfo_t *info;
     const struct timespec *timeout;
{
  /* XXX The size argument hopefully will have to be changed to the
     real size of the user-level sigset_t.  */
  return INLINE_SYSCALL (rt_sigtimedwait, 4, CHECK_SIGSET (set),
			 CHECK_1 (info), timeout, _NSIG / 8);
}
weak_alias (__sigtimedwait, sigtimedwait)
#else
# include <sysdeps/generic/sigtimedwait.c>
#endif
