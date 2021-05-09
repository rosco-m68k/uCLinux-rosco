#ifndef _MWTYPES_H
#define _MWTYPES_H
/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Exported Microwindows engine typedefs and defines
 */

/* builtin font std names*/
#define MWFONT_SYSTEM_VAR	"System"	/* winSystem 14x16 (ansi)*/
#define MWFONT_GUI_VAR		"Helvetica"	/* winMSSansSerif 11x13 (ansi)*/
#define MWFONT_OEM_FIXED	"Terminal"	/* rom8x16 (oem)*/
#define MWFONT_SYSTEM_FIXED	"SystemFixed"	/* X6x13 (should be ansi)*/

/* Text/GetTextSize encoding flags*/
#define MWTF_ASCII	0x0000	/* 8 bit packing, ascii*/
#define MWTF_UTF8	0x0001	/* 8 bit packing, utf8*/
#define MWTF_UC16	0x0002	/* 16 bit packing, unicode 16*/
#define MWTF_UC32	0x0004	/* 32 bit packing, unicode 32*/
#define MWTF_PACKMASK	0x0007	/* packing mask*/

/* Text alignment flags*/
#define MWTF_TOP	0x0010	/* align on top*/
#define MWTF_BASELINE	0x0020	/* align on baseline*/
#define MWTF_BOTTOM	0x0040	/* align on bottom*/

/* SetFontAttr flags*/
#define MWTF_KERNING	0x1000	/* font kerning*/
#define MWTF_ANTIALIAS	0x2000	/* antialiased output*/
#define MWTF_UNDERLINE	0x4000	/* draw underline*/

/* Drawing modes*/
#define	MWMODE_SET	0	/* draw pixels as given (default) */
#define	MWMODE_XOR	1	/* draw pixels using XOR */
#define	MWMODE_OR	2	/* draw pixels using OR (notimp)*/
#define	MWMODE_AND	3	/* draw pixels using AND (notimp)*/
#define MWMODE_MAX	3

/* Mouse button bits*/
#define MWBUTTON_L	04
#define MWBUTTON_M	02
#define MWBUTTON_R	01

/* Color defines*/
#define MWRGB(r,g,b)	((MWCOLORVAL)(((unsigned char)(r)|\
				((unsigned short)((unsigned char)(g))<<8))|\
				(((unsigned long)(unsigned char)(b))<<16)))
#define MWF_PALINDEX	0x01000000
#define MWPALINDEX(x)	((MWCOLORVAL)MWF_PALINDEX | (x))

/* 
 * ROP blitter opcodes
 */
#define MWROP_EXTENSION		0xff000000L	/* rop extension bits*/

/* copy src -> dst*/
#define MWROP_SRCCOPY		0x00000000L

/* copy src -> dst except for transparent color in src*/
#define MWROP_SRCTRANSCOPY	0x01000000L

/* alpha blend src -> dst with constant alpha, alpha value in low 8 bits*/
#define MWROP_BLENDCONSTANT	0x02000000L

/* alpha blend fg/bg color -> dst with src as alpha channel*/
#define MWROP_BLENDFGBG		0x03000000L

/* alpha blend src -> dst with separate per pixel alpha channel*/
#define MWROP_BLENDCHANNEL	0x04000000L

/* stretch src -> dst*/
#define MWROP_STRETCH		0x05000000L

#define MWROP_SRCAND		0x06000000L
#define MWROP_SRCINVERT		0x07000000L
#define MWROP_BLACKNESS     	0x08000000L

/* 
 * Pixel formats
 * Note the two pseudo pixel formats are never returned by display drivers,
 * but rather used as a data structure type in GrArea.  The other
 * types are both returned by display drivers and used as pixel packing
 * specifiers.
 */
#define MWPF_RGB	   0	/* pseudo, convert from packed 32 bit RGB*/
#define MWPF_PIXELVAL	   1	/* pseudo, no convert from packed PIXELVAL*/
#define MWPF_PALETTE	   2	/* pixel is packed 8 bits 1, 4 or 8 pal index*/
#define MWPF_TRUECOLOR0888 3	/* pixel is packed 32 bits 8/8/8 truecolor*/
#define MWPF_TRUECOLOR888  4	/* pixel is packed 24 bits 8/8/8 truecolor*/
#define MWPF_TRUECOLOR565  5	/* pixel is packed 16 bits 5/6/5 truecolor*/
#define MWPF_TRUECOLOR555  6	/* pixel is packed 16 bits 5/5/5 truecolor*/
#define MWPF_TRUECOLOR332  7	/* pixel is packed 8 bits 3/3/2 truecolor*/

