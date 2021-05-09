 /*
  * @(#) stdarg.h 1.2 91/11/30 21:10:39
  * 
  * Sample stdarg.h file for use with the unproto filter.
  * 
  * This file serves two purposes.
  * 
  * 1 - As an include file for use with ANSI-style C source that implements
  * variadic functions.
  * 
  * 2 - To configure the unproto filter itself. If the _VA_ALIST_ macro is
  * defined, its value will appear in the place of the "..." in argument
  * lists of variadic function *definitions* (not declarations).
  * 
  * Compilers that pass arguments via the stack can use the default code at the
  * end of this file (this usually applies for the VAX, MC68k and 80*86
  * architectures).
  * 
  * RISC-based systems often need special tricks. An example of the latter is
  * given for the SPARC architecture. Read your /usr/include/varargs.h for
  * more information.
  * 
  * You can use the varargs.c program provided with the unproto package to
  * verify that the stdarg.h file has been set up correctly.
  */

#ifndef __STDARG_H
#define __STDARG_H

typedef char *va_list;
#define va_start(ap, p)		(ap = (char *) (&(p)+1))
#define va_arg(ap, type)	((type *) (ap += sizeof(type)))[-1]
#define va_end(ap)

#endif /* __STDARG_H */
