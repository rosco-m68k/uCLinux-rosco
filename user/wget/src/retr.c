/* File retrieval.
   Copyright (C) 1995, 1996, 1997, 1998, 2000 Free Software Foundation, Inc.

This file is part of Wget.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif /* HAVE_STRING_H */
#include <ctype.h>
#include <assert.h>

#include "wget.h"
#include "utils.h"
#include "retr.h"
#include "url.h"
#include "recur.h"
#include "ftp.h"
#include "host.h"
#include "connect.h"

#ifndef errno
extern int errno;
#endif

#ifdef WINDOWS
LARGE_INTEGER internal_time;
#else
/* Internal variables used by the timer.  */
static long internal_secs, internal_msecs;
#endif

void logflush PARAMS ((void));

/* From http.c.  */
uerr_t http_loop PARAMS ((struct urlinfo *, char **, int *));

/* Flags for show_progress().  */
enum spflags { SP_NONE, SP_INIT, SP_FINISH };

static int show_progress PARAMS ((long, long, enum spflags));

/* Reads the contents of file descriptor FD, until it is closed, or a
   read error occurs.  The data is read in 8K chunks, and stored to
   stream fp, which should have been open for writing.  If BUF is
   non-NULL and its file descriptor is equal to FD, flush RBUF first.
   This function will *not* use the rbuf_* functions!

   The EXPECTED argument is passed to show_progress() unchanged, but
   otherwise ignored.

   If opt.verbose is set, the progress is also shown.  RESTVAL
   represents a value from which to start downloading (which will be
   shown accordingly).  If RESTVAL is non-zero, the stream should have
   been open for appending.

   The function exits and returns codes of 0, -1 and -2 if the
   connection was closed, there was a read error, or if it could not
   write to the output stream, respectively.

   IMPORTANT: The function flushes the contents of the buffer in
   rbuf_flush() before actually reading from fd.  If you wish to read
   from fd immediately, flush or discard the buffer.  */
int
get_contents (int fd, FILE *fp, long *len, long restval, long expected,
	      struct rbuf *rbuf)
{
  int res;
  static char c[8192];

  *len = restval;
  if (opt.verbose)
    show_progress (restval, expected, SP_INIT);
  if (rbuf && RBUF_FD (rbuf) == fd)
    {
      while ((res = rbuf_flush (rbuf, c, sizeof (c))) != 0)
	{
	  if (fwrite (c, sizeof (char), res, fp) < res)
	    return -2;
	  if (opt.verbose)
	    {
	      if (show_progress (res, expected, SP_NONE))
		fflush (fp);
	    }
	  *len += res;
	}
    }
  /* Read from fd while there is available data.  */
  do
    {
      res = iread (fd, c, sizeof (c));
      if (res > 0)
	{
	  if (fwrite (c, sizeof (char), res, fp) < res)
	    return -2;
	  if (opt.verbose)
	    {
	      if (show_progress (res, expected, SP_NONE))
		fflush (fp);
	    }
	  *len += res;
	}
    } while (res > 0);
  if (res < -1)
    res = -1;
  if (opt.verbose)
    show_progress (0, expected, SP_FINISH);
  return res;
}

static void
print_percentage (long bytes, long expected)
{
  int percentage = (int)(100.0 * bytes / expected);
  logprintf (LOG_VERBOSE, " [%3d%%]", percentage);
}

/* Show the dotted progress report of file loading.  Called with
   length and a flag to tell it whether to reset or not.  It keeps the
   offset information in static local variables.

   Return value: 1 or 0, designating whether any dots have been drawn.

   If the init argument is set, the routine will initialize.

   If the res is non-zero, res/line_bytes lines are skipped
   (meaning the appropriate number ok kilobytes), and the number of
   "dots" fitting on the first line are drawn as ','.  */
