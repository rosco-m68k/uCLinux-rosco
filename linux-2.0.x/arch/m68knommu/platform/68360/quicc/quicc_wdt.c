/*
 *      68360 System Integration Module Watchdog Timer Device Driver.
 *
 *      Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *              <hamilton@sedsystems.ca>
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <asm/system.h>
#include <asm/quicc_cpm.h>
#include <asm/m68360.h>

static int wdt_lseek(struct inode *in, struct file *f, off_t o, int i)
{
        return(-ESPIPE);
}

static int wdt_write(struct inode *inode, struct file *filp,
                const char *buf, int count)
{
#if defined(CONFIG_M68360_SIM_WDT_55_AA)
        int i;
        unsigned char local;

        if(count <= 0)
                return(count);
        if(buf == NULL)
                return(0);

        for(i = 0; i < count; ++i)
        {
                memcpy_fromfs(&local, &(buf[i]), 1);
                pquicc->sim_swsr = local;
        }
        return(count);
#else
        /* Kick the watchdog timer. */
        quicc_kick_wdt();
        return(count);
#endif
}

static int wdt_ioctl(struct inode *i, struct file *f, unsigned int c,
                unsigned long a)
{
        return(-EINVAL);
}

int quicc_sim_wdt_init(void)
{
        static struct file_operations wdt_fops =
        {
                lseek:          wdt_lseek,
                write:          wdt_write,
                ioctl:          wdt_ioctl
        };
        static struct miscdevice wdt_miscdev =
        {
                WATCHDOG_MINOR,
                "wdt",
                &wdt_fops
        };
        printk("QUICC Watchdog Timer By SED Systems,"
                       " A Division of Calian Ltd.\n");
        misc_register(&wdt_miscdev);
        return(0);
}
