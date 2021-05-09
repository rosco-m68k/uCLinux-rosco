/*****************************************************************************/
/*
 *	t6963fb.c -- Driver for DMA driven t6963 framebuffer device
 *
 *	Copyright (C) 1999, 2000 Rob Scott (rscott@mtrob.fdns.net)
 *
 *  General scheme:
 *    The t6963 is a LCD graphics controller chip. It has two registers:
 *  control and data. One of the modes allows data to be written sequentally,
 *  without accessing the control register. This mode is used to copy data
 *  from a region of host memory to the LCD framebuffer. DMA is used to perform
 *  the transfer, if the define CONFIG_T6963_DMA is set, else PIO mode is used.
 *
 *
 *  Implements only enough of the FB ioctls to get pixy to work.
 *
 * 
 */



/*****************************************************************************/

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <asm/param.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcfdma.h>
#include <asm/mcftimer.h>

#include <linux/fb.h>

/*****************************************************************************/
/*
 * LCD parameters
 */

#define LCD_XRES        240
#define LCD_YRES        64
#define LCD_BPP         1       /* Bits per pixel */
/* how many bytes per row on screen */
#define BYTES_PER_ROW (LCD_XRES/(8/LCD_BPP))

#define T_BASE 0x0200           /* base address of text memory */
#define G_BASE 0x0000           /* base address of graphics memory */

#define	T6963FB_MAJOR	29

#ifdef CONFIG_T6963_DMA
#define	T6963FB_VEC	124
#define	T6963FB_DMA_CHAN	1
/* Rate at which frame buffer copied to LCD memory (in Hz) */
#define LCD_REFRESH_RATE 100
#else
/* Number of times 100Hz timer tick must occur before copying fb to LCD */
#define PIO_REFRESH_TIMER_TICKS 4
#endif

#define LCD_DATA 0x30C00000
#define LCD_CMD (LCD_DATA + 4)

#define	T6963FB_DMA_BUFSIZE	((LCD_XRES * LCD_YRES * LCD_BPP)/8 )
#define	T6963FB_DMA_DESTADDR	LCD_DATA

/*
 * LCD controller command/status defines
 */

/* Set this define to use ColdFire MSB to T6963 LSB mapping */
/* #define BIT_REV */

/* Value to use for timeout when polling for LCD controller ready */
#define TIMEOUT_VALUE 0x1000

#ifdef BIT_REV
#define CMD_DONE 0xC0
#define AUTO_CMD_DONE 0x10
#else
#define CMD_DONE 0x03
#define AUTO_CMD_DONE 0x08
#endif

#define SET_ADDR_CMD  0x24
#define WRITE_DATA_CMD 0xc0 
#define AUTO_WRITE_CMD 0xb0
#define RESET_AUTO_CMD 0xb2
#define SET_GBASE_CMD 0x42
#define SET_GROW_CMD 0x43
#define SET_TBASE_CMD 0x40
#define SET_TROW_CMD 0x41
#define SET_CURSOR_HEIGHT_CMD 0xa7
#define SET_CURSOR_LOC_CMD 0x21
#define SET_MODE_GRAPH_OR_TEXT_CMD 0x80
#define SET_MODE_GRAPHICS_CMD 0x9b

/*
 * A cute logo to use at boot time.
 * Put on upper right part of display.
 */

#define penguin_width 16
#define penguin_height 16
#define penguin_xbegin ((LCD_XRES - penguin_width)/(8/LCD_BPP))
#define penguin_xend (penguin_xbegin + (penguin_width/(8/LCD_BPP)))
#define penguin_ybegin 0
#define penguin_yend (penguin_ybegin + penguin_height)

#ifdef BIT_REV
static unsigned char penguin_bits[] = {
  0xc0, 0x03, 0xc0, 0x03, 0xa0, 0x06, 0x60, 0x07, 0x20, 0x06, 0xe0, 0x07,
  0x20, 0x0c, 0x10, 0x1c, 0x10, 0x18, 0x18, 0x18, 0x08, 0x38, 0x14, 0x38,
  0x62, 0x44, 0x16, 0x6a, 0xe4, 0x37, 0x38, 0x0c };   
#else
static unsigned char penguin_bits[] = {
  0x03, 0xc0, 0x03, 0xc0, 0x05, 0x60, 0x06, 0xe0, 0x04, 0x60, 0x07, 0xe0,
  0x04, 0x30, 0x08, 0x38, 0x08, 0x18, 0x18, 0x18, 0x10, 0x1c, 0x28, 0x1c,
  0x46, 0x22, 0x68, 0x56, 0x27, 0xec, 0x1c, 0x30 };   
