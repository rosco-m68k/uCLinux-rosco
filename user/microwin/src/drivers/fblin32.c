/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 32bpp Linear Video Driver for Microwindows
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear32_init(PSD psd)
{
	if (!psd->size) {
		psd->size = psd->yres * psd->linelen;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen /= 4;
	}
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR32	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	if(gr_mode == MWMODE_XOR)
		addr[x + y * psd->linelen] ^= c;
	else
		addr[x + y * psd->linelen] = c;
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear32_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR32	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear32_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR32	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x1 + y * psd->linelen;
	if(gr_mode == MWMODE_XOR) {
		while(x1++ <= x2)
			*addr++ ^= c;
	} else
		//FIXME: memsetl(dst, c, x2-x1+1)?
		while(x1++ <= x2)
			*addr++ = c;
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear32_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR32	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += x + y1 * linelen;
	if(gr_mode == MWMODE_XOR)
		while(y1++ <= y2) {
			*addr ^= c;
			addr += linelen;
		}
	else
		while(y1++ <= y2) {
			*addr = c;
			addr += linelen;
		}
	DRAWOFF;
}

/* srccopy bitblt*/
static void
linear32_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst8, src8;
	ADDR32	dst = dstpsd->addr;
	ADDR32	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	dlinelen_minus_w4;
	int	slinelen_minus_w4;
#if ALPHABLEND
	unsigned int alpha;
#endif

	assert (dst != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (src != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst += dstx + dsty * dlinelen;
	src += srcx + srcy * slinelen;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	alpha = op & 0xff;

	src8 = (ADDR8)src;
	dst8 = (ADDR8)dst;
	dlinelen_minus_w4 = (dlinelen - w) * 4;
	slinelen_minus_w4 = (slinelen - w) * 4;
	while(--h >= 0) {
		for(i=0; i<w; ++i) {
			register unsigned long s = *src8++;
			register unsigned long d = *dst8;
			*dst8++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src8++;
			d = *dst8;
			*dst8++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src8;
			d = *dst8;
			*dst8 = (unsigned char)(((s - d)*alpha)>>8) + d;
			dst8 += 2;
			src8 += 2;
		}
		dst8 += dlinelen_minus_w4;
		src8 += slinelen_minus_w4;
	}
	DRAWOFF;
	return;
stdblit:
#endif
	/* copy from bottom up if dst in src rectangle*/
	/* memmove is used to handle x case*/
	if (srcy < dsty) {
		src += (h-1) * slinelen;
		dst += (h-1) * dlinelen;
		slinelen *= -1;
		dlinelen *= -1;
	}

	while(--h >= 0) {
#if 1
		/* a _fast_ memmove is a _must_ in this routine*/
		memmove(dst, src, w<<2);
		dst += dlinelen;
		src += slinelen;
#else
		for(i=0; i<w; ++i)
			*dst++ = *src++;
		dst += dlinelen - w;
		src += slinelen - w;
#endif
	}
	DRAWOFF;
}

SUBDRIVER fblinear32 = {
	linear32_init,
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	linear32_blit
};
