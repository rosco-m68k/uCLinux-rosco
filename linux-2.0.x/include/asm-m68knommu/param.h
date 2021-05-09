#ifndef _M68K_PARAM_H
#define _M68K_PARAM_H

#include <linux/config.h>

#ifndef HZ
#ifdef CONFIG_HW_FEITH
#define HZ 1000
#endif
#ifdef CONFIG_SHGLCORE
#define HZ 50
#endif
#ifdef CONFIG_SM2010
/* 2002-05-14 gc: */
#define HZ 500
#endif
#ifndef HZ
#define	HZ 100
#endif
#endif

#define EXEC_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif /* _M68K_PARAM_H */
