/* -*- linux-c -*-
 *
 * JFFS -- Journaling Flash File System, Linux implementation.
 *
 * Copyright (C) 2000  Axis Communications AB.
 *
 * Created by Simon Kagstrom <simonk@axis.com>.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Id: jffs_proc.c,v 1.1 2000/10/06 08:40:31 davidm Exp $
 *
 */

/*
 * TODO:
 *
 * 1. Create some more proc-files for different kinds of info, i.e.
 *    jffs_flash_structure for the current one,
 *    jffs_flash_status for info on if jffs is GC'ing and so on.
 * 2. Move to the /proc/fs/ subdir (2.2/2.4-kernels).
 *
 */

/* jffs_proc.c -- This file defines jffs's entry in the proc file system.  */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/jffs.h>
#include <linux/fs.h>
#include <linux/locks.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/stat.h>
#include <linux/blkdev.h>
#include <asm/byteorder.h>
#include <linux/proc_fs.h>

#include "jffs_proc.h"
#include "jffs_fm.h"

#if defined(CONFIG_JFFS_FS_VERBOSE) && CONFIG_JFFS_FS_VERBOSE
#define D(x) x
#else
#define D(x)
#endif
#define D1(x)
#define D2(x)
#define D3(x)
#define ASSERT(x) x

/* The super blocks from VFS.  */
extern struct super_block super_blocks[NR_SUPER];

/* This function is called upon when someone accesses the file in the
   proc-file system.  */
static int
jffs_procfile_read(char *buffer, char **buffer_location, off_t offset,
		   int length, int dummy)
{
	int i;
	off_t pos = 0;
	off_t begin = 0;
        int len = 0;  /* The number of bytes actually used.   */
	char *chunk_state = NULL;
	struct jffs_control *c;
	struct jffs_fm *fm;

	/* Seek the first jffs-file system.  */
	for (i = 0; i < NR_SUPER; i++) {
		/* This allows for one jffs only! FIX!  */
		if (strstr(super_blocks[i].s_type->name, "jffs")) {
			/* Get the jffs_control struct from the superblock.  */
			c = (struct jffs_control *)
			    super_blocks[i].u.generic_sbp;
			break;
		}
	}

	fm = c->fmc->head;
	if (c->fmc->no_call_gc == 1) {
		chunk_state = "yes";
	}
	else {
		chunk_state = "no";
	}
	len += sprintf(buffer + len, "flash size:\n%d\n", c->fmc->flash_size);
	len += sprintf(buffer + len, "sector size:\n%d\n", c->fmc->sector_size);
	len += sprintf(buffer + len, "used size:\n%d\n", c->fmc->used_size);
	len += sprintf(buffer + len, "dirty size:\n%d\n", c->fmc->dirty_size);
	len += sprintf(buffer + len, "garbage collecting:\n%s\n", chunk_state);

	len += sprintf(buffer + len, "jffs flash structure:\n"
		       "(Offset, size and state)\n");

	while (fm != NULL) {
		/* used.  */
		if (fm->nodes != NULL) {
			chunk_state = "clean";
		}
		else {
			chunk_state = "**dirty**";
		}

		len += sprintf(buffer + len,
			       "---node---\n"
			       "%lu\n"
			       "%lu\n"
			       "%s\n",
			       fm->offset - c->fmc->flash_start, fm->size,
			       chunk_state);

		D3(printk("---node---\n"
			  "%lu\n"
			  "%lu\n"
			  "%s\n",
			  fm->offset-c->fmc->flash_start, fm->size,
			  chunk_state));
		pos = begin + len; /* Length here is...
				      sort of strange...   */
		if (pos < offset) {
			len = 0;
			begin = pos;
		}
		if (pos > offset + length) {
			break;
		}
		fm = fm->next;
	}

	*buffer_location = buffer + (offset - begin);
	len -= (offset - begin);
	if (len > length) {
		len = length;
	}
	if (len < 0) {
		len = 0;
	}
	D3(printk("offset: %d, len: %d, buffer_length: %d\n",
		  offset, len, length));
	/* Return the length.  */
	return len;
} /* jffs_procfile_read */


/* This is the struct for jffs's proc entry.  */
struct proc_dir_entry jffs_proc_file =
{
	0,                   /* Inode number - ignore, it will be filled by
				proc_register[_dynamic].  */
	9,                   /* Length of the file name.  */
	"jffs_test",         /* The file name.  */
	S_IFREG | S_IRUGO,   /* File mode - this is a regular file which
				can be read by its owner, its group, and
				everybody else.  */
	1,                   /* Number of links (directories where the file
				is referenced).  */
	0, 0,                /* The uid and gid for the file - we give it
				to root.  */
	0,                   /* The size of the file reported by ls.  */
	NULL,                /* functions which can be done on the inode
				(linking, removing, etc.) - we don't support
				any. */
	jffs_procfile_read,  /* The read function for this file, the
				function called when somebody tries to
				read something from it. */
	NULL
};
