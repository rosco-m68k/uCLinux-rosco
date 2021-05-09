/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * 8bpp Linear Video Driver for Microwindows
 * 	00/01/26 added alpha blending with lookup tables (64k total)
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */
/*#define NDEBUG*/
#include <assert.h>

/* We want to do string copying fast, so inline assembly if possible */
#ifndef __OPTIMIZE__
#define __OPTIMIZE__
#endif
#include <string.h>
#include <stdlib.h>

#include "device.h"
#include "fb.h"

#if ALPHABLEND
/*
 * Alpha lookup tables for 256 color palette systems
 * A 5 bit alpha value is used to keep tables smaller.
 *
 * Two tables are created.  The first, alpha_to_rgb contains 15 bit RGB 
 * values for each alpha value for each color: 32*256 short words.
 * RGB values can then be blended.  The second, rgb_to_palindex contains
 * the closest color (palette index) for each of the 5-bit
 * R, G, and B values: 32*32*32 bytes.
 */
static unsigned short *alpha_to_rgb = NULL;
static unsigned char  *rgb_to_palindex = NULL;
void init_alpha_lookup(void);
#endif

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear8_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear8_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

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
linear8_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return addr[x + y * psd->linelen];
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear8_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

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
		memset(addr, c, x2 - x1 + 1);
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear8_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
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
linear8_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst;
	ADDR8	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
#if ALPHABLEND
	unsigned int srcalpha, dstalpha;
#endif

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst = dstpsd->addr + dstx + dsty * dlinelen;
	src = srcpsd->addr + srcx + srcy * slinelen;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	srcalpha = op & 0xff;

	/* FIXME create lookup table after palette is stabilized...*/
	if(!rgb_to_palindex || !alpha_to_rgb) {
		init_alpha_lookup();
		if(!rgb_to_palindex || !alpha_to_rgb)
			goto stdblit;
	}

	/* Create 5 bit alpha value index for 256 color indexing*/

	/* destination alpha is (1 - source) alpha*/
	dstalpha = ((srcalpha>>3) ^ 31) << 8;
	srcalpha = (srcalpha>>3) << 8;

	while(--h >= 0) {
	    int	i;
	    for(i=0; i<w; ++i) {
		/* Get source RGB555 value for source alpha value*/
		unsigned short s = alpha_to_rgb[srcalpha + *src++];

		/* Get destination RGB555 value for dest alpha value*/
		unsigned short d = alpha_to_rgb[dstalpha + *dst];

		/* Add RGB values together and get closest palette index to it*/
		*dst++ = rgb_to_palindex[s + d];
	    }
	    dst += dlinelen - w;
	    src += slinelen - w;
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
		/* a _fast_ memcpy is a _must_ in this routine*/
		memmove(dst, src, w);
		dst += dlinelen;
		src += slinelen;
	}
	DRAWOFF;
}

#if ALPHABLEND
void
init_alpha_lookup(void)
{
	int	i, a;
	int	r, g, b;
	extern MWPALENTRY gr_palette[256];

	if(!alpha_to_rgb)
		alpha_to_rgb = (unsigned short *)malloc(
			sizeof(unsigned short)*32*256);
	if(!rgb_to_palindex)
		rgb_to_palindex = (unsigned char *)malloc(
			sizeof(unsigned char)*32*32*32);
	if(!rgb_to_palindex || !alpha_to_rgb)
		return;

	/*
	 * Precompute alpha to rgb lookup by premultiplying
	 * each palette rgb value by each possible alpha
	 * and storing it as RGB555.
	 */
	for(i=0; i<256; ++i) {
		MWPALENTRY *p = &gr_palette[i];
		for(a=0; a<32; ++a) {
			alpha_to_rgb[(a<<8)+i] =
				(((p->r * a / 31)>>3) << 10) |
				(((p->g * a / 31)>>3) << 5) |
				 ((p->b * a / 31)>>3);
		}
	}

	/*
	 * Precompute RGB555 to palette index table by
	 * finding the nearest palette index for all RGB555 colors.
	 */
	for(r=0; r<32; ++r) {
		for(g=0; g<32; ++g)
			for(b=0; b<32; ++b)
				rgb_to_palindex[ (r<<10)|(g<<5)|b] =
					GdFindNearestColor(gr_palette, 256,
						MWRGB(r<<3, g<<3, b<<3));
	}
}
#endif

SUBDRIVER fblinear8 = {
	linear8_init,
	linear8_drawpixel,
	linear8_readpixel,
	linear8_drawhorzline,
	linear8_drawvertline,
	gen_fillrect,
	linear8_blit
};
