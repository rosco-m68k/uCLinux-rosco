/*
 * Device driver for driving LCD module via DMA.
 * It continously copies a memory area to the LCD.
 * It is up to the user to embed timing control signals in the memory data.
 *
 * Hardware config:
 *  Second timer output connected to DMA1 request (DREQ 0) input
 *
 * Copyright (C) 1999 Rob Scott (rscott@mtrob.fdns.net)
 */

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>

#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcfdma.h>
#include <asm/mcftimer.h>
#include <asm/param.h>     /* for HZ */

/*
 * From sysdep 2.1:
 */

#ifndef LINUX_VERSION_CODE
#  include <linux/version.h>
#endif

#ifndef VERSION_CODE
#  define VERSION_CODE(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#endif

/* only allow 2.0.x and 2.1.y */

#if LINUX_VERSION_CODE < VERSION_CODE(2,0,0)
#  error "This kernel is too old: not supported by this file"
#endif
#if LINUX_VERSION_CODE < VERSION_CODE(2,1,0)
#  define LINUX_20
#else
#  define LINUX_21
#endif

/* changed the prototype of read/write */

#if defined(LINUX_21) || defined(__alpha__)
# define count_t unsigned long
# define read_write_t long
#else
# define count_t int
# define read_write_t int
#endif


#if LINUX_VERSION_CODE < VERSION_CODE(2,1,31)
# define release_t void
#  define release_return(x) return
#else
#  define release_t int
#  define release_return(x) return (x)
#endif

/*
 * access to user space: use the 2.1 functions,
 * and implement them as macros for 2.0
 */

#ifdef LINUX_20
#  include <asm/segment.h>
#  define access_ok(t,a,sz)           (verify_area((t),(a),(sz)) ? 0 : 1)
#  define verify_area_20              verify_area
#  define copy_to_user(t,f,n)         (memcpy_tofs(t,f,n), 0)
#  define __copy_to_user(t,f,n)       copy_to_user((t),(f),(n))
#  define copy_to_user_ret(t,f,n,r)   copy_to_user((t),(f),(n))
#  define copy_from_user(t,f,n)       (memcpy_fromfs((t),(f),(n)), 0)
#  define __copy_from_user(t,f,n)     copy_from_user((t),(f),(n))
#  define copy_from_user_ret(t,f,n,r) copy_from_user((t),(f),(n))
#  define PUT_USER(val,add)           (put_user((val),(add)), 0)
#  define __PUT_USER(val,add)         PUT_USER((val),(add))
#  define PUT_USER_RET(val,add,ret)   PUT_USER((val),(add))
#  define GET_USER(dest,add)          ((dest)=get_user((add)), 0)
#  define __GET_USER(dest,add)        GET_USER((dest),(add))
#  define GET_USER_RET(dest,add,ret)  GET_USER((dest),(add))
#else
#  include <asm/uaccess.h>
#  include <asm/io.h>
#  define verify_area_20(t,a,sz) (0) /* == success */
#  define PUT_USER put_user
#  define __PUT_USER __put_user
#  define PUT_USER_RET put_user_ret
#  define GET_USER get_user
#  define __GET_USER __get_user
#  define GET_USER_RET get_user_ret
#endif


/*
 * Use one of the "local" device numbers
 */

#define LCDDMA_MAJOR 120

/* Define interrupt vector location */
#define DMA_VEC_LOC 120

/* Define which dma channel to use */
#define CHAN 0 

/* Define DMA data destination address (I/O device) */
#define DMA_DEST_ADDR 0x40000000

/*
 * Driver data structures
 */

unsigned int dma_xfer_len = 16;

/* Place to store screen data */
unsigned short dma_buf[32*1024];

unsigned short twiddle_array[16] = {0x3f00, 0x0600, 0x5b00, 0x4f00,
				    0x6600, 0x6d00, 0x7d00, 0x0700,
				    0x7f00, 0x6700, 0x7700, 0x7c00,
				    0x3900, 0x5e00, 0x7900, 0x7100};


/*
 * Define return values
 */
static int lcddma_open(struct inode *inode, struct file *filp);
static release_t lcddma_release (struct inode *inode, struct file *filp);
static read_write_t lcddma_read (struct inode *inode, struct file *filp,
				 char *buf, count_t count);

static read_write_t lcddma_write (struct inode *inode, struct file *filp,
				  const char *buf, count_t count);

/* structure defined in fs.h */
struct file_operations lcddma_fops = {
  NULL,          /* lseek */
  lcddma_read,
  lcddma_write,
  NULL,          /* readdir */
  NULL,          /* poll */
  NULL,          /* ioctl */
  NULL,          /* mmap */
  lcddma_open,
  lcddma_release,
  NULL,          /* fsync */
  NULL,          /* fasync */
  NULL,          /* check_media_change */
  NULL           /* revalidate */
};

//#define LCDDMA_DEBUG

#ifdef LCDDMA_DEBUG
void lcddma_dump_regs(void) {
  volatile unsigned long	*dmap;
  volatile unsigned short	*dmapw;
  volatile unsigned char	*dmapb;

  dmap = (unsigned long *)   (MCF_MBAR + MCFDMA_BASE0);
  dmapw = (unsigned short *) (MCF_MBAR + MCFDMA_BASE0);
  dmapb = (unsigned char *)  (MCF_MBAR + MCFDMA_BASE0);

  /* Dump regs */
  printk("Src:  %08x\n", dmap[MCFDMA_SAR]);
  printk("Dest: %08x\n", dmap[MCFDMA_DAR]);
  printk("Byte: %08x\n", dmapw[MCFDMA_BCR]);
  printk("Ctl:  %08x\n", dmapw[MCFDMA_DCR]);
  printk("stat: %08x\n", dmapb[MCFDMA_DSR]);
  printk("vec:  %08x\n", dmapb[MCFDMA_DIVR]);
}
#endif


