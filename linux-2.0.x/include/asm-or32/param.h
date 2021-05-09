#ifndef _OR32_PARAM_H
#define _OR32_PARAM_H

#include <linux/config.h>

#define HZ 100

#define EXEC_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif /* _OR32_PARAM_H */
