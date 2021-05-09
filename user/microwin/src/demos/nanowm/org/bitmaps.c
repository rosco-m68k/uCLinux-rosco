/*
 * NanoWM- the NanoGUI window manager.
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * FIXME: Someone with some artistic ability should redo these.
 *
 * Vladimir Cotfas <vladimircotfas@vtech.ca>, Aug 31, 2000:
 *    re-done the system, minimize, maximize, close buttons
 *    in the Motif/KDE style (only two colours though)
 */

#include <nano-X.h>

/*
 * For defining bitmaps easily.
 */
#define X	((unsigned) 1)
#define _	((unsigned) 0)

#define BITS(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
	(((((((((((((((a*2+b)*2+c)*2+d)*2+e)*2+f)*2+g)*2+h)*2+i)*2+j)*2+k) \
	*2+l)*2+m)*2+n)*2+o)*2+p)

/* The utility button not pressed bitmap */
GR_BITMAP utilitybutton_notpressed[] = {
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,X,X,X,X,X,X,X,X,X,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,_,X,_,_,X,_),
	BITS(X,_,_,X,X,X,X,X,X,X,X,X,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
};

/* The utility button pressed bitmap */
GR_BITMAP utilitybutton_pressed[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,_,X,_,_,X),
	BITS(_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X)
};

/* The maximise button not pressed bitmap */
GR_BITMAP maximisebutton_notpressed[] = {
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,X,X,X,X,X,X,X,X,X,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,X,X,X,X,X,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
};

/* The maximise button pressed bitmap */
GR_BITMAP maximisebutton_pressed[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,X,X,X,X,X,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X)
};

/* The iconise button not pressed bitmap */
GR_BITMAP iconisebutton_notpressed[] = {
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,X,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,X,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,X,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
};

/* The iconise button pressed bitmap */
GR_BITMAP iconisebutton_pressed[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,X,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,X,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,X,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X)
};

/* The close button not pressed bitmap */
GR_BITMAP closebutton_notpressed[] = {
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,X,X,_,_,_,_,_,X,X,_,_,X,_),
	BITS(X,_,_,X,X,X,_,_,_,X,X,X,_,_,X,_),
	BITS(X,_,_,_,X,X,X,_,X,X,X,_,_,_,X,_),
	BITS(X,_,_,_,_,X,X,X,X,X,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,X,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,X,X,X,X,X,_,_,_,_,X,_),
	BITS(X,_,_,_,X,X,X,_,X,X,X,_,_,_,X,_),
	BITS(X,_,_,X,X,X,_,_,_,X,X,X,_,_,X,_),
	BITS(X,_,_,X,X,_,_,_,_,_,X,X,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
};

/* The close button pressed bitmap */
GR_BITMAP closebutton_pressed[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,X,X,_,_,_,_,_,X,X,_,_,X),
	BITS(_,X,_,_,X,X,X,_,_,_,X,X,X,_,_,X),
	BITS(_,X,_,_,_,X,X,X,_,X,X,X,_,_,_,X),
	BITS(_,X,_,_,_,_,X,X,X,X,X,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,X,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,X,X,X,X,X,_,_,_,_,X),
	BITS(_,X,_,_,_,X,X,X,_,X,X,X,_,_,_,X),
	BITS(_,X,_,_,X,X,X,_,_,_,X,X,X,_,_,X),
	BITS(_,X,_,_,X,X,_,_,_,_,_,X,X,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X)
};

/* The restore button not pressed bitmap */
GR_BITMAP restorebutton_notpressed[] = {
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,X,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,_,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,X,_,_,_,X,_,_,_,_,X,_),
	BITS(X,_,_,_,X,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,_,X,_,_,X,_),
	BITS(X,_,X,X,X,X,X,X,X,X,X,X,X,_,X,_),
	BITS(X,_,_,X,_,_,_,_,_,_,_,X,_,_,X,_),
	BITS(X,_,_,_,X,_,_,_,_,_,X,_,_,_,X,_),
	BITS(X,_,_,_,_,X,_,_,_,X,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,X,_,X,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,X,_,_,_,_,_,_,X,_),
	BITS(X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
};