#endif


/*****************************************************************************/
/*
 *     Frame buffer memory
 */
unsigned char	t6963fb_buff[T6963FB_DMA_BUFSIZE];

/*
 * Globals
 */
struct fb_fix_screeninfo fix;
volatile unsigned char	*LCD_data = (unsigned char *) LCD_DATA;
volatile unsigned char	*LCD_cmd  = (unsigned char *) LCD_CMD;
#ifndef CONFIG_T6963_DMA
struct timer_list pio_timer;
#endif

/*****************************************************************************/
/*  LCD controller specifics                                                 */
/*****************************************************************************/

/*
 * Bit reversal command
 */

unsigned char bit_rev(unsigned char data) {
  unsigned char result, bit;
  int i;

  result = 0;
  for (i = 0; i < 8; i++) {
    bit = (data & (1 << i)) >> i;
    result = result | (bit << (7 - i));
  }
  return(result);
}

/*
 * Check status of T6963. Wait until ready.
 */

int check_status(void) {
  int i;

  // set timeout value
  i = TIMEOUT_VALUE;

  // Wait until cmd ready and no data operations executing
  while (((*LCD_cmd & CMD_DONE) != CMD_DONE) && (i > 0)) {
    i--;
#ifdef DEBUG
    printk("status: %02x, %02x\n", *LCD_cmd, *LCD_cmd & CMD_DONE);
#endif
  };

  if (i == 0)
    printk("%s(%d): %s() timeout\n", __FILE__, __LINE__, __FUNCTION__);

  return(i == 0);
}

int check_status_auto_write(void) {
  int i;

  // set timeout value
  i = TIMEOUT_VALUE;

  // Wait until cmd ready and no data operations executing
  while (((*LCD_cmd & AUTO_CMD_DONE) != AUTO_CMD_DONE) && (i > 0)) {
    i--;
#ifdef DEBUG
    printk("status: %02x, %02x\n", *LCD_cmd, *LCD_cmd & AUTO_CMD_DONE);
#endif
  }
  if (i == 0)
    printk("%s(%d): %s() timeout\n", __FILE__, __LINE__, __FUNCTION__);

  return(i == 0);
}

/*
 * Do data write
 */
int write_data(unsigned char data) {
  int ret_val;
  ret_val = check_status();
#ifdef BIT_REV
  //  printk("write data: %02x, %02x\n", data, bit_rev(data));
  *LCD_data = bit_rev(data);
#else
  *LCD_data = data;
#endif
  return(ret_val);
}

/*
 * Do command write
 */
int write_cmd(unsigned char data) {
  int ret_val;
  ret_val = check_status();
#ifdef BIT_REV
  //  printk("write data: %02x, %02x\n", data, bit_rev(data));
  *LCD_cmd = bit_rev(data);
#else
  *LCD_cmd = data;
#endif
  return(ret_val);
}
#ifdef CONFIG_T6963_DMA
/*****************************************************************************/
/* Timer stuff                                                               */
/* Uses timer 1, timer 0 reserved for system use                             */
/*****************************************************************************/
void set_timer(unsigned int rate)
{
	volatile unsigned short	*timerp;
	unsigned long		flags;

	timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE2);
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;

	save_flags(flags); cli();
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;
	timerp[MCFTIMER_TER] = MCFTIMER_TER_CAP | MCFTIMER_TER_REF;
        timerp[MCFTIMER_TRR] = (unsigned short) (MCF_CLK /rate );
        timerp[MCFTIMER_TMR] = (0 << 8) | /* set prescaler to divide by 1 */
                        MCFTIMER_TMR_DISCE | MCFTIMER_TMR_DISOM |
                        MCFTIMER_TMR_ENORI | MCFTIMER_TMR_RESTART |
                        MCFTIMER_TMR_CLK1  | MCFTIMER_TMR_ENABLE;
	restore_flags(flags);
}

