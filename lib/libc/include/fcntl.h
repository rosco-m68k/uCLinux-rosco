#ifndef __FCNTL_H
#define __FCNTL_H

#include <features.h>
#include <sys/types.h>
#include <linux/fcntl.h>

#ifndef FNDELAY
#define FNDELAY	O_NDELAY
#endif

__BEGIN_DECLS

extern int creat __P ((__const char * __filename, mode_t __mode));
extern int fcntl __P ((int __fildes,int __cmd, ...));
extern int open __P ((__const char * __filename, int __flags, ...));

__END_DECLS

#endif
