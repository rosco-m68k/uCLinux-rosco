#ifndef __KERNEL__
#  define __KERNEL__
#endif

#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#else
#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT
#endif

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/sched.h>
#include <linux/spi.h> 
#include <linux/malloc.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/unistd.h>


#include <asm/io.h>
#include <asm/segment.h>
#include <asm/system.h>

#if !defined(SEEK_SET)
#define SEEK_SET 0
#endif


inline int SPI_Transfer( int address, int data );
inline int SPI_Recv_Byte( void );
inline int SPI_Send_Byte( int byte );


static unsigned int   spi_major = 60; /* a local major, can be overwritten */
static          int   openflag  =  0;
static np_spi * const spi_ptr   = na_spi;

                    /*******************************/
                    /* SPI data transfer routines. */
                    /*******************************/

#define SPI_XMIT_READY 0x40
#define SPI_RECV_READY 0x80

// Sends an address and byte, and then waits to receive data
inline int SPI_Transfer( int address, int data )
{
	int value;

	value = ((address & 0xFF) << 8) | (data & 0xFF);
	SPI_Send_Byte( value );

	for ( value = SPI_Recv_Byte(); value == -1; value = SPI_Recv_Byte() )
	    ;

	return value;
}

// returns -1 if there is no data present, otherwise returns
// the value
inline int SPI_Recv_Byte( void )
{
        return (spi_ptr->np_spistatus & SPI_RECV_READY) ? spi_ptr->np_spirxdata : -1;
}


// Sends the 16 bit address+data
inline int SPI_Send_Byte( int byte )
{
	int counter;

	for ( counter = 11; counter; counter-- )
	  	if ( spi_ptr->np_spistatus & SPI_XMIT_READY )
			break;

	if (counter < 0)
		return -1;

	spi_ptr->np_spitxdata = byte;

	return 0;
}



                    /*************************/
                    /* SPI Driver functions. */
                    /*************************/

int spi_reset( void )
{
  // Nothing to do: The Nios does _not_
  // support burst xfers. Chip Enables
  // (Selects) are inactive after each xfer.
  return 0;
}

int spi_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
  // Nothing to do.
  return 0;
}			



/***************************************************/
/* The SPI Write routine. The first 8 bits are     */
/* the device register, and the rest of the buffer */
/* is data.                                        */
/***************************************************/
int spi_write( struct inode *inode, struct file *file, const char *buf, int count )
{
  unsigned char *temp;                 /* Our pointer to the buffer */
  int            i;
  int            addr;
  
  if ( count < 3 )
    return -EINVAL;          	       /* Address is 2 bytes: Must have _something_ to send */

  temp = (char *)buf;
  addr = (int)*((u16 *)temp);          /* chip register address.  */
  temp += sizeof(u16);

  for ( i = count - sizeof(u16); i; i--, temp++ )
      *temp = (unsigned char)SPI_Transfer( addr, (int)*temp );
    
  
  return count;                        /* we can always send all data */
}



int spi_lseek ( struct inode *inode, struct file *file, off_t offset, int origin )
{
#if 0
  int     bit_count, i;
#endif

  if ( origin != SEEK_SET || offset != (offset & 0xFFFF) )
  {
      errno = -EINVAL;
      return -1;
  }

#if 0
  /****************************************/
  /* Nios SPI implementation safeguard:   */
  /* It is possible to have more than     */
  /* one CS active at a time. Check that  */
  /* the given address is a power of two. */
  /****************************************/
  bit_count = 0;
  for ( i = 0; i < sizeof(u16); i++ )
  {
      if ( (1 << i) & offset )
      {
	  if ( ++bit_count > 1 )
	  {
	      errno = -EINVAL;
	      return -1;
	  }
      }
  }
#endif
  spi_ptr->np_spislaveselect = offset;
  return 0;
}

int spi_open( struct inode *inode, struct file *file )
{
  if ( openflag )
    return -EBUSY;

  MOD_INC_USE_COUNT;
  openflag = 1;

  return 0;
}

void spi_release(struct inode * inode, struct file * file)
{
  openflag = 0;
  MOD_DEC_USE_COUNT;
}


/* static struct file_operations spi_fops  */

static struct file_operations spi_fops = {
  spi_lseek,     /* Set chip-select line. The offset is used as an address. */
  spi_write,     /* spi_read and spi_write are the same */
  spi_write,
  NULL,          /* spi_readdir */
  NULL,          /* spi_select */
  spi_ioctl,	
  NULL,          /* spi_mmap */
  spi_open, 
  spi_release,
  NULL,
  NULL,
  NULL,
  NULL
#ifdef MAGIC_ROM_PTR
  , NULL
#endif
};


int register_NIOS_SPI( void )
{
  int result = register_chrdev( spi_major, "spi", &spi_fops );
  if ( result < 0 )
  {
    printk( "SPI: unable to get major %d for SPI bus \n", spi_major );
    return result;
  }/*end-if*/
  
  if ( spi_major == 0 )
    spi_major = result; /* here we got our major dynamically */

  /* reserve our port, but check first if free */
  if ( check_region( (unsigned int)na_spi, sizeof(np_spi) ) )
  {
    printk( "SPI: port at adr 0x%08x already occupied\n", (unsigned int)na_spi );
    unregister_chrdev( spi_major, "spi" );

    return result;
  }/*end-if*/

  return 0;
}

void unregister_NIOS_SPI( void )
{
    if ( spi_major > 0 )
      unregister_chrdev( spi_major, "spi" );

    release_region( (unsigned int)na_spi, sizeof(np_spi) );
}


#ifdef MODULE
void cleanup_module( void )
{
  unregister_NIOS_SPI();
}



int init_module( void )
{
  return register_NIOS_SPI();
}
#endif