/*****************************************************************************/
/* DMA stuff                                                                 */
/*****************************************************************************/
void t6963fb_startdma(unsigned char *buf, unsigned int size)
{

	/* Clear DMA interrupt */
        disable_dma(T6963FB_DMA_CHAN);
  
	/* Wait until no auto mode writes executing */
	check_status_auto_write();

	/* Reset mode to normal */
	write_cmd(RESET_AUTO_CMD);

	/* Prepare LCD controller to do auto mode write */
	write_data(G_BASE & 0xff);
	write_data(G_BASE >> 8);

	/* Set address pointer */
	write_cmd(SET_ADDR_CMD);

	/* Set mode to auto write */
	write_cmd(AUTO_WRITE_CMD);

	/* Do DMA write to i/o operation */
	set_dma_mode(T6963FB_DMA_CHAN, DMA_MODE_WRITE);
	set_dma_device_addr(T6963FB_DMA_CHAN, T6963FB_DMA_DESTADDR);
	set_dma_addr(T6963FB_DMA_CHAN, (unsigned int) buf);
	set_dma_count(T6963FB_DMA_CHAN, size);

	/* Fire it off! */
	enable_dma(T6963FB_DMA_CHAN);
}

void t6963fb_isr(int irq, void *dev_id, struct pt_regs *regs)
{

        /* Repeat the DMA operation */
	/* Interrupt flag cleared in startdma() */
	t6963fb_startdma(t6963fb_buff, T6963FB_DMA_BUFSIZE);
}

/*****************************************************************************/

#else
/*****************************************************************************/
/* Use PIO to copy frame buffer contents to LCD controller                   */
/*****************************************************************************/
void t6963fb_pio_copy(void) {
          int i;

          /* Re-init refresh timer */
          del_timer(&pio_timer);
	  pio_timer.expires = jiffies + PIO_REFRESH_TIMER_TICKS;
	  add_timer(&pio_timer);

	  /* Wait until no auto mode writes executing */
	  check_status_auto_write();

	  /* Reset mode to normal */
	  write_cmd(RESET_AUTO_CMD);

	  /* Prepare LCD controller to do auto mode write */
	  write_data(G_BASE & 0xff);
	  write_data(G_BASE >> 8);

	  /* Set address pointer */
	  write_cmd(SET_ADDR_CMD);

	  /* Set mode to auto write */
	  write_cmd(AUTO_WRITE_CMD);

	  for (i = 0; i < T6963FB_DMA_BUFSIZE; i++) {
	    check_status_auto_write();
	    *LCD_data = t6963fb_buff[i];
	  }
}
#endif

/*****************************************************************************/

static int 
t6963fb_read(struct inode *inode, struct file *file, char *buf, int count)
{
	unsigned long p = file->f_pos;
	int copy_size;
	char *base_addr;

	if (count < 0)
		return -EINVAL;

	base_addr = (char *) fix.smem_start;
	copy_size = (count + p <= fix.smem_len ? count : fix.smem_len - p);
	memcpy_tofs(buf, base_addr+p, copy_size);
	file->f_pos += copy_size;
	return copy_size;
}

static int 
t6963fb_write(struct inode *inode, struct file *file, const char *buf, int count)
{
	unsigned long p = file->f_pos;
	int copy_size;
	char *base_addr;

	if (count < 0)
		return -EINVAL;

	base_addr = (char *) fix.smem_start;
	copy_size = (count + p <= fix.smem_len ? count : fix.smem_len - p);
	memcpy_fromfs(base_addr+p, buf, copy_size); 
	file->f_pos += copy_size;
	return copy_size;
}

#define  FILE_OP_DEBUG

