#define  PIO_PER             0xffff0000
#define  PIO_OER             0xffff0010
#define  PIO_SODR            0xffff0030
#define  PIO_CODR            0xffff0034
#define  LED                 (1<<1)
#define  BLIP_LED_OFF        0
#define  BLIP_LED_ON         1
#define  BLIP_LED_TOGGLE     2
#define  BLIP_LED_BLINK      3

int blip_led_init(void);
int blip_led_read(struct inode *, struct file *, char *, int);
int blip_led_write(struct inode *, struct file *, const char *, int);
int blip_led_select(struct inode *, struct file *, int, select_table *);
int blip_led_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
int blip_led_open(struct inode *, struct file *);
