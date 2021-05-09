#ifndef _I960_DMA_H
#define _I960_DMA_H

/* Don't define MAX_DMA_ADDRESS; it's useless on the i960 and any
   occurrence should be flagged as an error.  */

/* the VH has 2 DMA channels, the Cx has 4 */
#define MAX_DMA_CHANNELS 4

extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
extern void free_dma(unsigned int dmanr);	/* release it again */

#endif /* _I960_DMA_H */