static int 
t6963fb_mmap(struct inode *inode, struct file *file, struct vm_area_struct * vma)
{
#ifdef  FILE_OP_DEBUG
	printk("%s(%d): %s()\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	vma->vm_start = fix.smem_start+vma->vm_offset;

	return 0;
}

int t6963fb_open(struct inode *inode, struct file *filp)
{
#ifdef  FILE_OP_DEBUG
	printk("%s(%d): %s()\n", __FILE__, __LINE__, __FUNCTION__);
#endif
	return(0);
}

/*****************************************************************************/

void t6963fb_release(struct inode *inode, struct file *filp)
{
#ifdef FILE_OP_DEBUG
	printk("%s(%d): %s()\n", __FILE__, __LINE__, __FUNCTION__);
#endif
}

/*****************************************************************************/


int t6963fb_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct fb_var_screeninfo var;
  int	rc = 0;
  int   i;

#ifdef FILE_OP_DEBUG
	printk("%s(%d): t6963fb_ioctl()\n", __FILE__, __LINE__);
#endif

  switch (cmd) {

  case FBIOGET_VSCREENINFO:
    i = verify_area(VERIFY_WRITE, (void *) arg, 
		    sizeof(struct fb_var_screeninfo));

    var.xres = LCD_XRES;		/* visible resolution		*/
    var.yres = LCD_YRES;
    var.xres_virtual = LCD_XRES;	/* virtual resolution		*/
    var.yres_virtual = LCD_YRES;
    var.xoffset = 0;			/* offset from virtual to visible */
    var.yoffset = 0;			/* resolution			*/
    var.bits_per_pixel = LCD_BPP;	/* guess what 			*/
    var.grayscale = 1;			/* != 0 Graylevels instead of colors */

    memcpy_tofs((void *) arg, &var, sizeof(var));
    break;
  case FBIOGET_FSCREENINFO:
    i = verify_area(VERIFY_WRITE, (void *) arg, 
		    sizeof(struct fb_fix_screeninfo));
    if (i)	return i;
    memcpy_tofs((void *) arg, &fix, sizeof(fix));
    return i;

  default:
    rc = -EINVAL;
    break;
  }

  return(rc);
}
/*
 *	Exported file operations structure for driver...
 */

struct file_operations	t6963fb_fops = {
	NULL,          /* lseek */
	t6963fb_read,
	t6963fb_write,
	NULL,          /* readdir */
	NULL,          /* poll */
	t6963fb_ioctl,
	t6963fb_mmap,
	t6963fb_open,
	t6963fb_release,
	NULL,          /* fsync */
	NULL,          /* fasync */
	NULL,          /* check_media_change */
	NULL           /* revalidate */
};

/*****************************************************************************/
/*
 * Setup LCD module parameters
 */

int lcd_init(void) {

  /* Reset mode to normal, fail if we get a timeout */
  if (write_cmd(RESET_AUTO_CMD))
    return(-1);

  /* set graphics memory to address G_BASE */
  write_data(G_BASE & 0xff);
  write_data(G_BASE >> 8);
  write_cmd(SET_GBASE_CMD);

  /* set bytes per graphics line to BYTES_PER_ROW */
  write_data(BYTES_PER_ROW & 0xff);
  write_data(BYTES_PER_ROW >> 8);
  write_cmd(SET_GROW_CMD);

  /* set text memory to address T_BASE */
  write_data(T_BASE & 0xff);
  write_data(T_BASE >> 8);
  write_cmd(SET_TBASE_CMD);

  /* set bytes per text line to BYTES_PER_ROW */
  write_data(BYTES_PER_ROW & 0xff);
  write_data(BYTES_PER_ROW >> 8);
  write_cmd(SET_TROW_CMD);

  /* mode set: Graphics OR Text, ROM CGen */
  write_cmd(SET_MODE_GRAPH_OR_TEXT_CMD);

  /* cursor is 8 lines high */
  write_cmd(SET_CURSOR_HEIGHT_CMD);

  /* put cursor at (x,y) location */
  write_data(0x00);
  write_data(0x00);
  write_cmd(SET_CURSOR_LOC_CMD);

  /* Graphics on */
  write_cmd(SET_MODE_GRAPHICS_CMD);

  return(0);
} 

void t6963fb_init(void)
{
	int			result;
	int                     i, j;

	printk ("t6963fb: Copyright (C) 1999, Rob Scott"
		"(rscott@mtrob.fdns.net)\n");

	/* Setup chip select for T6963 LCD controller */
	/* CS6, address 0x30C00000 */
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_CSMR6)) = 0x0001;
	/* 8 bit, internal ack, no burst read and write */
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_CSCR6)) = 0x3140;

	/* Do LCD init */
	if (lcd_init()) {
	  printk(KERN_WARNING "t6963fb: can't init LCD controller, exiting\n");
	  return;
	}

	/* Do "frame buffer" init */
	for (j = 0; j < LCD_YRES; j++) {
	  for (i = 0; i < BYTES_PER_ROW; i++) {
	    t6963fb_buff[(j * BYTES_PER_ROW) + i] = 
	      ((i >= penguin_xbegin) && (i < penguin_xend) &&
	       (j >= penguin_ybegin) && (j < penguin_yend)) ?
	      penguin_bits[(j * (penguin_width/8)) + (i - penguin_xbegin)] : 0;
	  }
	}

	/*  Setup data structures for ioctls: */
	//	fix.id = "t6963fb";	/* identification string eg "TT Builtin" */
       /* Start of frame buffer mem */
	fix.smem_start = (unsigned long)t6963fb_buff;
	fix.smem_len = T6963FB_DMA_BUFSIZE;  /* Length of frame buffer mem */
	fix.type = FB_TYPE_PACKED_PIXELS;    /* see FB_TYPE_* 		*/
	fix.type_aux = 0;		/* Interleave for interleaved Planes */
	fix.visual = FB_VISUAL_MONO01;	     /* see FB_VISUAL_*              */
	fix.xpanstep = 0;                    /* zero if no hardware panning  */
	fix.ypanstep = 0;                    /* zero if no hardware panning  */
	fix.ywrapstep = 0;                   /* zero if no hardware ywrap    */
	fix.line_length = LCD_XRES/LCD_BPP;  /* length of a line in bytes    */

	/* Register character device */
	result = register_chrdev(T6963FB_MAJOR, "t6963fb", &t6963fb_fops);
	if (result < 0) {
		printk(KERN_WARNING "t6963fb: can't get major %d\n",
			T6963FB_MAJOR);
		return;
	}

