#ifndef _LINUX_NVRAM_H
#define _LINUX_NVRAM_H

#include <linux/ioctl.h>

/* /dev/nvram ioctls - copied from 2.4.18 kernel */
#define NVRAM_INIT	_IO('p', 0x40) /* clear NVRAM and set checksum */
#define NVRAM_SETCKS	_IO('p', 0x41) /* recalculate checksum */

#endif  /* _LINUX_NVRAM_H */
