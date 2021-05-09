/* ioctl constants for 68332dio.c */

#include <linux/ioctl.h>

#define TRUE	1
#define FALSE	0

#define PIN0	0x01
#define PIN1	0x02
#define PIN2	0x04
#define PIN3	0x08
#define PIN4	0x10
#define PIN5	0x20
#define PIN6	0x40
#define PIN7	0x80

/* PORTE ioctl defines */
#define PORTESOUT	_IOW ('t', 0, char)
#define PORTESIN	_IOW ('t', 1, char)

/* PORTF ioctl defines */
#define PORTFSOUT	_IOW ('t', 2, char)
#define PORTFSIN	_IOW ('t', 3, char)

/* TPU defines */

