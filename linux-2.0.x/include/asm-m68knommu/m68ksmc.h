/****************************************************************************/

/*
 *	m68ksmc.h -- SMC ethernet support for M68k environments.
 */

/****************************************************************************/
#ifndef	m68ksmc_h
#define	m68ksmc_h
/****************************************************************************/

#include <linux/config.h>

/*
 * swap functions are sometimes needed to interface little-endian hardware
 */
static inline unsigned short _swapw(volatile unsigned short v)
{
    return ((v << 8) | (v >> 8));
}

static inline unsigned int _swapl(volatile unsigned long v)
{
    return ((v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24));
}


/* There is no difference between I/O and memory on 68k, these are the same */
#define inb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) ((addr)));\
     __v; })
#define inw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); \
       _swapw(__v); })
#define inl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); _swapl(__v); })

#define outb(b,addr) ((*(volatile unsigned char *) ((addr))) = (b))
#define outw(b,addr) ((*(volatile unsigned short *) (addr)) = (_swapw(b)))
#define outl(b,addr) ((*(volatile unsigned int *) (addr)) = (_swapl(b)))

/* FIXME: these need to be optimized.  Watch out for byte swapping, they
 * are used mostly for Intel devices... */
#define outsw(addr,buf,len) \
    ({ unsigned short * __p = (unsigned short *)(buf); \
       unsigned short * __e = (unsigned short *)(__p) + (len); \
       while (__p < __e) { \
	  *(volatile unsigned short *)(addr) = *__p++;\
       } \
     })

#define insw(addr,buf,len) \
    ({ unsigned short * __p = (unsigned short *)(buf); \
       unsigned short * __e = (unsigned short *)(__p) + (len); \
       while (__p < __e) { \
          *(__p++) = *(volatile unsigned short *)(addr); \
       } \
     })

 
// Macros to replace x86 input and output macros.  these were taken from io.h
// in a coldfire ifdef but should work OK for the m68360 we are using here

static inline void outsl(unsigned int addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*ap = *bp++;
}

static inline void insb(unsigned int addr, void *buf, int len)
{
	volatile unsigned char *ap = (volatile unsigned char *) addr;
	unsigned char *bp = (unsigned char *) buf;
	while (len--)
		*bp++ = *ap;
}

static inline void insl(unsigned int addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*bp++ = *ap;
}
/****************************************************************************/
#endif	/* m68ksmc_h */
