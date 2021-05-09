/* Copyright (C) 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>


static inline void
__ifreq (struct ifreq **ifreqs, int *num_ifs)
{
  int fd = __opensock ();
  struct ifconf ifc;
  int rq_len;
  int nifs;
# define RQ_IFS	4

  if (fd < 0)
    {
      *num_ifs = 0;
      *ifreqs = NULL;
      return;
    }

  ifc.ifc_buf = NULL;
  rq_len = RQ_IFS * sizeof (struct ifreq);
  do
    {
      ifc.ifc_len = rq_len;
      ifc.ifc_buf = realloc (ifc.ifc_buf, ifc.ifc_len);
      if (ifc.ifc_buf == NULL || __ioctl (fd, SIOCGIFCONF, &ifc) < 0)
	{
	  if (ifc.ifc_buf)
	    free (ifc.ifc_buf);
	  
	  __close (fd);
	  *num_ifs = 0;
	  *ifreqs = NULL;
	  return;
	}
      rq_len *= 2;
    }
  while (rq_len < sizeof (struct ifreq) + ifc.ifc_len);

  nifs = ifc.ifc_len / sizeof (struct ifreq);

  __close (fd);

  *num_ifs = nifs;
  *ifreqs = realloc (ifc.ifc_buf, nifs * sizeof (struct ifreq));
}


static inline void
__if_freereq (struct ifreq *ifreqs)
{
  free (ifreqs);
}
