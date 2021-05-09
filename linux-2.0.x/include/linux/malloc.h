#ifndef _LINUX_MALLOC_H
#define _LINUX_MALLOC_H

#include <linux/mm.h>

#ifdef DEBUG_KMALLOC

#undef kmalloc
#undef kfree

extern void * kmalloc(size_t size, int priority);
extern void kfree(void * obj);

extern void * kmalloc_flf(size_t size, int priority, char * file, int line, char * function);
extern void kfree_flf(void * obj, char * file, int line, char * function);
extern unsigned int ksize(void * obj);

#define kmalloc(size, priority) kmalloc_flf(size, priority, __FILE__, __LINE__, __FUNCTION__)
#define kfree(obj) kfree_flf(obj, __FILE__, __LINE__, __FUNCTION__)

#else /* !DEBUG_KMALLOC */

extern void * kmalloc(size_t size, int priority);
extern void kfree(void * obj);
extern unsigned int ksize(void * obj);

#endif /* !DEBUG_KMALLOC */

#define kfree_s(a,b) kfree(a)

#endif /* _LINUX_MALLOC_H */