/*
 * MWPIXELVAL definition: changes based on target system
 * Set using -DMWPIXEL_FORMAT=MWPF_XXX
 *
 * For the Nano-X server, it is important to use the correct MWPF_* value
 * for the MWPIXEL_FORMAT macro in order to match the hardware,
 * while the Nano-X clients that includes this file can get away with
 * a default pixel format of 24-bit color as the client will either:
 *    1) Use the MWPF_PIXELVAL native format when calling GrReadArea, in
 *       which case we have to have enough spare room to hold 32-bit
 *       pixlevalues (hence the default MWPF_TRUECOLOR0888 format), or
 *    2) Will use some other PF_* format, in which case the application
 *       is well aware of which pixel-format it uses and can avoid the
 *       device specific RGB2PIXEL and use RGB2PIXEL565 etc. instead,
 *       and specifiy the pixel fomar as MWPF_TRUECOLOR565 etc. when
 *       calling the GrArea function(s).
 */
#ifndef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_TRUECOLOR0888
#endif

#if defined(__AS386_16__)
/* Force 8 bit palettized display for ELKS*/
#undef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_PALETTE
#endif

#if MWPIXEL_FORMAT == MWPF_TRUECOLOR565
typedef unsigned short MWPIXELVAL;
#else
  #if MWPIXEL_FORMAT == MWPF_TRUECOLOR332
  typedef unsigned char MWPIXELVAL;
  #else
    #if MWPIXEL_FORMAT == MWPF_PALETTE
    typedef unsigned char MWPIXELVAL;
    #else
      typedef unsigned long MWPIXELVAL;
    #endif
  #endif
#endif

/*
 * Type definitions
 */
typedef int		MWCOORD;	/* device coordinates*/
typedef int		MWBOOL;		/* boolean value*/
typedef unsigned char	MWUCHAR;	/* unsigned char*/
typedef unsigned long	MWCOLORVAL;	/* device-independent color value*/
typedef unsigned short	MWIMAGEBITS;	/* bitmap image unit size*/
typedef unsigned long	MWTIMEOUT;	/* timeout value */

/* dbl linked list data structure*/
typedef struct _mwlist {		/* LIST must be first decl in struct*/
	struct _mwlist *next;		/* next item*/
	struct _mwlist *prev;		/* previous item*/
} MWLIST, *PMWLIST;

/* dbl linked list head data structure*/
typedef struct _mwlisthead {
	struct _mwlist *head;		/* first item*/
	struct _mwlist *tail;		/* last item*/
} MWLISTHEAD, *PMWLISTHEAD;

/* GetScreenInfo structure*/
typedef struct {
	MWCOORD  rows;		/* number of rows on screen */
	MWCOORD  cols;		/* number of columns on screen */
	int 	 xdpcm;		/* dots/centimeter in x direction */
	int 	 ydpcm;		/* dots/centimeter in y direction */
	int	 planes;	/* hw # planes*/
	int	 bpp;		/* hw bpp*/
	long	 ncolors;	/* hw number of colors supported*/
	int 	 fonts;		/* number of built-in fonts */
	int 	 buttons;	/* buttons which are implemented */
	int	 modifiers;	/* modifiers which are implemented */
	int	 pixtype;	/* format of pixel value*/
	MWCOORD	 xpos;		/* current x mouse position*/
	MWCOORD	 ypos;		/* current y mouse position*/

/* items below are get/set by the window manager and not used internally*/
	int	vs_width;	/* virtual screen width/height*/
	int	vs_height;
	int	ws_width;	/* workspace width/height*/
	int	ws_height;
} MWSCREENINFO, *PMWSCREENINFO;

/* GetFontInfo structure*/
typedef struct {
	int 	maxwidth;	/* maximum width of any char */
	int 	height;		/* height of font in pixels*/
	int 	baseline;	/* baseline (ascent) of font */
	int	firstchar;	/* first character in font*/
	int	lastchar;	/* last character in font*/
	MWBOOL	fixed;		/* TRUE if font is fixed width */
	MWUCHAR	widths[256];	/* table of character widths */
} MWFONTINFO, *PMWFONTINFO;