/* The restore button pressed bitmap */
GR_BITMAP restorebutton_pressed[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,X,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,_,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,X,_,_,_,X,_,_,_,_,X),
	BITS(_,X,_,_,_,X,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,_,X,_,_,X),
	BITS(_,X,_,X,X,X,X,X,X,X,X,X,X,X,_,X),
	BITS(_,X,_,_,X,_,_,_,_,_,_,_,X,_,_,X),
	BITS(_,X,_,_,_,X,_,_,_,_,_,X,_,_,_,X),
	BITS(_,X,_,_,_,_,X,_,_,_,X,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,X,_,X,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,X,_,_,_,_,_,_,X),
	BITS(_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X)
};

/* The horizontal resize foreground */
GR_BITMAP horizontal_resize_fg[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,X,X,_,_,_,_),
	BITS(_,_,_,X,X,_,_,_,_,_,_,X,X,_,_,_),
	BITS(_,_,X,X,_,_,_,_,_,_,_,_,X,X,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,X,X,_,_,_,_,_,_,_,_,X,X,_,_),
	BITS(_,_,_,X,X,_,_,_,_,_,_,X,X,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
};
int horizontal_resize_columns = 16;
int horizontal_resize_rows = 10;
int horizontal_resize_hotx = 7;
int horizontal_resize_hoty = 4;

/* The horizontal resize cursor background */
GR_BITMAP horizontal_resize_bg[] = {
	BITS(_,_,_,_,X,X,X,_,_,X,X,X,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,X,X,X,X,_,_,_),
	BITS(_,_,X,X,X,X,_,_,_,_,X,X,X,X,_,_),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X),
	BITS(_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_),
	BITS(_,_,X,X,X,X,_,_,_,_,X,X,X,X,_,_),
	BITS(_,_,_,X,X,X,X,_,_,X,X,X,X,_,_,_),
	BITS(_,_,_,_,X,X,X,_,_,X,X,X,_,_,_,_),
};

/* The vertical resize foreground */
GR_BITMAP vertical_resize_fg[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(_,X,X,_,X,X,_,X,X,_,_,_,_,_,_,_),
	BITS(_,X,_,_,X,X,_,_,X,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,_,_,X,X,_,_,X,_,_,_,_,_,_,_),
	BITS(_,X,X,_,X,X,_,X,X,_,_,_,_,_,_,_),
	BITS(_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
};
int vertical_resize_columns = 10;
int vertical_resize_rows = 16;
int vertical_resize_hotx = 4;
int vertical_resize_hoty = 7;

/* The vertical resize cursor background */
GR_BITMAP vertical_resize_bg[] = {
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_),
	BITS(X,X,_,X,X,X,X,_,X,X,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(X,X,_,X,X,X,X,_,X,X,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_),
	BITS(_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_),
};

/* The righthand resize cursor foreground */
GR_BITMAP righthand_resize_fg[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,_,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,_,_,X,X,X,_,_,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,_,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
};
int righthand_resize_columns = 13;
int righthand_resize_rows = 13;
int righthand_resize_hotx = 6;
int righthand_resize_hoty = 6;

/* The righthand resize cursor background */
GR_BITMAP righthand_resize_bg[] = {
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_),
	BITS(X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
};

/* The lefthand resize cursor foreground */
GR_BITMAP lefthand_resize_fg[] = {
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,_,X,X,_,_,_,_),
	BITS(_,X,X,_,_,X,X,X,_,_,X,X,_,_,_,_),
	BITS(_,X,X,_,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_),
};
int lefthand_resize_columns = 13;
int lefthand_resize_rows = 13;
int lefthand_resize_hotx = 6;
int lefthand_resize_hoty = 6;

/* The lefthand resize cursor background */
GR_BITMAP lefthand_resize_bg[] = {
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_),
	BITS(X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_),
	BITS(X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_),
	BITS(X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
	BITS(X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_),
};
