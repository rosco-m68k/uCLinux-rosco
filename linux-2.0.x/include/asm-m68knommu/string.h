#ifndef _M68K_STRING_H_
#define _M68K_STRING_H_

#include <linux/config.h>
#include <asm/page.h>

#define __HAVE_ARCH_STRCPY
extern inline char * strcpy(char * dest,const char *src)
{
  char *xdest = dest;

  __asm__ __volatile__
       ("1:\tmoveb %1@+,%0@+\n\t"
        "jne 1b"
	: "=a" (dest), "=a" (src)
        : "0" (dest), "1" (src) : "memory");
  return xdest;
}

#define __HAVE_ARCH_STRNCPY
extern inline char * strncpy(char *dest, const char *src, __kernel_size_t n)
{
  char *xdest = dest;

  if (n == 0)
    return xdest;

  __asm__ __volatile__
       ("1:\tmoveb %1@+,%0@+\n\t"
	"jeq 2f\n\t"
        "subql #1,%2\n\t"
        "jne 1b\n\t"
        "2:"
        : "=a" (dest), "=a" (src), "=d" (n)
        : "0" (dest), "1" (src), "2" (n)
        : "memory");
  return xdest;
}

/*
 *	The following functions are defined in the arch library.
 */
#define __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMSET

#endif /* _M68K_STRING_H_ */
