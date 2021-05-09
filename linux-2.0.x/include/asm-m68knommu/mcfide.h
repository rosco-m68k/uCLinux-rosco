/****************************************************************************/

/*
 *	mcfide.h -- ColdFire specific IDE support
 *
 *	(C) Copyright 2002, Greg Ungerer (gerg@snapgear.com)
 */

/****************************************************************************/
#ifndef	mcfide_h
#define	mcfide_h
/****************************************************************************/

#include <linux/config.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>

/*-------------------- SECUREEDGE MP3 hardware mappings --------------------*/
#ifdef CONFIG_SECUREEDGEMP3

static const unsigned int default_io_base[MAX_HWIFS]   = {0x30800000};
static const byte	  default_irqs[MAX_HWIFS]      = {29};
static const byte	  ide_hwif_to_major[MAX_HWIFS] = {IDE0_MAJOR};
#define	MCFSIM_LOCALCS	  MCFSIM_CSCR4

/* Replace standard IO functions for funky mapping of MP3 board */
#undef outb
#undef outb_p
#undef outw
#undef outw_p
#undef inb
#undef inb_p
#undef inw
#undef inw_p

#define outb    ide_outb
#define outb_p  ide_outb
#define	outw	ide_outw
#define	outw_p	ide_outw
#define outsw   ide_outsw
#define inb     ide_inb
#define inb_p   ide_inb
#define	inw	ide_inw
#define	inw_p	ide_inw
#define insw    ide_insw

#define	IDECTRL(a)		((a) ? ((a) + 0xe) : 0)
#define ADDR8_PTR(addr)		(((addr) & 0x1) ? (0x8000 + (addr) - 1) : (addr))
#define ADDR16_PTR(addr)	(addr)
#define SWAP8(w)		((((w) & 0xffff) << 8) | (((w) & 0xffff) >> 8))
#define SWAP16(w)		(w)

void ide_outb(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = (volatile unsigned short *) ADDR8_PTR(addr);
#if 0
	printk("ide_outb(addr=%x,val=%x)\n", (int) rp, (int) SWAP8(val));
#endif
	*rp = SWAP8(val);
}


void ide_outw(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = (volatile unsigned short *) ADDR16_PTR(addr);
#if 0
	printk("ide_outw(addr=%x,val=%x)\n", (int) rp, (int) SWAP16(val));
#endif
	*rp = SWAP16(val);
}

void ide_outsw(unsigned int addr, const void *vbuf, unsigned long len)
{
	volatile unsigned short	*rp, val;
	unsigned short   	*buf;

	buf = (unsigned short *) vbuf;
	rp = (volatile unsigned short *) ADDR16_PTR(addr);
#if 0
	printk("ide_outsw(addr=%x,len=%d)\n", (int) rp, (int) len);
#endif
	for (; (len > 0); len--) {
		val = *buf++;
		*rp = SWAP16(val);
	}
}

int ide_inb(unsigned int addr)
{
	volatile unsigned short	*rp, val;

	rp = (volatile unsigned short *) ADDR8_PTR(addr);
	val = *rp;
#if 0
	printk("ide_inb(addr=%x)=%x\n", (int) rp, (int) SWAP8(val));
#endif
	return(SWAP8(val));
}

int ide_inw(unsigned int addr)
{
        volatile unsigned short *rp, val;

        rp = (volatile unsigned short *) ADDR16_PTR(addr);
        val = *rp;
#if 0
	printk("ide_inw(addr=%x)=%x\n", (int) rp, (int) SWAP16(val));
#endif
        return(SWAP16(val));
}

void ide_insw(unsigned int addr, void *vbuf, unsigned long len)
{
	volatile unsigned short *rp;
	unsigned short          w, *buf;

	buf = (unsigned short *) vbuf;
	rp = (volatile unsigned short *) ADDR16_PTR(addr);
#if 0
	printk("ide_insw(addr=%x,len=%d)\n", (int) rp, (int) len);
#endif
	for (; (len > 0); len--) {
		w = *rp;
		*buf++ = SWAP16(w);
	}
}

#define	ACCESS_MODE_16BIT()
#define	ACCESS_MODE_8BIT()

static inline void ide_interruptenable(int irq)
{
	mcf_autovector(irq);
}

#define ide_intr_ack(i)

#endif
/*--------------------- eLIA/DISKtel hardware mappings ---------------------*/
#if defined(CONFIG_eLIA) || defined(CONFIG_DISKtel)

static const unsigned int default_io_base[MAX_HWIFS]   = {0x30c00000};
static const byte	  default_irqs[MAX_HWIFS]      = {29};
static const byte	  ide_hwif_to_major[MAX_HWIFS] = {IDE0_MAJOR};
#define	MCFSIM_LOCALCS	  MCFSIM_CSCR6

#define	IDECTRL(a)		((a) ? ((a) + 0xe) : 0)

/*
 *	8/16 bit acesses is controlled by flicking bits in the CS register.
 */
#define	ACCESS_MODE_16BIT()	\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_LOCALCS)) = 0x0080
#define	ACCESS_MODE_8BIT()	\
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_LOCALCS)) = 0x0040

static inline void ide_interruptenable(int irq)
{
	mcf_autovector(irq);
}

#define ide_intr_ack(i)

