#include <unistd.h>
#include <errno.h>
#undef _POSIX_PTHREADS

#include "dirstream.h"

void seekdir(DIR * dir, off_t offset)
{
  if (!dir) {
    errno = EBADF;
    return;
  }
  dir->dd_nextoff = lseek(dir->dd_fd, offset, SEEK_SET);
  dir->dd_size = dir->dd_nextloc = 0;
}
