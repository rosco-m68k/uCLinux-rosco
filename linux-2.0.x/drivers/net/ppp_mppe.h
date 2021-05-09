#ifndef PPP_MPPE_H
#define PPP_MPPE_H

/* This one doesn't appear to be used anywhere */
/* struct compressor * get_mppe_compressor(void); */

/* We need the ability to expand packets by the mppe overhead */
#define MPPE_OVHD		4
extern int mppe_init(void);

#endif
