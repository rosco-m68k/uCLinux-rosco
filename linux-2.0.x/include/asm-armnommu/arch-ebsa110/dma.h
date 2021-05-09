#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#define MAX_DMA_ADDRESS		0xffffffff

/*
 * DMA modes - we have two, IN and OUT
 */
typedef enum {
	DMA_MODE_READ,
	DMA_MODE_WRITE
} dmamode_t;

#define MAX_DMA_CHANNELS	0

#endif /* _ASM_ARCH_DMA_H */

