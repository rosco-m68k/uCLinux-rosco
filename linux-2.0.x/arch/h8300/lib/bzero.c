#include <linux/types.h>
#include <linux/string.h>

/* FIXME: put this over in libgcc.a on the compiler side */

void bzero(void * s, size_t count) {
	memset(s, '\0', count);
}
