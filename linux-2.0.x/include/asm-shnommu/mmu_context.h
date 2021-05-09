#ifndef __SH7615_MMU_CONTEXT_H
#define __SH7615_MMU_CONTEXT_H
/*
 * Modification History :
 *
 * 12 Aug : posh2- get_mmu_context macro is changed respect
 *          to H8 code
 *   
 */

#include <linux/config.h>

/*
 * get a new mmu context.. do we need this on the m68k?
 */
#define get_mmu_context(x) do { } while (0)

#endif
