#ifndef _I960_PARAM_H
#define _I960_PARAM_H

#include <linux/config.h>

/* this is not quite arbitrary; it needs to be longer than the longest path
 * through schedule() */
#ifndef HZ
#define HZ 100
#endif

#define EXEC_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif /* _I960_PARAM_H */