/*
 * lcddma_open - allocate memory buffer, start DMA
 */

static int lcddma_open(struct inode *inode, struct file *filp) {

  /* Clear DMA interrupt */
  disable_dma(CHAN);

  /* Do DMA write to i/o operation */
  set_dma_mode(CHAN, DMA_MODE_WRITE_WORD);
  set_dma_device_addr(CHAN, DMA_DEST_ADDR);
  set_dma_addr(CHAN, (unsigned int)dma_buf);
  set_dma_count(CHAN, dma_xfer_len);

#ifdef LCDDMA_DEBUG
  lcddma_dump_regs();
#endif

  /* Fire it off! */
  enable_dma(CHAN);

  return(0);
}

/*
 * lcddma_read - return device bitmap address
 */

static read_write_t lcddma_read (struct inode *inode, struct file *filp,
			  char *buf, count_t count)
{
  unsigned long temp;
  unsigned long *tempP;

  tempP = &(temp);
  temp = (unsigned int)(dma_buf);

  copy_to_user(buf, tempP, 4);

  /* Return number of bytes read */
  return (4);
}


/*
 * lcddma_write - set dma xfer length
 */

static read_write_t lcddma_write (struct inode *inode, struct file *filp,
			   const char *buf, count_t count)
{
  int i;
  unsigned int temp;

  temp = 0;
  for (i = 0; i < 4; i++) {
    temp = (temp << 8) | (unsigned char)buf[i];
  }

  dma_xfer_len = temp & 0xffff;

  return(4);
}

/*
 * lcddma_release - deallocate DMA buffer, stop DMAs...
 */

static release_t lcddma_release (struct inode *inode, struct file *filp) {
  unsigned long flags;

  /* Save current interrupt status: enabled or disabled */
  save_flags(flags);

  /* Disable interrupts */
  cli();

  /* Turn off DMA interrupts and stop any DMA in progress */
  disable_dma(CHAN);
  
  /* Restore interrupt status */
  restore_flags(flags);

  MOD_DEC_USE_COUNT;

  release_return(0);
}

/*
 * Send out the same buffer upon completion of DMA
 */

void lcddma_isr(int irq, void *dev_id, struct pt_regs *regs) {

  /* Clear DMA interrupt */
  disable_dma(CHAN);

  /* Do DMA write to i/o operation */
  set_dma_mode(CHAN, DMA_MODE_WRITE_WORD);
  set_dma_device_addr(CHAN, DMA_DEST_ADDR);
  set_dma_addr(CHAN, (unsigned int)dma_buf);
  set_dma_count(CHAN, dma_xfer_len);

  /* Fire it off! */
  enable_dma(CHAN);
  
}


/*
 * Init the interrupt vector location, and setup timer 2
 */


void lcddma_init(void) {
  volatile unsigned char *regp;
  volatile unsigned short *shortp;
  int i;
  int result;

  /* Init dma buffer */
  for (i= 0; i < 16; i++) {
    dma_buf[i] = twiddle_array[i] | i;
  }

  /* Register lcddma as character device */
  result = register_chrdev(LCDDMA_MAJOR, "lcddma", &lcddma_fops);
  if (result < 0) {
    printk(KERN_WARNING "LCDDMA: can't get major %d\n",LCDDMA_MAJOR);
    return;
  }

  /* Set interrupt level and priority */
  /* Note: this is hardwired for channel 0 */
  regp = (volatile unsigned char *) (MCF_MBAR);
  regp[MCFSIM_DMA1ICR] = MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI3;
  mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_DMA1);

  /* Set interrupt vector location */
  /* Note: this is hardwired for channel 0 */
  regp[MCFDMA_BASE0 + MCFDMA_DIVR] = DMA_VEC_LOC;

  /* Install ISR (interrupt service routine) */
  printk ("lcddma: requesting IRQ %d\n", DMA_VEC_LOC);
  result = request_irq(DMA_VEC_LOC, lcddma_isr, SA_INTERRUPT, "lcddma", NULL);
  if (result) {
    printk ("lcddma: IRQ %d already in use\n", DMA_VEC_LOC);
    return;
  }

  /* Request DMA channel */
  printk ("lcddma: requesting DMA channel %d\n", CHAN);
  result = request_dma(CHAN, "lcddma");
  if (result) {
    printk ("lcddma: dma channel %d already in use\n", CHAN);
    return;
  }

  /* Setup timer 2 as LCD pixel clock */
  shortp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE2);

  shortp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;
  shortp[MCFTIMER_TER] = MCFTIMER_TER_CAP | MCFTIMER_TER_REF;
  shortp[MCFTIMER_TRR] = (unsigned short) ((MCF_CLK/10000) / HZ );
  shortp[MCFTIMER_TMR] = (0 << 8) | /* set prescaler to divide by 1 */
                        MCFTIMER_TMR_DISCE | MCFTIMER_TMR_DISOM |
                        MCFTIMER_TMR_ENORI | MCFTIMER_TMR_RESTART |
                        MCFTIMER_TMR_CLK1  | MCFTIMER_TMR_ENABLE;

  /* Enable external DMA requests on TIN0/DREQ0 pin */
#define MCFSIM_PAR_PAR8 (1 << 8)
  shortp = (volatile unsigned short *)(MCF_MBAR + MCFSIM_PAR);
  shortp[0] |= MCFSIM_PAR_PAR8;
}