static int
show_progress (long res, long expected, enum spflags flags)
{
  static long line_bytes;
  static long offs;
  static int ndot, nrow;
  int any_output = 0;

  if (flags == SP_FINISH)
    {
      if (expected)
	{
	  int dot = ndot;
	  char *tmpstr = (char *)alloca (2 * opt.dots_in_line + 1);
	  char *tmpp = tmpstr;
	  for (; dot < opt.dots_in_line; dot++)
	    {
	      if (!(dot % opt.dot_spacing))
		*tmpp++ = ' ';
	      *tmpp++ = ' ';
	    }
	  *tmpp = '\0';
	  logputs (LOG_VERBOSE, tmpstr);
	  print_percentage (nrow * line_bytes + ndot * opt.dot_bytes + offs,
			    expected);
	}
      logputs (LOG_VERBOSE, "\n\n");
      return 0;
    }

  /* Temporarily disable flushing.  */
  opt.no_flush = 1;
  /* init set means initialization.  If res is set, it also means that
     the retrieval is *not* done from the beginning.  The part that
     was already retrieved is not shown again.  */
  if (flags == SP_INIT)
    {
      /* Generic initialization of static variables.  */
      offs = 0L;
      ndot = nrow = 0;
      line_bytes = (long)opt.dots_in_line * opt.dot_bytes;
      if (res)
	{
	  if (res >= line_bytes)
	    {
	      nrow = res / line_bytes;
	      res %= line_bytes;
	      logprintf (LOG_VERBOSE,
			 _("\n          [ skipping %dK ]"),
			 (int) ((nrow * line_bytes) / 1024));
	      ndot = 0;
	    }
	}
      logprintf (LOG_VERBOSE, "\n%5ldK ->", nrow * line_bytes / 1024);
    }
  /* Offset gets incremented by current value.  */
  offs += res;
  /* While offset is >= opt.dot_bytes, print dots, taking care to
     precede every 50th dot with a status message.  */
  for (; offs >= opt.dot_bytes; offs -= opt.dot_bytes)
    {
      if (!(ndot % opt.dot_spacing))
	logputs (LOG_VERBOSE, " ");
      any_output = 1;
      logputs (LOG_VERBOSE, flags == SP_INIT ? "," : ".");
      ++ndot;
      if (ndot == opt.dots_in_line)
	{
	  ndot = 0;
	  ++nrow;
	  if (expected)
	    print_percentage (nrow * line_bytes, expected);
	  logprintf (LOG_VERBOSE, "\n%5ldK ->", nrow * line_bytes / 1024);
	}
    }
  /* Reenable flushing.  */
  opt.no_flush = 0;
  if (any_output)
    /* Force flush.  #### Oh, what a kludge!  */
    logflush ();
  return any_output;
}

/* Reset the internal timer.  */
void
reset_timer (void)
{
#ifndef WINDOWS
  /* Under Unix, the preferred way to measure the passage of time is
     through gettimeofday() because of its granularity.  However, on
     some old or weird systems, gettimeofday() might not be available.
     There we use the simple time().  */
# ifdef HAVE_GETTIMEOFDAY
  struct timeval t;
  gettimeofday (&t, NULL);
  internal_secs = t.tv_sec;
  internal_msecs = t.tv_usec / 1000;
# else  /* not HAVE_GETTIMEOFDAY */
  internal_secs = time (NULL);
  internal_msecs = 0;
# endif /* not HAVE_GETTIMEOFDAY */
#else  /* WINDOWS */
  /* Under Windows, use Windows-specific APIs. */
  FILETIME ft;
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st,&ft);
  internal_time.HighPart = ft.dwHighDateTime;
  internal_time.LowPart = ft.dwLowDateTime;
#endif /* WINDOWS */
}

/* Return the time elapsed from the last call to reset_timer(), in
   milliseconds.  */
