#ifndef _SH7615_PARAM_H
#define _SH7615_PARAM_H

/*
 * Modification History
 *
 * 14 Aug 2002:  posh2
 *             This file need to be modified. The frequency parameter
 *              has to be modified. But for the time being ,
 *              there is no change is done.
 *
 */
#include <linux/config.h>

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

#endif /* _SH7615_PARAM_H */