/* logical font descriptor*/

/* font classes - used internally*/
#define MWLF_CLASS_BUILTIN	1	/* Builtin fonts (bitmaps) */
#define MWLF_CLASS_FREETYPE	2	/* FreeType fonts in TT format */
#define MWLF_CLASS_T1LIB	3	/* T1LIB outlined Adobe Type 1 fonts */
#define MWLF_CLASS_ANY		4	/* Any font*/

#define MWLF_FACESIZE		64	/* max facename size*/

/* font type selection - lfOutPrecision*/
#define MWLF_TYPE_DEFAULT	0	/* any font*/
#define MWLF_TYPE_SCALED	4	/* outlined font (tt or adobe)*/
#define MWLF_TYPE_RASTER	5	/* raster only*/
#define MWLF_TYPE_TRUETYPE	7	/* truetype only*/
#define MWLF_TYPE_ADOBE		10	/* adobe type 1 only*/

/* font weights - lfWeight*/
#define MWLF_WEIGHT_DEFAULT	0	/* any weight*/
#define MWLF_WEIGHT_THIN	100	/* thin*/
#define MWLF_WEIGHT_EXTRALIGHT	200
#define MWLF_WEIGHT_LIGHT	300	/* light */
#define MWLF_WEIGHT_NORMAL	400	/* regular*/
#define MWLF_WEIGHT_REGULAR	400
#define MWLF_WEIGHT_MEDIUM	500	/* medium */
#define MWLF_WEIGHT_DEMIBOLD	600
#define MWLF_WEIGHT_BOLD	700	/* bold*/
#define MWLF_WEIGTH_EXTRABOLD	800
#define MWLF_WEIGHT_BLACK	900	/* black */

/* font charset - lfCharSet*/
#define MWLF_CHARSET_ANSI	0	/* win32 ansi*/
#define MWLF_CHARSET_DEFAULT	1	/* any charset*/
#define MWLF_CHARSET_UNICODE	254	/* unicode*/
#define MWLF_CHARSET_OEM	255	/* local hw*/

/* font pitch - lfPitch */
#define MWLF_PITCH_DEFAULT		0	/* any pitch */
#define MWLF_PITCH_ULTRACONDENSED	10
#define MWLF_PITCH_EXTRACONDENSED	20
#define MWLF_PITCH_CONDENSED		30
#define MWLF_PITCH_SEMICONDENSED	40
#define MWLF_PITCH_NORMAL		50
#define MWLF_PITCH_SEMIEXPANDED		60
#define MWLF_PITCH_EXPANDED		70
#define MWLF_PITCH_EXTRAEXPANDED	80
#define MWLF_PITCH_ULTRAEXPANDED	90

/* flags for the GdAddFont function */
#define MWLF_FLAGS_ALIAS	1

typedef struct {
	long	lfHeight;		/* desired height in pixels*/
	long	lfWidth;		/* desired width in pixels or 0*/
	long	lfEscapement;		/* rotation in tenths of degree*/
	long	lfOrientation;		/* not used*/
	long	lfWeight;		/* font weight*/
	MWUCHAR	lfItalic;		/* =1 for italic */
	MWUCHAR	lfUnderline;		/* =1 for underline */
	MWUCHAR	lfStrikeOut;		/* not used*/
	MWUCHAR	lfCharSet;		/* font character set*/
	MWUCHAR	lfOutPrecision;		/* font type selection*/
	MWUCHAR	lfClipPrecision;	/* not used*/
	MWUCHAR	lfQuality;		/* not used*/

	/* the following differs from windows font model*/
	MWUCHAR	lfRoman;		/* =1 for Roman letters (upright) */
	MWUCHAR	lfSerif;		/* =1 for Serifed font */
	MWUCHAR	lfSansSerif;		/* =1 for Sans-serif font */
	MWUCHAR	lfModern;		/* =1 for Modern font */
	MWUCHAR	lfMonospace;		/* =1 for Monospaced font */
	MWUCHAR	lfProportional;		/* =1 for Proportional font */
	MWUCHAR	lfOblique;		/* =1 for Oblique (kind of Italic) */
	MWUCHAR	lfSmallCaps;		/* =1 for small caps */
	MWUCHAR	lfPitch;		/* font pitch (width) */

	char	lfFaceName[MWLF_FACESIZE];	/* font name, may be aliased*/
} MWLOGFONT, *PMWLOGFONT;

