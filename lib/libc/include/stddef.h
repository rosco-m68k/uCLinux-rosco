/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */
/* We don't care, ignore GCC's __need hackery */

#undef __need_wchar_t
#undef __need_size_t
#undef __need_ptrdiff_t
#undef __need_NULL

/* Fact is these are _normal_ */
#if 1	/* __BCC__ */	/* Only for Bcc 8086/80386 */

#ifndef __STDDEF_H
#define __STDDEF_H

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#ifndef NULL
#define NULL 0
#endif

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif /* __STDDEF_H */
#endif /* __AS386_16__ */
