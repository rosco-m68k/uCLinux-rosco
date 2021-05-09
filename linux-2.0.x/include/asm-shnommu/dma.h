#ifndef _SHNOMMU_DMA_H
#define _SHNOMMU_DMA_H 1
 
#include <linux/config.h>

/* These are in kernel/dma.c: */
extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
extern void free_dma(unsigned int dmanr);	/* release it again */
 
#endif /* _SHNOMMU_DMA_H */
