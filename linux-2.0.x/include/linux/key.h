/*****************************************************************************/

/*
 *	key.h -- Crypto key storage driver
 *
 *	(C) Copyright 2002, Greg Ungerer (gerg@snapgear.com)
 */

/*****************************************************************************/
#ifndef _KEY_H_
#define	_KEY_H_
/*****************************************************************************/

#include <linux/time.h>

/*
 *	Define the ioctl set used to control the driver
 */
#define	IOCMD(x)	(('k' << 8) | (x))

#define	KEY_GET		IOCMD(1)		/* Get key from driver */
#define	KEY_SET		IOCMD(2)		/* Set key */
#define	KEY_FRESHEN	IOCMD(3)		/* Freshen key */


/*
 *	Define the key structure. This is what is stored in FLASH,
 *	and what is used as the argument to the ioctls. Maximum
 *	key size is currently 2048 bits.
 */
#define	KEY_MAXSIZE	(2048 / 8)		/* 2048 bit key maximum */
#define	KEY_MAGIC	0x4b655924		/* Magic number value */

struct key {
	unsigned int	magic;			/* Magic number */
	struct timeval	stamp;			/* Time of key set/freshen */
	int		size;			/* Number of bytes in key */
	unsigned char	key[KEY_MAXSIZE];	/* Actual key data */
};


/*****************************************************************************/
#ifndef __KERNEL__

/*
 *	Simple user access function to get the current key. If a valid
 *	key is stored in FLASH then it is return. Return value is -1
 *	on failure (no key), otherwise the keysize (in bytes) is
 *	returned.
 */
static __inline__ int getdriverkey(unsigned char *kp, int ksize)
{
	struct key	k;
	int		fd, rc = -1;

	if ((fd = open("/dev/key", O_RDONLY)) >= 0) {
		if (ioctl(fd, KEY_GET, &k) >= 0) {
			rc = (k.size < ksize) ? k.size : ksize;
			memcpy(kp, &k.key[0], rc);
		}
		close(fd);
	}
	return(rc);
}

/*
 *	Simple user access function to set the current key. 
 *	Returns -1 on failure, 0 otherwise.
 */
static __inline__ int setdriverkey(unsigned char *kp, int ksize)
{
	struct key	k;
	int		fd, rc = -1;

	k.magic = KEY_MAGIC;
	k.size = ksize;
	memcpy(&k.key[0], kp, ksize);
	if ((fd = open("/dev/key", O_RDONLY)) >= 0) {
		rc = ioctl(fd, KEY_SET, &k);
		close(fd);
	}
	return(rc);
}
 
/*
 *	Simple user function to freshen current key.
 *	Returns -1 on failure, 0 otherwise.
 */
static __inline__ int freshendriverkey(void)
{
	int	fd, rc = -1;

	if ((fd = open("/dev/key", O_RDONLY)) >= 0) {
		rc = ioctl(fd, KEY_FRESHEN, NULL);
		close(fd);
	}
	return(rc);
}
 
#endif /* __KERNEL__ */
/*****************************************************************************/
#endif /* _KEY_H_ */
