#include "m68360_regs.h"
#include "m68360_pram.h"
#include "m68360_quicc.h"
/*#include "m68360_enet.h" */

#ifndef __ASM_M68360_H__
#define __ASM_M68360_H__

#ifdef __KERNEL__

extern QUICC *m68360_get_pquicc(void);
#define IRQ1_IRQ_NUM    (25)
#define IRQ2_IRQ_NUM    (26)
#define IRQ3_IRQ_NUM    (27)
#define IRQ4_IRQ_NUM    (28)
#define IRQ5_IRQ_NUM    (29)
#define IRQ6_IRQ_NUM    (30)
#define IRQ7_IRQ_NUM    (31)

#endif //kernel
#endif
