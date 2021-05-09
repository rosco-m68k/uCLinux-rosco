/*
 *  linux/fs/proc/serial.c
 *
 *  Copyright (C) 1999, Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 2000  Lineo Inc. (www.lineo.com)  
 *
 *  Copied and hacked from array.c, which was:
 *
 *  Copyright (C) 1991, 1992 Linus Torvalds
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/tty.h>
#include <linux/user.h>
#include <linux/a.out.h>
#include <linux/string.h>
#include <linux/mman.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/config.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/swap.h>

#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#if defined(CONFIG_COLDFIRE_SERIAL)
extern int mcfrs_readproc(char *buffer);
#elif defined(CONFIG_M68360)
//FIXME: get a better constant testing for SMC serial support
int quicc_smc_readproc(char *buffer);
int quicc_scc_readproc(char *buffer);
#endif

int get_serialinfo(char * buffer)
{
	int len = 0;

#if defined(CONFIG_COLDFIRE_SERIAL)
	len = mcfrs_readproc(buffer);

#elif defined(CONFIG_M68360_SMC_UART) || defined(CONFIG_M68360_SCC_UART)
#if defined(CONFIG_M68360_SMC_UART)
	len =  quicc_smc_readproc(buffer);
#endif
#if defined(CONFIG_M68360_SCC_UART)
	len += quicc_scc_readproc(&buffer[len]);
#endif

#else
	len += sprintf(buffer, "No Serial Info\n");
#endif

	return(len);
}

