/***********************************
 * $Id: m68360_regs.h,v 1.3 2002/04/18 04:23:27 davidm Exp $
 ***********************************
 *
 ***************************************
 * Definitions of the QUICC registers
 ***************************************
 */

#ifndef __ASM_M68360_REGISTERS_H__
#define __ASM_M68360_REGISTERS_H__

#include <linux/config.h>

/* Mask to select if the PLL prescaler is enabled. */
#define MCU_PREEN   ((unsigned short)(0x0001 << 13))

/*****************************************************************
        CPM Interrupt vector encodings (MC68360UM p. 7-376)
*****************************************************************/
#define CPM_VECTOR_BASE         64  /*Must be divisable by 4*/

#define CPMVEC_NR               32
#define CPMVEC_PIO_PC0	        (0x1f + CPM_VECTOR_BASE)
#define CPMVEC_SCC1	            (0x1e + CPM_VECTOR_BASE)
#define CPMVEC_SCC2	            (0x1d + CPM_VECTOR_BASE)
#define CPMVEC_SCC3	            (0x1c + CPM_VECTOR_BASE)
#define CPMVEC_SCC4		        (0x1b + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC1		    (0x1a + CPM_VECTOR_BASE)
#define CPMVEC_TIMER1		    (0x19 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC2		    (0x18 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC3		    (0x17 + CPM_VECTOR_BASE)
#define CPMVEC_SDMA_CB_ERR	    (0x16 + CPM_VECTOR_BASE)
#define CPMVEC_IDMA1		    (0x15 + CPM_VECTOR_BASE)
#define CPMVEC_IDMA2		    (0x14 + CPM_VECTOR_BASE)
#define CPMVEC_RESERVED3	    (0x13 + CPM_VECTOR_BASE)
#define CPMVEC_TIMER2		    (0x12 + CPM_VECTOR_BASE)
#define CPMVEC_RISCTIMER	    (0x11 + CPM_VECTOR_BASE)
#define CPMVEC_RESERVED2	    (0x10 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC4		    (0x0f + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC5		    (0x0e + CPM_VECTOR_BASE)
#define CPMVEC_TIMER3		    (0x0c + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC6		    (0x0b + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC7		    (0x0a + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC8		    (0x09 + CPM_VECTOR_BASE)
#define CPMVEC_RESERVED1	    (0x08 + CPM_VECTOR_BASE)
#define CPMVEC_TIMER4		    (0x07 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC9		    (0x06 + CPM_VECTOR_BASE)
#define CPMVEC_SPI              (0x05 + CPM_VECTOR_BASE)
#define CPMVEC_SMC1		        (0x04 + CPM_VECTOR_BASE)
#define CPMVEC_SMC2		        (0x03 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC10		    (0x02 + CPM_VECTOR_BASE)
#define CPMVEC_PIO_PC11		    (0x01 + CPM_VECTOR_BASE)
#define CPMVEC_ERROR		    (0x00 + CPM_VECTOR_BASE)
#define CPM_VECTOR_END          CPMVEC_PIO_PC0

#define QUICC_SIM_PIT_INT_VEC   96  /*The interrupt vector for the PIT*/

#define INTERNAL_IRQS           256

// PIT Related constants
#define QUICC_SIM_PIT_INT_LVL   (0x06)
#define QUICC_PITR_SWP          ((unsigned short)(0x0001 << 9))

//#if defined(CONFIG_SED_SIOS)
//#define QUICC_PIT_SPCLK         257812  //See page 6-10 of the 68360 manual
//#endif

#define QUICC_PIT_MAX_COUNT         255
#define QUICC_PIT_PRESCALE_THRES    512 //What point to prescale
#define QUICC_PITR_PTP          ((unsigned short)(0x0001 << 8))
#define QUICC_PIT_DIVIDE        4       //Software divide clock down, needs to
                                        // be a power of 2.


/*****************************************************************
        Macros for Multi channel
*****************************************************************/
/*
#define QMC_BASE(quicc,page) (struct global_multi_pram *)(&quicc->pram[page])
#define MCBASE(quicc,page) (unsigned long)(quicc->pram[page].m.mcbase)
#define CHANNEL_PRAM_BASE(quicc,channel) ((struct quicc32_pram *) \
		(&(quicc->ch_or_u.ch_pram_tbl[channel])))
#define TBD_32_ADDR(quicc,page,channel) ((struct quicc_bd *) \
    (MCBASE(quicc,page) + (CHANNEL_PRAM_BASE(quicc,channel)->tbase)))
#define RBD_32_ADDR(quicc,page,channel) ((struct quicc_bd *) \
    (MCBASE(quicc,page) + (CHANNEL_PRAM_BASE(quicc,channel)->rbase)))
#define TBD_32_CUR_ADDR(quicc,page,channel) ((struct quicc_bd *) \
    (MCBASE(quicc,page) + (CHANNEL_PRAM_BASE(quicc,channel)->tbptr)))
#define RBD_32_CUR_ADDR(quicc,page,channel) ((struct quicc_bd *) \
    (MCBASE(quicc,page) + (CHANNEL_PRAM_BASE(quicc,channel)->rbptr)))
#define TBD_32_SET_CUR_ADDR(bd,quicc,page,channel) \
     CHANNEL_PRAM_BASE(quicc,channel)->tbptr = \
    ((unsigned short)((char *)(bd) - (char *)(MCBASE(quicc,page))))
#define RBD_32_SET_CUR_ADDR(bd,quicc,page,channel) \
     CHANNEL_PRAM_BASE(quicc,channel)->rbptr = \
    ((unsigned short)((char *)(bd) - (char *)(MCBASE(quicc,page))))

#define INCREASE_TBD_32(bd,quicc,page,channel) {  \
    if((bd)->status & T_W)                        \
        (bd) = TBD_32_ADDR(quicc,page,channel);   \
    else                                          \
        (bd)++;                                   \
}
#define DECREASE_TBD_32(bd,quicc,page,channel) {  \
    if ((bd) == TBD_32_ADDR(quicc, page,channel)) \
        while (!((bd)->status & T_W))             \
            (bd)++;                               \
    else                                          \
        (bd)--;                                   \
}
#define INCREASE_RBD_32(bd,quicc,page,channel) {  \
    if((bd)->status & R_W)                        \
        (bd) = RBD_32_ADDR(quicc,page,channel);   \
    else                                          \
        (bd)++;                                   \
}
#define DECREASE_RBD_32(bd,quicc,page,channel) {  \
    if ((bd) == RBD_32_ADDR(quicc, page,channel)) \
        while (!((bd)->status & T_W))             \
            (bd)++;                               \
    else                                          \
        (bd)--;                                   \
}
*/
#endif