long
elapsed_time (void)
{
#ifndef WINDOWS
# ifdef HAVE_GETTIMEOFDAY
  struct timeval t;
  gettimeofday (&t, NULL);
  return ((t.tv_sec - internal_secs) * 1000
	  + (t.tv_usec / 1000 - internal_msecs));
# else  /* not HAVE_GETTIMEOFDAY */
  return 1000 * ((long)time (NULL) - internal_secs);
# endif /* not HAVE_GETTIMEOFDAY */
#else  /* WINDOWS */
  FILETIME ft;
  SYSTEMTIME st;
  LARGE_INTEGER li;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st,&ft);
  li.HighPart = ft.dwHighDateTime;
  li.LowPart = ft.dwLowDateTime;
  return (long) ((li.QuadPart - internal_time.QuadPart) / 1e4);
#endif /* WINDOWS */
}

/* Print out the appropriate download rate.  Appropriate means that if
   rate is > 1024 bytes per second, kilobytes are used, and if rate >
   1024 * 1024 bps, megabytes are used.  */
char *
rate (long bytes, long msecs)
{
  static char res[15];
  double dlrate;

  if (!msecs)
    ++msecs;
  dlrate = (double)1000 * bytes / msecs;
  /* #### Should these strings be translatable?  */
  if (dlrate < 1024.0)
    sprintf (res, "%.2f B/s", dlrate);
  else if (dlrate < 1024.0 * 1024.0)
    sprintf (res, "%.2f KB/s", dlrate / 1024.0);
  else
    sprintf (res, "%.2f MB/s", dlrate / (1024.0 * 1024.0));
  return res;
}

#define USE_PROXY_P(u) (opt.use_proxy && getproxy((u)->proto)		\
			&& no_proxy_match((u)->host,			\
					  (const char **)opt.no_proxy))

/* Retrieve the given URL.  Decides which loop to call -- HTTP, FTP,
   or simply copy it with file:// (#### the latter not yet
   implemented!).  */