#endif
/*------------------------ M5249C3 hardware mappings -----------------------*/
#ifdef CONFIG_M5249C3

static const unsigned int default_io_base[MAX_HWIFS]   = {0x50000020};
static const byte	  default_irqs[MAX_HWIFS]      = {165};
static const byte	  ide_hwif_to_major[MAX_HWIFS] = {IDE0_MAJOR};

#define	IDECTRL(a)	0

#define	ADDR(a)		(((a) & 0xfffffff0) + (((a) & 0xf) << 1))

#define	SWAP8(w)	(w)
#define SWAP16(w)	((((w) & 0xffff) << 8) | (((w) & 0xffff) >> 8))

/* Replace standard IO functions for access to IDE channels */
#undef outb
#undef outb_p
#undef outw
#undef outw_p
#undef inb
#undef inb_p
#undef inw
#undef inw_p

#define outb    ide_outb
#define outb_p  ide_outb
#define	outw	ide_outw
#define	outw_p	ide_outw
#define outsw   ide_outsw
#define inb     ide_inb
#define inb_p   ide_inb
#define	inw	ide_inw
#define	inw_p	ide_inw
#define insw    ide_insw

void ide_outb(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = (volatile unsigned short *) ADDR(addr);
#if 0
	printk("ide_outb(addr=%x,val=%x) [%x,%x]\n", addr, val,
		(int) rp, (int) SWAP8(val));
#endif
	*rp = SWAP8(val);
}


void ide_outw(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = (volatile unsigned short *) ADDR(addr);
#if 0
	printk("ide_outw(addr=%x,val=%x)\n", (int) rp, (int) SWAP16(val));
#endif
	*rp = SWAP16(val);
}

void ide_outsw(unsigned int addr, const void *vbuf, unsigned long len)
{
	volatile unsigned short	*rp, val;
	unsigned short   	*buf;

	buf = (unsigned short *) vbuf;
	rp = (volatile unsigned short *) ADDR(addr);
#if 0
	printk("ide_outsw(addr=%x,len=%d)\n", (int) rp, (int) len);
#endif
	for (; (len > 0); len--) {
		val = *buf++;
		*rp = SWAP16(val);
	}
}

int ide_inb(unsigned int addr)
{
	volatile unsigned short	*rp, val;

	rp = (volatile unsigned short *) ADDR(addr);
	val = *rp;
#if 0
	printk("ide_inb(addr=%x)=%x [%x=%x]\n", addr, (int) SWAP8(val),
		(int) rp, (int) val);
#endif
	return(SWAP8(val));
}

int ide_inw(unsigned int addr)
{
        volatile unsigned short *rp, val;

        rp = (volatile unsigned short *) ADDR(addr);
        val = *rp;
#if 0
	printk("ide_inw(addr=%x)=%x\n", (int) rp, (int) SWAP16(val));
#endif
        return(SWAP16(val));
}

void ide_insw(unsigned int addr, void *vbuf, unsigned long len)
{
	volatile unsigned short *rp;
	unsigned short          w, *buf;

	buf = (unsigned short *) vbuf;
	rp = (volatile unsigned short *) ADDR(addr);
#if 0
	printk("ide_insw(addr=%x,len=%d)\n", (int) rp, (int) len);
#endif
	for (; (len > 0); len--) {
		w = *rp;
		*buf++ = SWAP16(w);
	}
}


/* No crazy 8/16 bit access magic required */
#define	ACCESS_MODE_16BIT()
#define	ACCESS_MODE_8BIT()

static inline void ide_interruptenable(int irq)
{
	/* Enable interrupts for GPIO5 */
	*((volatile unsigned long *) 0x800000c4) |= 0x00000020;

	/* Enable interrupt level for GPIO5 - VEC37 */
	*((volatile unsigned long *) 0x80000150) |= 0x00200000;
}

static inline void ide_intr_ack(int irq)
{
	/* Clear interrupts for GPIO5 */
	*((volatile unsigned long *) 0x800000c0) = 0x00002020;
}

#endif
/*--------------------------------------------------------------------------*/

inline unsigned long probe_irq_on(void)		{ return(0); }
inline int probe_irq_off(unsigned long v)	{ return(0); }
inline void enable_irq(unsigned int v)		{ }
inline void disable_irq(unsigned int v)		{ }
#define	probe_irq_off	

/*
 *	Adjust drive ID for byte/word swapping and endianess.
 */
static inline void ide_fix_driveid(struct hd_driveid *id)
{
	int i, n;
	unsigned short *wp = (unsigned short *) id;
	int avoid[] = {49, 51, 52, 59, -1 }; /* do not swap these words */

	/* Need to byte swap shorts, but not char fields */
	for (i = n = 0; i < sizeof(*id) / sizeof(*wp); i++, wp++) {
		if (avoid[n] == i) {
			n++;
			continue;
		}
		*wp = ((*wp & 0xff) << 8) | ((*wp >> 8) & 0xff);
	}

	/* Have to word swap the one 32 bit field */
	id->lba_capacity = ((id->lba_capacity & 0xffff) << 16) |
		((id->lba_capacity >> 16) & 0xffff);
}

/*
 *	Need slow probes (most ColdFire boards boot fast).
 */
#define	CONFIG_IDE_SLOWPROBE	1

/****************************************************************************/
#endif	/* mcfide_h */
