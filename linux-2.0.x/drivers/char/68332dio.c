/* 
 * 68332dio.c: Driver to use ports as dio's 
 * 	       used as a sample
 *
 * Copyright (C) 2003  Gerold Boehler <gboehler@mail.austria.at>
 *
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <asm/MC68332.h>
#include <asm/68332dio.h>



#ifdef CONFIG_68332_PORTF

static int portf_read(struct inode *inode, struct file *file, char *buffer, int count) {
	*buffer = PORTF & count;
	return 1;
}

static int portf_write(struct inode *inode, struct file *file, const char *buffer, int count) {
	if (*buffer)	PORTF |= count;
	else		PORTF &= ~count;
	return 1;
}

/*
 * cmd tells ioctl to init the pin within arg as in or output
 */

static int portf_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {

	char pin = (char)arg;

	PEPAR = 0;					/* all signals belong to portf */

	switch ( cmd ) {
		case PORTFSOUT	:	DDRE |= pin;	return 0;
		case PORTFSIN	:	DDRE &= ~pin;	return 0;
	}
	return -1;
}

static struct file_operations portf_fops = {
	NULL,			/* lseek */
	portf_read,		/* read */
	portf_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	portf_ioctl,		/* ioctl */
	NULL,			/* open */
	NULL			/* release */
};

int portf_68332_init(void) {
	int err;

	if ((err = register_chrdev(PORTF_MAJOR, "portf", &portf_fops)) < 0)
		printk("Couldn't register /dev/portf\n");

	return err;
}

#endif /* CONFIG_68332_PORTF */



#ifdef CONFIG_68332_TPU

static int tpu_read(struct inode *inode, struct file *file, char *buffer, int count) {
	return 0;
}

static int tpu_write(struct inode *inode, struct file *file, const char *buffer, int count) {
	return 0;
}

static int tpu_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	return 0;
}

static struct file_operations tpu_fops = {
	NULL,			/* lseek */
	tpu_read,		/* read */
	tpu_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	tpu_ioctl,		/* ioctl */
	NULL,			/* open */
	NULL			/* release */
};

int tpu_68332_init(void) {
	int err;

	if ((err = register_chrdev(TPU_MAJOR, "tpu", &tpu_fops)) < 0)
		printk("Couldn't register /dev/tpu\n");

	return err;
}

#endif /* CONFIG_68332_TPU */



#ifdef CONFIG_68332_PORTE

static int porte_read(struct inode *inode, struct file *file, char *buffer, int count) {
	*buffer = PORTE & count;
	return 1;
}

static int porte_write(struct inode *inode, struct file *file, const char *buffer, int count) {
	if (*buffer)	PORTE |= count;
	else		PORTE &= ~count;
	return 1;
}

/*
 * cmd tells ioctl to init the pin within arg as in or output
 */

static int porte_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {

	char pin = (char)arg;

	PEPAR = 6;					/* don't use avec and pin1 */

	switch ( cmd ) {
		case PORTESOUT	:	DDRE |= pin;	return 0;
		case PORTESIN	:	DDRE &= ~pin;	return 0;
	}
	return -1;
}

static struct file_operations porte_fops = {
	NULL,			/* lseek */
	porte_read,		/* read */
	porte_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	porte_ioctl,		/* ioctl */
	NULL,			/* open */
	NULL			/* release */
};

int porte_68332_init(void) {
	int err;

	if ((err = register_chrdev(PORTE_MAJOR, "porte", &porte_fops)) < 0)
		printk("Couldn't register /dev/porte\n");

	return err;
}

#endif /* CONFIG_68332_PORTE */