uerr_t
retrieve_url (const char *origurl, char **file, char **newloc,
	      const char *refurl, int *dt)
{
  uerr_t result;
  char *url;
  int location_changed, dummy;
  int local_use_proxy;
  char *mynewloc, *proxy;
  struct urlinfo *u;
  slist *redirections;

  /* If dt is NULL, just ignore it.  */
  if (!dt)
    dt = &dummy;
  url = xstrdup (origurl);
  if (newloc)
    *newloc = NULL;
  if (file)
    *file = NULL;

  redirections = NULL;

  u = newurl ();
  /* Parse the URL. */
  result = parseurl (url, u, 0);
  if (result != URLOK)
    {
      logprintf (LOG_NOTQUIET, "%s: %s.\n", url, uerrmsg (result));
      freeurl (u, 1);
      free_slist (redirections);
      free (url);
      return result;
    }

 redirected:

  /* Set the referer.  */
  if (refurl)
    u->referer = xstrdup (refurl);
  else
    {
      if (opt.referer)
	u->referer = xstrdup (opt.referer);
      else
	u->referer = NULL;
    }

  local_use_proxy = USE_PROXY_P (u);
  if (local_use_proxy)
    {
      struct urlinfo *pu = newurl ();

      /* Copy the original URL to new location.  */
      memcpy (pu, u, sizeof (*u));
      pu->proxy = NULL; /* A minor correction :) */
      /* Initialize u to nil.  */
      memset (u, 0, sizeof (*u));
      u->proxy = pu;
      /* Get the appropriate proxy server, appropriate for the
	 current protocol.  */
      proxy = getproxy (pu->proto);
      if (!proxy)
	{
	  logputs (LOG_NOTQUIET, _("Could not find proxy host.\n"));
	  freeurl (u, 1);
	  free_slist (redirections);
	  free (url);
	  return PROXERR;
	}
      /* Parse the proxy URL.  */
      result = parseurl (proxy, u, 0);
      if (result != URLOK || u->proto != URLHTTP)
	{
	  if (u->proto == URLHTTP)
	    logprintf (LOG_NOTQUIET, "Proxy %s: %s.\n", proxy, uerrmsg(result));
	  else
	    logprintf (LOG_NOTQUIET, _("Proxy %s: Must be HTTP.\n"), proxy);
	  freeurl (u, 1);
	  free_slist (redirections);
	  free (url);
	  return PROXERR;
	}
      u->proto = URLHTTP;
    }

  assert (u->proto != URLFILE);	/* #### Implement me!  */
  mynewloc = NULL;

  if (u->proto == URLHTTP)
    result = http_loop (u, &mynewloc, dt);
  else if (u->proto == URLFTP)
    {
      /* If this is a redirection, we must not allow recursive FTP
	 retrieval, so we save recursion to oldrec, and restore it
	 later.  */
      int oldrec = opt.recursive;
      if (redirections)
	opt.recursive = 0;
      result = ftp_loop (u, dt);
      opt.recursive = oldrec;
      /* There is a possibility of having HTTP being redirected to
	 FTP.  In these cases we must decide whether the text is HTML
	 according to the suffix.  The HTML suffixes are `.html' and
	 `.htm', case-insensitive.

	 #### All of this is, of course, crap.  These types should be
	 determined through mailcap.  */
      if (redirections && u->local && (u->proto == URLFTP ))
	{
	  char *suf = suffix (u->local);
	  if (suf && (!strcasecmp (suf, "html") || !strcasecmp (suf, "htm")))
	    *dt |= TEXTHTML;
	  FREE_MAYBE (suf);
	}
    }
  location_changed = (result == NEWLOCATION);
  if (location_changed)
    {
      char *construced_newloc;
      uerr_t newloc_result;
      struct urlinfo *newloc_struct;

      assert (mynewloc != NULL);

      /* The HTTP specs only allow absolute URLs to appear in
	 redirects, but a ton of boneheaded webservers and CGIs out
	 there break the rules and use relative URLs, and popular
	 browsers are lenient about this, so wget should be too. */
      construced_newloc = url_concat (url, mynewloc);
      free (mynewloc);
      mynewloc = construced_newloc;

      /* Now, see if this new location makes sense. */
      newloc_struct = newurl ();
      newloc_result = parseurl (mynewloc, newloc_struct, 1);
      if (newloc_result != URLOK)
	{
	  logprintf (LOG_NOTQUIET, "%s: %s.\n", mynewloc, uerrmsg (newloc_result));
	  freeurl (newloc_struct, 1);
	  freeurl (u, 1);
	  free_slist (redirections);
	  free (url);
	  free (mynewloc);
	  return result;
	}

      /* Now mynewloc will become newloc_struct->url, because if the
         Location contained relative paths like .././something, we
         don't want that propagating as url.  */
      free (mynewloc);
      mynewloc = xstrdup (newloc_struct->url);

      /* Check for redirection to back to itself.  */
      if (!strcmp (u->url, newloc_struct->url))
	{
	  logprintf (LOG_NOTQUIET, _("%s: Redirection to itself.\n"),
		     mynewloc);
	  freeurl (newloc_struct, 1);
	  freeurl (u, 1);
	  free_slist (redirections);
	  free (url);
	  free (mynewloc);
	  return WRONGCODE;
	}

      /* The new location is OK.  Let's check for redirection cycle by
         peeking through the history of redirections. */
      if (in_slist (redirections, newloc_struct->url))
	{
	  logprintf (LOG_NOTQUIET, _("%s: Redirection cycle detected.\n"),
		     mynewloc);
	  freeurl (newloc_struct, 1);
	  freeurl (u, 1);
	  free_slist (redirections);
	  free (url);
	  free (mynewloc);
	  return WRONGCODE;
	}

      redirections = add_slist (redirections, newloc_struct->url, NOSORT);

      free (url);
      url = mynewloc;
      freeurl (u, 1);
      u = newloc_struct;
      goto redirected;
    }

  if (file)
    {
      if (u->local)
	*file = xstrdup (u->local);
      else
	*file = NULL;
    }
  freeurl (u, 1);
  free_slist (redirections);

  if (newloc)
    *newloc = url;
  else
    free (url);

  return result;
}

