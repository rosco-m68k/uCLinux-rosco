#include <asm/segment.h>
#include <linux/fs.h>

#include "blip-led.h"

unsigned int define_as_pio(unsigned int pio_ctrl_id, unsigned int mask);
unsigned int define_as_output(unsigned int pio_ctrl_id, unsigned int mask);

struct file_operations blip_led_fops = {
  NULL,                /* lseek   */
  blip_led_read,       /* read    */
  NULL,                /* write   */
  NULL,                /* readdir */
  NULL,                /* select  */
  blip_led_ioctl,      /* ioctl   */
  NULL,                /* mmap    */
  NULL,                /* open    */
  NULL                 /* release */
};

//struct file_operations blip_led_fops = {
//  NULL,                /* lseek   */
//  blip_led_read,       /* read    */
//  blip_led_write,      /* write   */
//  NULL,                /* readdir */
//  blip_led_select,     /* select  */
//  blip_led_ioctl,      /* ioctl   */
//  NULL,                /* mmap    */
//  blip_led_open,       /* open    */
//  NULL                 /* release */
//};

volatile unsigned int *pio_output_enable = (unsigned int *) PIO_OER;
volatile unsigned int *pio_enable = (unsigned int *) PIO_PER;
volatile unsigned int *pio_set_output_data_register = (unsigned int *) PIO_SODR;
volatile unsigned int *pio_clear_output_data_register = (unsigned int *) PIO_CODR;

static unsigned int blip_led_on = BLIP_LED_OFF;

int blip_led_init(void)
{
  printk("blip LED.\n");

  /* Defines the LED as pio. */
  *pio_enable = LED;

  /* Defines the LED as output. */
  *pio_output_enable = LED;

  /* Turns the LED on. */
  *pio_set_output_data_register = LED;
  blip_led_on = BLIP_LED_ON;
  
  return 0;
}


int blip_led_read(struct inode *p_inode, struct file *p_file, char *buffer, int length)
{
  char message_ON[] = "The blip LED is ON.";
  char message_OFF[] = "The blip LED is OFF.";
  char *message;
  int no_bytes=0;

  if (BLIP_LED_ON == blip_led_on) {
    message = message_ON;
  }
  else {
    message = message_OFF;
  }

  while((0 != length) && (NULL != message)) {
    put_user(*(message++), buffer++);
    length--;
    no_bytes++;
  }

  return no_bytes;
}

/*
int blip_led_write(struct inode *, struct file *, const char *, int)
{
  return -EINVAL;
}

int blip_led_select(struct inode *, struct file *, int, select_table *)
{
  return -EINVAL;
}
*/

int blip_led_ioctl(struct inode *p_inode, struct file *p_file, unsigned int cmd, unsigned long arg)
{
  static unsigned int blip_led_on = 0;
  switch(cmd) {
  case BLIP_LED_ON:
    *pio_set_output_data_register = LED;
    blip_led_on = BLIP_LED_ON;
    return 0;
    break;
  case BLIP_LED_OFF:
    *pio_clear_output_data_register = LED;
    blip_led_on = BLIP_LED_OFF;
    return 0;
    break;
  case BLIP_LED_TOGGLE:
    if (BLIP_LED_OFF == blip_led_on) {
      *pio_clear_output_data_register = LED;
      blip_led_on = BLIP_LED_ON;
    }
    else {
      *pio_set_output_data_register = LED;
      blip_led_on = BLIP_LED_ON;
    }
    return 0;
    break;
  case BLIP_LED_BLINK:
    return 0;
    break;
  default:
    return -EINVAL;
    break;
  }

  return -EINVAL;
}

/*
int blip_led_open(struct inode *p_inode, struct file *p_file)
{
  return -EINVAL;
}
*/