#ifdef CONFIG_T6963_DMA
	{
	  volatile unsigned char	*mbarp, *dmap;
	  unsigned long		imr;
	  unsigned int		icr;

	  /* Make sure that the parallel port pin is set for DREQ */
	  if (T6963FB_DMA_CHAN == 0)
	    /* Make parallel port pin DREQ[0] */
	    *(volatile unsigned short *)(MCF_MBAR + MCFSIM_PAR) = 
	                                                  MCFSIM_PAR_DREQ0;
	  if (T6963FB_DMA_CHAN == 1)
	    /* Make parallel port pin DREQ[1] */
	    *(volatile unsigned short *)(MCF_MBAR + MCFSIM_PAR) = 
                                                          MCFSIM_PAR_DREQ1;

	  /* Install ISR (interrupt service routine) */
	  result = request_irq(T6963FB_VEC, t6963fb_isr, SA_INTERRUPT,
						   "t6963fb", NULL);
	  if (result) {
		printk ("t6963fb: IRQ %d already in use\n", T6963FB_VEC);
		return;
	  }

	  /* Set interrupt vector location */
	  dmap = (volatile unsigned char *) dma_base_addr[T6963FB_DMA_CHAN];
	  dmap[MCFDMA_DIVR] = T6963FB_VEC;

	  /* Set interrupt level and priority */
	  switch (T6963FB_DMA_CHAN) {
	  case 0:  icr = MCFSIM_DMA0ICR ; imr = MCFSIM_IMR_DMA0; break;
	  case 1:  icr = MCFSIM_DMA1ICR ; imr = MCFSIM_IMR_DMA1; break;
	  case 2:  icr = MCFSIM_DMA2ICR ; imr = MCFSIM_IMR_DMA2; break;
	  case 3:  icr = MCFSIM_DMA3ICR ; imr = MCFSIM_IMR_DMA3; break;
	  default: icr = MCFSIM_DMA0ICR ; imr = MCFSIM_IMR_DMA0; break;
	  }

	  mbarp = (volatile unsigned char *) MCF_MBAR;
	  mbarp[icr] = MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI2;
	  mcf_setimr(mcf_getimr() & ~imr);

	  /* Request DMA channel */
	  printk ("t6963fb: requesting DMA channel %d\n", T6963FB_DMA_CHAN);
	  result = request_dma(T6963FB_DMA_CHAN, "t6963fb");
	  if (result) {
		printk ("t6963fb: dma channel %d already in use\n",
			T6963FB_DMA_CHAN);
		return;
	  }

	  /* Make sure we don't get any extraneous interrupts */
	  disable_dma(T6963FB_DMA_CHAN);

	  /* Set "refresh" rate */
	  set_timer(LCD_REFRESH_RATE * T6963FB_DMA_BUFSIZE);

	  /* Do first DMA. Subsequent DMAs will be started by ISR */
	  t6963fb_startdma(t6963fb_buff, T6963FB_DMA_BUFSIZE);
	}
#else
	printk("t6963fb: using PIO mode\n");
	/* Set up PIO timer */
	pio_timer.function = t6963fb_pio_copy;
	pio_timer.expires = jiffies + PIO_REFRESH_TIMER_TICKS;
	add_timer(&pio_timer);
#endif
}

/*****************************************************************************/