/* Find the URLs in the file and call retrieve_url() for each of
   them.  If HTML is non-zero, treat the file as HTML, and construct
   the URLs accordingly.

   If opt.recursive is set, call recursive_retrieve() for each file.  */
uerr_t
retrieve_from_file (const char *file, int html, int *count)
{
  uerr_t status;
  urlpos *url_list, *cur_url;

  /* If spider-mode is on, we do not want get_urls_html barfing
     errors on baseless links.  */
  url_list = (html ? get_urls_html (file, NULL, opt.spider, FALSE)
	      : get_urls_file (file));
  status = RETROK;             /* Suppose everything is OK.  */
  *count = 0;                  /* Reset the URL count.  */
  recursive_reset ();
  for (cur_url = url_list; cur_url; cur_url = cur_url->next, ++*count)
    {
      char *filename, *new_file;
      int dt;

      if (downloaded_exceeds_quota ())
	{
	  status = QUOTEXC;
	  break;
	}
      status = retrieve_url (cur_url->url, &filename, &new_file, NULL, &dt);
      if (opt.recursive && status == RETROK && (dt & TEXTHTML))
	status = recursive_retrieve (filename, new_file ? new_file
				                        : cur_url->url);

      if (filename && opt.delete_after && file_exists_p (filename))
	{
	  DEBUGP (("Removing file due to --delete-after in"
		   " retrieve_from_file():\n"));
	  logprintf (LOG_VERBOSE, _("Removing %s.\n"), filename);
	  if (unlink (filename))
	    logprintf (LOG_NOTQUIET, "unlink: %s\n", strerror (errno));
	  dt &= ~RETROKF;
	}

      FREE_MAYBE (new_file);
      FREE_MAYBE (filename);
    }

  /* Free the linked list of URL-s.  */
  free_urlpos (url_list);

  return status;
}

/* Print `giving up', or `retrying', depending on the impending
   action.  N1 and N2 are the attempt number and the attempt limit.  */
void
printwhat (int n1, int n2)
{
  logputs (LOG_VERBOSE, (n1 == n2) ? _("Giving up.\n\n") : _("Retrying.\n\n"));
}

/* Increment opt.downloaded by BY_HOW_MUCH.  If an overflow occurs,
   set opt.downloaded_overflow to 1. */
void
downloaded_increase (unsigned long by_how_much)
{
  VERY_LONG_TYPE old;
  if (opt.downloaded_overflow)
    return;
  old = opt.downloaded;
  opt.downloaded += by_how_much;
  if (opt.downloaded < old)	/* carry flag, where are you when I
                                   need you? */
    {
      /* Overflow. */
      opt.downloaded_overflow = 1;
      opt.downloaded = ~((VERY_LONG_TYPE)0);
    }
}

/* Return non-zero if the downloaded amount of bytes exceeds the
   desired quota.  If quota is not set or if the amount overflowed, 0
   is returned. */
int
downloaded_exceeds_quota (void)
{
  if (!opt.quota)
    return 0;
  if (opt.downloaded_overflow)
    /* We don't really know.  (Wildly) assume not. */
    return 0;

  return opt.downloaded > opt.quota;
}

/* If opt.wait or opt.waitretry are specified, and if certain
   conditions are met, sleep the appropriate number of seconds.  See
   the documentation of --wait and --waitretry for more information.

   COUNT is the count of current retrieval, beginning with 1. */

void
sleep_between_retrievals (int count)
{
  static int first_retrieval = 1;

  if (!first_retrieval && (opt.wait || opt.waitretry))
    {
      if (opt.waitretry && count > 1)
	{
	  /* If opt.waitretry is specified and this is a retry, wait
	     for COUNT-1 number of seconds, or for opt.waitretry
	     seconds.  */
	  if (count <= opt.waitretry)
	    sleep (count - 1);
	  else
	    sleep (opt.waitretry);
	}
      else if (opt.wait)
	/* Otherwise, check if opt.wait is specified.  If so, sleep.  */
	sleep (opt.wait);
    }
  if (first_retrieval)
    first_retrieval = 0;
}
