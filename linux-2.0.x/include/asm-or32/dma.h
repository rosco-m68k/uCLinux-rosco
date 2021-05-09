#ifndef _OR32_DMA_H
#define _OR32_DMA_H 1
 
#include <linux/config.h>

#define MAX_DMA_CHANNELS 8
 
/* These are in kernel/dma.c: */
 extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
 extern void free_dma(unsigned int dmanr);	/* release it again */
 
#endif /* _OR32_DMA_H */
