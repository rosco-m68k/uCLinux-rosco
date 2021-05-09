#ifndef __SHNOMMU_UNALIGNED_H
#define __SHNOMMU_UNALIGNED_H

#define get_unaligned(ptr) ({			\
	typeof((*(ptr))) x;			\
	memcpy(&x, (void*)ptr, sizeof(*(ptr)));	\
	x;					\
})

#define put_unaligned(val, ptr) ({		\
	typeof((*(ptr))) x = val;		\
	memcpy((void*)ptr, &x, sizeof(*(ptr)));	\
})

#endif