/*
 * Macros to initialize the MWLOGFONT structure to the most common defaults
 * needed by application programs and the nano-X server program.
 */

#define MWLF_Clear(lf)					\
	do {						\
		(lf)->lfHeight = 0;			\
		(lf)->lfWidth = 0;			\
		(lf)->lfEscapement = 0;			\
		(lf)->lfOrientation = 0;		\
		(lf)->lfWeight = MWLF_WEIGHT_REGULAR;	\
		(lf)->lfPitch = 0;			\
		(lf)->lfItalic = 0;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 0;			\
		(lf)->lfSerif = 0;			\
		(lf)->lfSansSerif = 0;			\
		(lf)->lfModern = 0;			\
		(lf)->lfMonospace = 0;			\
		(lf)->lfProportional = 0;		\
		(lf)->lfSmallCaps = 0;			\
		(lf)->lfUnderline = 0;			\
		(lf)->lfStrikeOut = 0;			\
		(lf)->lfCharSet = 0;			\
		(lf)->lfOutPrecision = 0;		\
		(lf)->lfClipPrecision = 0;		\
		(lf)->lfQuality = 0;			\
		(lf)->lfFaceName[0] = '\0';		\
	} while (0)

#define MWLF_SetBold(lf)				\
	do {						\
		(lf)->lfWeight = MWLF_WEIGHT_BOLD;	\
	} while (0)

#define MWLF_SetRegular(lf)				\
	do {						\
		(lf)->lfWeight = MWLF_WEIGHT_REGULAR;	\
	} while (0)

#define MWLF_SetItalics(lf)				\
	do {						\
		(lf)->lfItalic = 1;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 0;			\
	} while (0)

#define MWLF_SetRoman(lf)				\
	do {						\
		(lf)->lfItalic = 0;			\
		(lf)->lfOblique = 0;			\
		(lf)->lfRoman = 1;			\
	} while (0)

/*
 * Rectangle and point structures.
 * These structures are "inherited" in wingdi.h for
 * the Win32 RECT and POINT structures, so they must match
 * Microsoft's definition.
 */

/* MWPOINT used in GdPoly, GdFillPoly*/
typedef struct {
	MWCOORD x;
	MWCOORD y;
} MWPOINT;

/* MWRECT used in region routines*/
typedef struct {
	MWCOORD	left;
	MWCOORD	top;
	MWCOORD	right;
	MWCOORD	bottom;
} MWRECT;

/* dynamically allocated multi-rectangle clipping region*/
typedef struct {
	int	size;		/* malloc'd # of rectangles*/
	int	numRects;	/* # rectangles in use*/
	int	type; 		/* region type*/
	MWRECT *rects;		/* rectangle array*/
	MWRECT	extents;	/* bounding box of region*/
} MWCLIPREGION;

/* region types */
#define MWREGION_ERROR		0
#define MWREGION_NULL		1
#define MWREGION_SIMPLE		2
#define MWREGION_COMPLEX	3

/* GdRectInRegion return codes*/
#define MWRECT_OUT	0	/* rectangle not in region*/
#define MWRECT_ALLIN	1	/* rectangle all in region*/
#define MWRECT_PARTIN	2	/* rectangle partly in region*/

/* GdAllocPolyRegion types*/
#define MWPOLY_EVENODD		1
#define MWPOLY_WINDING		2

/* In-core color palette structure*/
typedef struct {
	MWUCHAR	r;
	MWUCHAR	g;
	MWUCHAR	b;
} MWPALENTRY;

/* In-core mono and color image structure*/
#define MWIMAGE_UPSIDEDOWN	01	/* compression flag: upside down image*/
#define MWIMAGE_BGR		00	/* compression flag: BGR byte order*/
#define MWIMAGE_RGB		02	/* compression flag: RGB not BGR bytes*/

typedef struct {
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		pitch;		/* bytes per line*/
	int		bytesperpixel;	/* bytes per pixel*/
	int		compression;	/* compression algorithm*/
	int		palsize;	/* palette size*/
	long		transcolor;	/* transparent color or -1 if none*/
	MWPALENTRY *	palette;	/* palette*/
	MWUCHAR *	imagebits;	/* image bits (dword right aligned)*/
} MWIMAGEHDR, *PMWIMAGEHDR;

