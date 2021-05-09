#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

/*
 * This is the maximum DMA address that can be DMAd to.
 */
#define MAX_DMA_ADDRESS		0x03000000

/*
 * DMA modes - we have two, IN and OUT
 */
typedef enum {
	DMA_MODE_READ,
	DMA_MODE_WRITE
} dmamode_t;

#define MAX_DMA_CHANNELS  3
#define DMA_VIRTUAL_FLOPPY0 0
#define DMA_VIRTUAL_FLOPPY1 1
#define DMA_VIRTUAL_SOUND 2


#define DMA_FLOPPY		DMA_VIRTUAL_FLOPPY

#endif /* _ASM_ARCH_DMA_H */
