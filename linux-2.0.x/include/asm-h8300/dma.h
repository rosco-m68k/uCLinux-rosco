#ifndef _H8300H_DMA_H
#define _H8300H_DMA_H 1
 
#include <linux/config.h>

#define MAX_DMA_CHANNELS 0

/* These are in kernel/dma.c: */
 extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
 extern void free_dma(unsigned int dmanr);	/* release it again */
 
#endif /* _H8300H_DMA_H */