/* image information structure - returned by GdGetImageInfo*/
typedef struct {
	int		id;		/* image id*/
	int		width;		/* image width in pixels*/
	int		height;		/* image height in pixels*/
	int		planes;		/* # image planes*/
	int		bpp;		/* bits per pixel (1, 4 or 8)*/
	int		pitch;		/* bytes per line*/
	int		bytesperpixel;	/* bytes per pixel*/
	int		compression;	/* compression algorithm*/
	int		palsize;	/* palette size*/
	MWPALENTRY 	palette[256];	/* palette*/
} MWIMAGEINFO, *PMWIMAGEINFO;

#define	MWMAX_CURSOR_SIZE 16		/* maximum cursor x and y size*/

/* In-core software cursor structure*/
typedef struct {
	int		width;			/* cursor width in pixels*/
	int		height;			/* cursor height in pixels*/
	MWCOORD		hotx;			/* relative x pos of hot spot*/
	MWCOORD		hoty;			/* relative y pos of hot spot*/
	MWCOLORVAL	fgcolor;		/* foreground color*/
	MWCOLORVAL	bgcolor;		/* background color*/
	MWIMAGEBITS	image[MWMAX_CURSOR_SIZE];/* cursor image bits*/
	MWIMAGEBITS	mask[MWMAX_CURSOR_SIZE];/* cursor mask bits*/
} MWCURSOR, *PMWCURSOR;

typedef struct _mwfont *	PMWFONT;

/* outline and filled arc and pie types*/
#define MWARC		0x0001	/* arc*/
#define MWOUTLINE	0x0002
#define MWARCOUTLINE	0x0003	/* arc + outline*/
#define MWPIE		0x0004	/* pie (filled)*/
#define MWELLIPSE	0x0008	/* ellipse outline*/
#define MWELLIPSEFILL	0x0010	/* ellipse filled*/

#ifdef MWINCLUDECOLORS
/*
 * Common colors - note any color including these may not be
 * available on palettized systems, and the system will
 * then use the nearest color already in the system palette,
 * or allocate a new color entry.
 * These colors are the first 16 entries in the std palette,
 * and are written to the system palette if writable.
 */
#define BLACK		MWRGB( 0  , 0  , 0   )
#define BLUE		MWRGB( 0  , 0  , 128 )
#define GREEN		MWRGB( 0  , 128, 0   )
#define CYAN		MWRGB( 0  , 128, 128 )
#define RED		MWRGB( 128, 0  , 0   )
#define MAGENTA		MWRGB( 128, 0  , 128 )
#define BROWN		MWRGB( 128, 64 , 0   )
#define LTGRAY		MWRGB( 192, 192, 192 )
#define GRAY		MWRGB( 128, 128, 128 )
#define LTBLUE		MWRGB( 0  , 0  , 255 )
#define LTGREEN		MWRGB( 0  , 255, 0   )
#define LTCYAN		MWRGB( 0  , 255, 255 )
#define LTRED		MWRGB( 255, 0  , 0   )
#define LTMAGENTA	MWRGB( 255, 0  , 255 )
#define YELLOW		MWRGB( 255, 255, 0   )
#define WHITE		MWRGB( 255, 255, 255 )

/* other common colors*/
#define DKGRAY		MWRGB( 32,  32,  32)
#endif /* MWINCLUDECOLORS*/

/* Keyboard values*/
typedef unsigned short	MWKEY;
typedef unsigned char	MWSCANCODE;

#define MWKEY_UNKNOWN		0
/* Following special control keysyms are mapped to ASCII*/
#define MWKEY_BACKSPACE		8
#define MWKEY_TAB		9
#define MWKEY_ENTER		13
#define MWKEY_ESCAPE		27
/* Keysyms from 32-126 are mapped to ASCII*/

#define MWKEY_NONASCII_MASK	0xFF00
/* Following keysyms are mapped to private use portion of Unicode-16*/
/* arrows + home/end pad*/
#define MWKEY_FIRST		0xF800
#define MWKEY_LEFT		0xF800
#define MWKEY_RIGHT		0xF801
#define MWKEY_UP		0xF802
#define MWKEY_DOWN		0xF803
#define MWKEY_INSERT		0xF804
#define MWKEY_DELETE		0xF805
#define MWKEY_HOME		0xF806
#define MWKEY_END		0xF807
#define MWKEY_PAGEUP		0xF808
#define MWKEY_PAGEDOWN		0xF809

/* Numeric keypad*/
#define MWKEY_KP0		0xF80A
#define MWKEY_KP1		0xF80B
#define MWKEY_KP2		0xF80C
#define MWKEY_KP3		0xF80D
#define MWKEY_KP4		0xF80E
#define MWKEY_KP5		0xF80F
#define MWKEY_KP6		0xF810
#define MWKEY_KP7		0xF811
#define MWKEY_KP8		0xF812
#define MWKEY_KP9		0xF813
#define MWKEY_KP_PERIOD		0xF814
#define MWKEY_KP_DIVIDE		0xF815
#define MWKEY_KP_MULTIPLY	0xF816
#define MWKEY_KP_MINUS		0xF817
#define MWKEY_KP_PLUS		0xF818
#define MWKEY_KP_ENTER		0xF819
#define MWKEY_KP_EQUALS		0xF81A

/* Function keys */
#define MWKEY_F1		0xF81B
#define MWKEY_F2		0xF81C
#define MWKEY_F3		0xF81D
#define MWKEY_F4		0xF81E
#define MWKEY_F5		0xF81F
#define MWKEY_F6		0xF820
#define MWKEY_F7		0xF821
#define MWKEY_F8		0xF822
#define MWKEY_F9		0xF823
#define MWKEY_F10		0xF824
#define MWKEY_F11		0xF825
#define MWKEY_F12		0xF827

/* Key state modifier keys*/
#define MWKEY_NUMLOCK		0xF828
#define MWKEY_CAPSLOCK		0xF829
#define MWKEY_SCROLLOCK		0xF82A
#define MWKEY_LSHIFT		0xF82B
#define MWKEY_RSHIFT		0xF82C
#define MWKEY_LCTRL		0xF82D
#define MWKEY_RCTRL		0xF82E
#define MWKEY_LALT		0xF82F
#define MWKEY_RALT		0xF830
#define MWKEY_LMETA		0xF831
#define MWKEY_RMETA		0xF832
#define MWKEY_ALTGR		0xF833

/* Misc function keys*/
#define MWKEY_PRINT		0xF834
#define MWKEY_SYSREQ		0xF835
#define MWKEY_PAUSE		0xF836
#define MWKEY_BREAK		0xF837
#define MWKEY_QUIT		0xF838	/* virtual key*/
#define MWKEY_MENU		0xF839	/* virtual key*/
#define MWKEY_REDRAW		0xF83A	/* virtual key*/

/* Handheld function keys*/
#define MWKEY_RECORD		0xF840
#define MWKEY_PLAY		0xF841
#define MWKEY_CONTRAST		0xF842
#define MWKEY_BRIGHTNESS	0xF843
#define MWKEY_SELECTUP		0xF844
#define MWKEY_SELECTDOWN	0xF845
#define MWKEY_ACCEPT		0xF846
#define MWKEY_CANCEL		0xF847
#define MWKEY_APP1		0xF848
#define MWKEY_APP2		0xF849
#define MWKEY_LAST		0xF849

/* Keyboard state modifiers*/
typedef unsigned int	MWKEYMOD;

#define MWKMOD_NONE  		0x0000
#define MWKMOD_LSHIFT		0x0001
#define MWKMOD_RSHIFT		0x0002
#define MWKMOD_LCTRL 		0x0040
#define MWKMOD_RCTRL 		0x0080
#define MWKMOD_LALT  		0x0100
#define MWKMOD_RALT  		0x0200
#define MWKMOD_LMETA 		0x0400		/* Windows key*/
#define MWKMOD_RMETA 		0x0800		/* Windows key*/
#define MWKMOD_NUM   		0x1000
#define MWKMOD_CAPS  		0x2000
#define MWKMOD_ALTGR 		0x4000

#define MWKMOD_CTRL	(MWKMOD_LCTRL|MWKMOD_RCTRL)
#define MWKMOD_SHIFT	(MWKMOD_LSHIFT|MWKMOD_RSHIFT)
#define MWKMOD_ALT	(MWKMOD_LALT|MWKMOD_RALT)
#define MWKMOD_META	(MWKMOD_LMETA|MWKMOD_RMETA)

#endif /* _MWTYPES_H*/
