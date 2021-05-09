/*
 * JFFS -- Journaling Flash File System, Linux implementation.
 *
 * Copyright (C) 1999, 2000  Axis Communications AB.
 *
 * Created by Finn Hakansson <finn@axis.com>.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Id: inode.c,v 1.2 2000/10/06 08:39:31 davidm Exp $
 *
 */

/* inode.c -- Contains the code that is called from the VFS.  */

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
#if CONFIG_JFFS_PROC_FS
#include <linux/proc_fs.h>
#endif

#include "jffs_fm.h"
#include "intrep.h"
#if CONFIG_JFFS_PROC_FS
#include "jffs_proc.h"
#endif

#if defined(CONFIG_JFFS_FS_VERBOSE) && CONFIG_JFFS_FS_VERBOSE
#define D(x) x
#else
#define D(x)
#endif
#define D1(x)
#define D2(x)
#define D3(x)
#define ASSERT(x) x

static int jffs_remove(struct inode *dir, const char *name,
		       int len, int type, int must_iput);

static struct super_operations jffs_ops;
static struct file_operations jffs_file_operations;
static struct inode_operations jffs_file_inode_operations;
static struct file_operations jffs_dir_operations;
static struct inode_operations jffs_dir_inode_operations;
static struct inode_operations jffs_symlink_inode_operations;


/* Called by the VFS at mount time to initialize the whole file system.  */
static struct super_block *
jffs_read_super(struct super_block *sb, void *data, int silent)
{
	kdev_t dev = sb->s_dev;

	printk(KERN_NOTICE "JFFS: Trying to mount device %s.\n",
	       kdevname(dev));

	MOD_INC_USE_COUNT;
	lock_super(sb);
	set_blocksize(dev, BLOCK_SIZE);
	sb->s_blocksize = BLOCK_SIZE;
	sb->s_blocksize_bits = BLOCK_SIZE_BITS;
	sb->u.generic_sbp = (void *) 0;

	/* Build the file system.  */
	if (jffs_build_fs(sb) < 0) {
		goto jffs_sb_err1;
	}
	sb->s_magic = JFFS_MAGIC_SB_BITMASK;
	sb->s_op = &jffs_ops;

	/* Get the root directory of this file system.  */
	if (!(sb->s_mounted = iget(sb, JFFS_MIN_INO))) {
		goto jffs_sb_err2;
	}

#ifdef USE_GC
	/* Do a garbage collect every time we mount.  */
	jffs_garbage_collect((struct jffs_control *)sb->u.generic_sbp);
#endif

	unlock_super(sb);
	printk(KERN_NOTICE "JFFS: Successfully mounted device %s.\n",
	       kdevname(dev));
	return sb;

jffs_sb_err2:
	jffs_cleanup_control((struct jffs_control *)sb->u.generic_sbp);
jffs_sb_err1:
	unlock_super(sb);
	MOD_DEC_USE_COUNT;
	printk(KERN_WARNING "JFFS: Failed to mount device %s.\n",
	       kdevname(dev));
	return 0;
}


/* This function is called when the file system is umounted.  */
static void
jffs_put_super(struct super_block *sb)
{
	kdev_t dev = sb->s_dev;
	D2(printk("jffs_put_super()\n"));
	lock_super(sb);
	sb->s_dev = 0;
	jffs_cleanup_control((struct jffs_control *)sb->u.generic_sbp);
	unlock_super(sb);
	MOD_DEC_USE_COUNT;
	printk(KERN_NOTICE "JFFS: Successfully unmounted device %s.\n",
	       kdevname(dev));
}


/* This function is called when user commands like chmod, chgrp and
   chown are executed. System calls like trunc() results in a call
   to this function.  */
static int
jffs_notify_change(struct inode *inode, struct iattr *iattr)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_fmcontrol *fmc;
	struct jffs_file *f;
	struct jffs_node *new_node;
	char *name = 0;
	int update_all;
	int res;

	f = (struct jffs_file *)inode->u.generic_ip;
	ASSERT(if (!f) {
		printk("jffs_notify_change(): Invalid inode number: %lu\n",
		       inode->i_ino);
		return -1;
	});

	D1(printk("***jffs_notify_change(): file: \"%s\", ino: %u\n",
		  f->name, f->ino));

	c = f->c;
	fmc = c->fmc;
	update_all = iattr->ia_valid & ATTR_FORCE;

	if (!JFFS_ENOUGH_SPACE(fmc)) {
		if (((update_all || iattr->ia_valid & ATTR_SIZE)
		    && (iattr->ia_size < f->size))) {
			/* See this case where someone is trying to
			   shrink the size of a file as an exception.
			   Accept it.  */
		}
		else {
			D1(printk("jffs_notify_change(): Free size = %u\n",
				  jffs_free_size1(fmc)
				  + jffs_free_size2(fmc)));
			D(printk(KERN_NOTICE "JFFS: No space left on "
				 "device\n"));
			return -ENOSPC;
		}
	}

	if (!(new_node = (struct jffs_node *)
			 kmalloc(sizeof(struct jffs_node), GFP_KERNEL))) {
		D(printk("jffs_notify_change(): Allocation failed!\n"));
		return -ENOMEM;
	}
	DJM(no_jffs_node++);
	new_node->data_offset = 0;
	new_node->removed_size = 0;
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = f->ino;
	raw_inode.pino = f->pino;
	raw_inode.version = f->highest_version + 1;
	raw_inode.mode = f->mode;
	raw_inode.uid = f->uid;
	raw_inode.gid = f->gid;
	raw_inode.atime = f->atime;
	raw_inode.mtime = f->mtime;
	raw_inode.ctime = f->ctime;
	raw_inode.dsize = 0;
	raw_inode.offset = 0;
	raw_inode.rsize = 0;
	raw_inode.dsize = 0;
	raw_inode.nsize = 0;
	raw_inode.nlink = f->nlink;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	if (update_all || iattr->ia_valid & ATTR_MODE) {
		raw_inode.mode = iattr->ia_mode;
		inode->i_mode = iattr->ia_mode;
	}
	if (update_all || iattr->ia_valid & ATTR_UID) {
		raw_inode.uid = iattr->ia_uid;
		inode->i_uid = iattr->ia_uid;
	}
	if (update_all || iattr->ia_valid & ATTR_GID) {
		raw_inode.gid = iattr->ia_gid;
		inode->i_gid = iattr->ia_gid;
	}
	if (update_all || iattr->ia_valid & ATTR_SIZE) {
		int len;
		D1(printk("jffs_notify_change(): Changing size "
			  "to %lu bytes!\n", iattr->ia_size));
		raw_inode.offset = iattr->ia_size;

		/* Calculate how many bytes need to be removed from
		   the end.  */

		if (f->size < iattr->ia_size) {
			len = 0;
		}
		else {
			len = f->size - iattr->ia_size;
		}

		raw_inode.rsize = len;

		/* The updated node will be a removal node, with
		   base at the new size and size of the nbr of bytes
		   to be removed.  */

		new_node->data_offset = iattr->ia_size;
		new_node->removed_size = len;
		inode->i_size = iattr->ia_size;

		/* If we truncate a file we want to add the name.  If we
		   always do that, we could perhaps free more space on
		   the flash (and besides it doesn't hurt).  */
		name = f->name;
		raw_inode.nsize = f->nsize;
		if (len) {
			invalidate_inode_pages(inode);
		}
		inode->i_ctime = CURRENT_TIME;
		inode->i_mtime = inode->i_ctime;
	}
	if (update_all || iattr->ia_valid & ATTR_ATIME) {
 		raw_inode.atime = iattr->ia_atime;
		inode->i_atime = iattr->ia_atime;
	}
	if (update_all || iattr->ia_valid & ATTR_MTIME) {
		raw_inode.mtime = iattr->ia_mtime;
		inode->i_mtime = iattr->ia_mtime;
	}
	if (update_all || iattr->ia_valid & ATTR_CTIME) {
		raw_inode.ctime = iattr->ia_ctime;
		inode->i_ctime = iattr->ia_ctime;
	}

	/* Write this node to the flash.  */
	if ((res = jffs_write_node(c, new_node, &raw_inode, name, 0)) < 0) {
		D(printk("jffs_notify_change(): The write failed!\n"));
		kfree(new_node);
		DJM(no_jffs_node--);
		return res;
	}

	jffs_insert_node(c, f, &raw_inode, 0, new_node);
	inode->i_dirt = 1;
	return 0;
} /* jffs_notify_change()  */


/* Get statistics of the file system.  */
static void
jffs_statfs(struct super_block *sb, struct statfs *buf, int bufsize)
{
	struct statfs tmp;
	struct jffs_control *c = (struct jffs_control *) sb->u.generic_sbp;
	struct jffs_fmcontrol *fmc = c->fmc;

	D2(printk("jffs_statfs()\n"));

	c = (struct jffs_control *)sb->u.generic_sbp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.f_type = JFFS_MAGIC_SB_BITMASK;
	tmp.f_bsize = PAGE_SIZE;
	tmp.f_blocks = (fmc->flash_size / BLOCK_SIZE)
		       - (fmc->min_free_size / BLOCK_SIZE);
	tmp.f_bfree = (jffs_free_size1(fmc) / BLOCK_SIZE
		       + jffs_free_size2(fmc) / BLOCK_SIZE)
		      - (fmc->min_free_size / BLOCK_SIZE);
	/* Find out how many files there are in the filesystem.  */
	tmp.f_files = jffs_foreach_file(c, jffs_file_count);
	tmp.f_ffree = tmp.f_bfree;
	/* tmp.f_fsid = 0; */
	tmp.f_namelen = JFFS_MAX_NAME_LEN;
	memcpy_tofs(buf, &tmp, bufsize);
}


/* Rename a file.  */
int
jffs_rename(struct inode *old_dir, const char *old_name, int old_len,
            struct inode *new_dir, const char *new_name, int new_len,
            int must_be_dir)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_file *old_dir_f;
	struct jffs_file *new_dir_f;
	struct jffs_file *del_f;
	struct jffs_file *f;
	struct jffs_node *node;
	struct inode *inode;
	int result = 0;
	__u32 rename_data = 0;

	D2(printk("***jffs_rename()\n"));

	if (!old_dir || !old_name || !new_dir || !new_name) {
		D(printk("jffs_rename(): old_dir: 0x%p, old_name: 0x%p, "
			 "new_dir: 0x%p, new_name: 0x%p\n",
			 old_dir, old_name, new_dir, new_name));
		return -1;
	}

	c = (struct jffs_control *)old_dir->i_sb->u.generic_sbp;
	ASSERT(if (!c) {
		printk(KERN_ERR "jffs_rename(): The old_dir inode "
		       "didn't have a reference to a jffs_file struct\n");
		return -1;
	});

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_rename(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		return -ENOSPC;
	}

	while (c->rename_lock) {
		sleep_on(&c->rename_wait);
	}
	c->rename_lock = 1;

	/* Check the lengths of the names.  */
	if ((old_len > JFFS_MAX_NAME_LEN) || (new_len > JFFS_MAX_NAME_LEN)) {
		result = -ENAMETOOLONG;
		goto jffs_rename_end;
	}
	/* Find the old directory.  */
	if (!(old_dir_f = (struct jffs_file *)old_dir->u.generic_ip)) {
		D(printk("jffs_rename(): Old dir invalid.\n"));
		result = -ENOTDIR;
		goto jffs_rename_end;
	}
	/* See if it really is a directory.  */
	if (!S_ISDIR(old_dir_f->mode)) {
		D(printk("jffs_rename(): old_dir is not a directory.\n"));
		result = -ENOTDIR;
		goto jffs_rename_end;
	}
	/* Try to find the file to move.  */
	if (!(f = jffs_find_child(old_dir_f, old_name, old_len))) {
		result = -ENOENT;
		goto jffs_rename_end;
	}
	/* Find the new directory.  */
	if (!(new_dir_f = (struct jffs_file *)new_dir->u.generic_ip)) {
		D(printk("jffs_rename(): New dir invalid.\n"));
		result = -ENOTDIR;
		goto jffs_rename_end;
	}
	/* See if the node really is a directory.  */
	if (!S_ISDIR(new_dir_f->mode)) {
		D(printk("jffs_rename(): The new position of the node "
			 "is not a directory.\n"));
		result = -ENOTDIR;
		goto jffs_rename_end;
	}

	/* Create a node and initialize as much as needed.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_rename(): Allocation failed: node == 0\n"));
		result = -ENOMEM;
		goto jffs_rename_end;
	}
	DJM(no_jffs_node++);
	node->data_offset = 0;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = f->ino;
	raw_inode.pino = new_dir_f->ino;
	raw_inode.version = f->highest_version + 1;
	raw_inode.mode = f->mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
#if 0
	raw_inode.uid = f->uid;
	raw_inode.gid = f->gid;
#endif
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = f->ctime;
	raw_inode.offset = 0;
	raw_inode.dsize = 0;
	raw_inode.rsize = 0;
	raw_inode.nsize = new_len;
	raw_inode.nlink = f->nlink;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* See if there already exists a file with the same name as
	   new_name.  */
	if ((del_f = jffs_find_child(new_dir_f, new_name, new_len))) {
		raw_inode.rename = 1;
		raw_inode.dsize = sizeof(__u32);
		rename_data = del_f->ino;
	}

	/* Write the new node to the flash memory.  */
	if ((result = jffs_write_node(c, node, &raw_inode, new_name,
				      (unsigned char*)&rename_data)) < 0) {
		D(printk("jffs_rename(): Failed to write node to flash.\n"));
		kfree(node);
		DJM(no_jffs_node--);
		goto jffs_rename_end;
	}
	raw_inode.dsize = 0;

	if (raw_inode.rename) {
		/* The file with the same name must be deleted.  */
		c->fmc->no_call_gc = 1;
		if ((result = jffs_remove(new_dir, new_name, new_len,
				          del_f->mode, 0)) < 0) {
			/* This is really bad.  */
			printk(KERN_ERR "JFFS: An error occurred in "
			       "rename().\n");
		}
		c->fmc->no_call_gc = 0;
	}

	if (old_dir_f != new_dir_f) {
		/* Remove the file from its old position in the
		   filesystem tree.  */
		jffs_unlink_file_from_tree(f);
	}

	/* Insert the new node into the file system.  */
	if ((result = jffs_insert_node(c, f, &raw_inode,
				       new_name, node)) < 0) {
		D(printk(KERN_ERR "jffs_rename(): jffs_insert_node() "
			 "failed!\n"));
	}

	if (old_dir_f != new_dir_f) {
		/* Insert the file to its new position in the
		   file system.  */
		jffs_insert_file_into_tree(f);
	}

	/* This is a kind of update of the inode we're about to make
	   here.  This is what they do in ext2fs.  Kind of.  */
	if ((inode = iget(new_dir->i_sb, f->ino))) {
		inode->i_ctime = CURRENT_TIME;
		inode->i_dirt = 1;
		iput(inode);
	}

jffs_rename_end:
	iput(old_dir);
	iput(new_dir);
	c->rename_lock = 0;
	wake_up(&c->rename_wait);
	return result;
} /* jffs_rename()  */


/* Read the contents of a directory.  Used by programs like `ls'
   for instance.  */
static int
jffs_readdir(struct inode *dir, struct file *filp, void *dirent,
	     filldir_t filldir)
{
	struct jffs_file *f;
	int j;
	int ddino;

	D2(printk("jffs_readdir(): dir: 0x%p, filp: 0x%p\n", dir, filp));

	if (!dir || !S_ISDIR(dir->i_mode)) {
		D(printk("jffs_readdir(): 'dir' is NULL or not a dir!\n"));
		return -EBADF;
	}

	switch (filp->f_pos)
	{
	case 0:
		D3(printk("jffs_readdir(): \".\" %lu\n", dir->i_ino));
		if (filldir(dirent, ".", 1, filp->f_pos, dir->i_ino) < 0) {
			return 0;
		}
		filp->f_pos = 1;
	case 1:
		if (dir->i_ino == JFFS_MIN_INO) {
			ddino = dir->i_sb->s_covered->i_ino;
		}
		else {
			ddino = ((struct jffs_file *)dir->u.generic_ip)->pino;
		}
		D3(printk("jffs_readdir(): \"..\" %u\n", ddino));
		if (filldir(dirent, "..", 2, filp->f_pos, ddino) < 0)
			return 0;
		filp->f_pos++;
	default:
		f = ((struct jffs_file *)dir->u.generic_ip)->children;
		for (j = 2; (j < filp->f_pos) && f; j++) {
			f = f->sibling_next;
		}
		for (; f ; f = f->sibling_next) {
			D3(printk("jffs_readdir(): \"%s\" ino: %u\n",
				  (f->name ? f->name : ""), f->ino));
			if (filldir(dirent, f->name, f->nsize,
				    filp->f_pos , f->ino) < 0)
				return 0;
			filp->f_pos++;
		}
	}
	return filp->f_pos;
} /* jffs_readdir()  */


/* Find a file in a directory. If the file exists, return its
   corresponding inode in the argument `result'.  */
static int
jffs_lookup(struct inode *dir, const char *name, int len,
            struct inode **result)
{
	struct jffs_file *d;
	struct jffs_file *f;
	int r = 0;

	D3({
		char *s = (char *)kmalloc(len + 1, GFP_KERNEL);
		memcpy(s, name, len);
		s[len] = '\0';
		printk("jffs_lookup(): dir: 0x%p, name: \"%s\"\n", dir, s);
		kfree(s);
	});

	*result = (struct inode *)0;
	if (!dir) {
		return -ENOENT;
	}
	if (!S_ISDIR(dir->i_mode)) {
		r = -ENOTDIR;
		goto jffs_lookup_end;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		r = -ENAMETOOLONG;
		goto jffs_lookup_end;
	}

	if (!(d = (struct jffs_file *)dir->u.generic_ip)) {
		D(printk("jffs_lookup(): No such inode! (%lu)\n",
			 dir->i_ino));
		r = -ENOENT;
		goto jffs_lookup_end;
	}

	/* Get the corresponding inode to the file.  */
	if ((len == 1) && (name[0] == '.')) {
		if (!(*result = iget(dir->i_sb, d->ino))) {
			D(printk("jffs_lookup(): . iget() ==> NULL\n"));
			r = -ENOENT;
		}
	}
	else if ((len == 2) && (name[0] == '.') && (name[1] == '.')) {
		if (!(*result = iget(dir->i_sb, d->pino))) {
			D(printk("jffs_lookup(): .. iget() ==> NULL\n"));
			r = -ENOENT;
		}
	}
	else if ((f = jffs_find_child(d, name, len))) {
		if (!(*result = iget(dir->i_sb, f->ino))) {
			D(printk("jffs_lookup(): iget() ==> NULL\n"));
			r = -ENOENT;
		}
	}
	else {
		D3(printk("jffs_lookup(): Couldn't find the file. "
			  "f = 0x%p, name = \"%s\", d = 0x%p, d->ino = %u\n",
			  f, name, d, d->ino));
		r = -ENOENT;
	}

jffs_lookup_end:
	iput(dir);
	return r;
} /* jffs_lookup()  */


/* Try to read a page of data from a file.  */
static int
jffs_readpage(struct inode *inode, struct page *page)
{
	unsigned long buf;
	unsigned long read_len;
	int result = -EIO;
	struct jffs_file *f = (struct jffs_file *)inode->u.generic_ip;
	int r;

	D2(printk("***jffs_readpage(): file = \"%s\", page->offset = %lu\n",
		  (f->name ? f->name : ""), page->offset));

	page->count++;
	set_bit(PG_locked, &page->flags);
	buf = page_address(page);
	clear_bit(PG_uptodate, &page->flags);
	clear_bit(PG_error, &page->flags);

	if (page->offset < inode->i_size) {
		read_len = jffs_min(inode->i_size - page->offset, PAGE_SIZE);
		r = jffs_read_data(f, (char *)buf, page->offset, read_len);
		if (r == read_len) {
			if (read_len < PAGE_SIZE) {
				memset((void *)(buf + read_len), 0,
				       PAGE_SIZE - read_len);
			}
			set_bit(PG_uptodate, &page->flags);
			result = 0;
		}
		D(else {
			printk("***jffs_readpage(): Read error! "
			       "Wanted to read %lu bytes but only "
			       "read %d bytes.\n", read_len, r);
		});
	}
	if (result) {
		set_bit(PG_error, &page->flags);
		memset((void *)buf, 0, PAGE_SIZE);
	}

	clear_bit(PG_locked, &page->flags);
	wake_up(&page->wait);
	free_page(buf);

	D3(printk("jffs_readpage(): Leaving...\n"));

	return result;
} /* jffs_readpage()  */


/* Create a new directory.  */
static int
jffs_mkdir(struct inode *dir, const char *name, int len, int mode)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_node *node;
	struct jffs_file *dir_f;
	int dir_mode;
	int result = 0;

	D1({
		char *_name = (char *) kmalloc(len + 1, GFP_KERNEL);
		memcpy(_name, name, len);
		_name[len] = '\0';
		printk("***jffs_mkdir(): dir = 0x%p, name = \"%s\", "
		       "len = %d, mode = 0x%08x\n", dir, _name, len, mode);
		kfree(_name);
	});

	if (!dir) {
		return -ENOENT;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		result = -ENAMETOOLONG;
		goto jffs_mkdir_end;
	}

	dir_f = (struct jffs_file *)dir->u.generic_ip;
	ASSERT(if (!dir_f) {
		printk(KERN_ERR "jffs_mkdir(): No reference to a "
		       "jffs_file struct in inode.\n");
		result = -1;
		goto jffs_mkdir_end;
	});

	c = dir_f->c;

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_mkdir(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		result = -ENOSPC;
		goto jffs_mkdir_end;
	}

	/* If there already exists a file or directory with the same name,
	   then this operation should fail. I originally thought that VFS
	   should take care of this issue.  */
	if (jffs_find_child(dir_f, name, len)) {
		D(printk("jffs_mkdir(): There already exists a file or "
			 "directory with the same name!\n"));
		result = -EEXIST;
		goto jffs_mkdir_end;
	}

	dir_mode = S_IFDIR | (mode & (S_IRWXUGO|S_ISVTX)
			      & ~current->fs->umask);
	if (dir->i_mode & S_ISGID) {
                dir_mode |= S_ISGID;
	}

	/* Create a node and initialize it as much as needed.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_mkdir(): Allocation failed: node == 0\n"));
		result = -ENOMEM;
		goto jffs_mkdir_end;
	}
	DJM(no_jffs_node++);
	node->data_offset = 0;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = c->next_ino++;
	raw_inode.pino = dir_f->ino;
	raw_inode.version = 1;
	raw_inode.mode = dir_mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = raw_inode.atime;
	raw_inode.offset = 0;
	raw_inode.dsize = 0;
	raw_inode.rsize = 0;
	raw_inode.nsize = len;
	raw_inode.nlink = 1;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* Write the new node to the flash.  */
	if (jffs_write_node(c, node, &raw_inode, name, 0) < 0) {
		D(printk("jffs_mkdir(): jffs_write_node() failed.\n"));
		kfree(node);
		DJM(no_jffs_node--);
		result = -1;
		goto jffs_mkdir_end;
	}

	/* Insert the new node into the file system.  */
	result = jffs_insert_node(c, 0, &raw_inode, name, node);

jffs_mkdir_end:
	iput(dir);
	return result;
} /* jffs_mkdir()  */


/* Remove a directory.  */
static int
jffs_rmdir(struct inode *dir, const char *name, int len)
{
	D3(printk("***jffs_rmdir()\n"));
	return jffs_remove(dir, name, len, S_IFDIR, 1);
}


/* Remove any kind of file except for directories.  */
static int
jffs_unlink(struct inode *dir, const char *name, int len)
{
	D3(printk("***jffs_unlink()\n"));
	return jffs_remove(dir, name, len, 0, 1);
}


/* Remove a JFFS entry, i.e. plain files, directories, etc.  Here we
   shouldn't test for free space on the device.  */
static int
jffs_remove(struct inode *dir, const char *name, int len, int type,
	    int must_iput)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_file *dir_f; /* The file-to-remove's parent.  */
	struct jffs_file *del_f; /* The file to remove.  */
	struct jffs_node *del_node;
	struct inode *inode = 0;
	int result = 0;

	D1({
		char *_name = (char *) kmalloc(len + 1, GFP_KERNEL);
		memcpy(_name, name, len);
		_name[len] = '\0';
		printk("***jffs_remove(): file = \"%s\"\n", _name);
		kfree(_name);
	});

	if (!dir) {
		return -ENOENT;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		result = -ENAMETOOLONG;
		goto jffs_remove_end;
	}

	dir_f = (struct jffs_file *) dir->u.generic_ip;
	c = dir_f->c;

	if (!(del_f = jffs_find_child(dir_f, name, len))) {
		D(printk("jffs_remove(): jffs_find_child() failed.\n"));
		result = -ENOENT;
		goto jffs_remove_end;
	}

	if (S_ISDIR(type)) {
		if (!S_ISDIR(del_f->mode)) {
			result = -ENOTDIR;
			goto jffs_remove_end;
		}
		if (del_f->children) {
			result = -ENOTEMPTY;
			goto jffs_remove_end;
		}
        }
	else if (S_ISDIR(del_f->mode)) {
		D(printk("jffs_remove(): node is a directory "
			 "but it shouldn't be.\n"));
		result = -EPERM;
		goto jffs_remove_end;
	}

	if (!(inode = iget(dir->i_sb, del_f->ino))) {
		printk(KERN_ERR "JFFS: Unlink failed.\n");
		result = -ENOENT;
		goto jffs_remove_end;
	}

	/* Create a node for the deletion.  */
	if (!(del_node = (struct jffs_node *)
			 kmalloc(sizeof(struct jffs_node), GFP_KERNEL))) {
		D(printk("jffs_remove(): Allocation failed!\n"));
		result = -ENOMEM;
		goto jffs_remove_end;
	}
	DJM(no_jffs_node++);
	del_node->data_offset = 0;
	del_node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = del_f->ino;
	raw_inode.pino = del_f->pino;
	raw_inode.version = del_f->highest_version + 1;
	raw_inode.mode = del_f->mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = del_f->mtime;
	raw_inode.ctime = raw_inode.atime;
	raw_inode.offset = 0;
	raw_inode.dsize = 0;
	raw_inode.rsize = 0;
	raw_inode.nsize = 0;
	raw_inode.nlink = del_f->nlink;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 1;

	/* Write the new node to the flash memory.  */
	if (jffs_write_node(c, del_node, &raw_inode, 0, 0) < 0) {
		kfree(del_node);
		DJM(no_jffs_node--);
		result = -1;
		goto jffs_remove_end;
	}

	/* Update the file.  This operation will make the file disappear
	   from the in-memory file system structures.  */
	jffs_insert_node(c, del_f, &raw_inode, 0, del_node);

	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
        dir->i_dirt = 1;
	inode->i_nlink = inode->i_nlink ? inode->i_nlink - 1 : 0;
	if (inode->i_nlink == 0) {
		inode->u.generic_ip = 0;
	}
	inode->i_dirt = 1;
	inode->i_ctime = dir->i_ctime;

jffs_remove_end:
	if (must_iput) {
		iput(dir);
	}
	if (inode) {
		iput(inode);
	}
	return result;
} /* jffs_remove()  */


static int
jffs_mknod(struct inode *dir, const char *name, int len, int mode, int rdev)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_file *dir_f;
	struct jffs_node *node = 0;
	struct jffs_control *c;
	int result = 0;
	kdev_t dev = to_kdev_t(rdev);

	D1(printk("***jffs_mknod()\n"));

	if (!dir) {
		return -ENOENT;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		result = -ENAMETOOLONG;
		goto jffs_mknod_end;
	}

	dir_f = (struct jffs_file *)dir->u.generic_ip;
	c = dir_f->c;

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_mknod(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		result = -ENOSPC;
		goto jffs_mknod_end;
	}

	/* Check and see if the file exists already.  */
	if (jffs_find_child(dir_f, name, len)) {
		D(printk("jffs_mknod(): There already exists a file or "
			 "directory with the same name!\n"));
		result = -EEXIST;
		goto jffs_mknod_end;
	}

	/* Create and initialize a new node.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_mknod(): Allocation failed!\n"));
		result = -ENOMEM;
		goto jffs_mknod_err;
	}
	DJM(no_jffs_node++);
	node->data_offset = 0;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = c->next_ino++;
	raw_inode.pino = dir_f->ino;
	raw_inode.version = 1;
	raw_inode.mode = mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = raw_inode.atime;
	raw_inode.offset = 0;
	raw_inode.dsize = sizeof(kdev_t);
	raw_inode.rsize = 0;
	raw_inode.nsize = len;
	raw_inode.nlink = 1;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* Write the new node to the flash.  */
	if (jffs_write_node(c, node, &raw_inode, name,
			    (unsigned char *)&dev) < 0) {
		D(printk("jffs_mknod(): jffs_write_node() failed.\n"));
		result = -1;
		goto jffs_mknod_err;
	}

	/* Insert the new node into the file system.  */
	if (jffs_insert_node(c, 0, &raw_inode, name, node) < 0) {
		result = -1;
		goto jffs_mknod_end;
	}

	goto jffs_mknod_end;

jffs_mknod_err:
	if (node) {
		kfree(node);
		DJM(no_jffs_node--);
	}

jffs_mknod_end:
	iput(dir);
	return result;
} /* jffs_mknod()  */


static int
jffs_symlink(struct inode *dir, const char *name, int len,
	     const char *symname)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_file *dir_f;
	struct jffs_node *node;
	int symname_len = strlen(symname);

	D1({
		char *_name = (char *)kmalloc(len + 1, GFP_KERNEL);
		char *_symname = (char *)kmalloc(symname_len + 1, GFP_KERNEL);
		memcpy(_name, name, len);
		_name[len] = '\0';
		memcpy(_symname, symname, symname_len);
		_symname[symname_len] = '\0';
		printk("***jffs_symlink(): dir = 0x%p, name = \"%s\", "
		       "symname = \"%s\"\n", dir, _name, _symname);
		kfree(_name);
		kfree(_symname);
	});

	if (!dir) {
		return -ENOENT;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		iput(dir);
		return -ENAMETOOLONG;
	}

	dir_f = (struct jffs_file *)dir->u.generic_ip;
	ASSERT(if (!dir_f) {
		printk(KERN_ERR "jffs_symlink(): No reference to a "
		       "jffs_file struct in inode.\n");
		iput(dir);
		return -1;
	});

	c = dir_f->c;

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_symlink(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		iput(dir);
		return -ENOSPC;
	}

	/* Check so there isn't an already existing file with the
	   specified name.  */
	if (jffs_find_child(dir_f, name, len)) {
		iput(dir);
		return -EEXIST;
	}

	/* Create a node and initialize it as much as needed.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_symlink(): Allocation failed: node = NULL\n"));
		iput(dir);
		return -ENOMEM;
	}
	DJM(no_jffs_node++);
	node->data_offset = 0;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = c->next_ino++;
	raw_inode.pino = dir_f->ino;
	raw_inode.version = 1;
	raw_inode.mode = S_IFLNK | S_IRWXUGO;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = raw_inode.atime;
	raw_inode.offset = 0;
	raw_inode.dsize = symname_len;
	raw_inode.rsize = 0;
	raw_inode.nsize = len;
	raw_inode.nlink = 1;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* Write the new node to the flash.  */
	if (jffs_write_node(c, node, &raw_inode, name,
			    (const unsigned char *)symname) < 0) {
		D(printk("jffs_symlink(): jffs_write_node() failed.\n"));
		kfree(node);
		DJM(no_jffs_node--);
		iput(dir);
		return -1;
	}

	/* Insert the new node into the file system.  */
	if (jffs_insert_node(c, 0, &raw_inode, name, node) < 0) {
		iput(dir);
		return -1;
	}

	iput(dir);
	return 0;
} /* jffs_symlink()  */


/* Read the path that a symbolic link is referring to.  */
static int
jffs_readlink(struct inode *inode, char *buffer, int buflen)
{
	struct jffs_file *f;
	int i;
	char *link;
	int result;

	D2(printk("***jffs_readlink()\n"));

	/* Continue only if the file is a symbolic link.  */
	if (!S_ISLNK(inode->i_mode)) {
		result = -EINVAL;
		goto jffs_readlink_end1;
	}
	f = (struct jffs_file *)inode->u.generic_ip;
	ASSERT(if (!f) {
		printk(KERN_ERR "jffs_readlink(): No reference to a "
		       "jffs_file struct in inode.\n");
		result = -1;
		goto jffs_readlink_end1;
	});
	if (!(link = (char *)kmalloc(f->size + 1, GFP_KERNEL))) {
		result = -ENOMEM;
		goto jffs_readlink_end1;
	}
	if ((result = jffs_read_data(f, link, 0, f->size)) < 0) {
		goto jffs_readlink_end2;
	}
	link[result] = '\0';
	for (i = 0; (i < buflen) && (i < result); i++) {
		put_user(link[i], buffer++);
	}
	UPDATE_ATIME(inode);

jffs_readlink_end2:
	kfree(link);
jffs_readlink_end1:
	iput(inode);
	return result;
} /* jffs_readlink()  */


static int
jffs_follow_link(struct inode *dir, struct inode *inode, int flag,
                 int mode, struct inode **res_inode)
{
	struct jffs_file *f;
	char *link;
	int r;

	D3(printk("jffs_follow_link(): dir = 0x%p, "
		  "inode = 0x%p, flag = 0x%08x, mode = 0x%08x\n",
		  dir, inode, flag, mode));

	*res_inode = 0;
	if (!dir) {
		dir = current->fs->root;
		dir->i_count++;
	}
	if (!inode) {
		iput(dir);
		return -ENOENT;
	}
	if (!S_ISLNK(inode->i_mode)) {
		*res_inode = inode;
		iput(dir);
		return 0;
	}
	if (current->link_count > 5) {
		iput(inode);
		iput(dir);
		return -ELOOP;
	}

	f = (struct jffs_file *)inode->u.generic_ip;
	if (!(link = (char *)kmalloc(f->size + 1, GFP_KERNEL))) {
		D(printk("jffs_follow_link(): kmalloc() failed.\n"));
		iput(inode);
		iput(dir);
		return -ENOMEM;
	}
	r = jffs_read_data(f, link, 0, f->size);
	if (r < f->size) {
		D(printk("jffs_follow_link(): Failed to read symname.\n"));
		kfree(link);
		iput(inode);
		iput(dir);
		return -EIO;
	}
	link[r] = '\0';
	UPDATE_ATIME(inode);
	current->link_count++;
	r = open_namei(link, flag, mode, res_inode, dir);
	current->link_count--;
	kfree(link);
	iput(inode);
	return r;
} /* jffs_follow_link()  */


/* Create an inode inside a JFFS directory (dir) and return it.  */
static int
jffs_create(struct inode *dir, const char *name, int len,
            int mode, struct inode **result)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_node *node;
	struct jffs_file *dir_f; /* JFFS representation of the directory.  */
	struct inode *inode;

	*result = (struct inode *)0;

	D1({
		char *s = (char *)kmalloc(len + 1, GFP_KERNEL);
		memcpy(s, name, len);
		s[len] = '\0';
		printk("jffs_create(): dir: 0x%p, name: \"%s\"\n", dir, s);
		kfree(s);
	});

	if (!dir) {
		return -ENOENT;
	}
	if (len > JFFS_MAX_NAME_LEN) {
		iput(dir);
		return -ENAMETOOLONG;
	}

	dir_f = (struct jffs_file *)dir->u.generic_ip;
	ASSERT(if (!dir_f) {
		printk(KERN_ERR "jffs_create(): No reference to a "
		       "jffs_file struct in inode.\n");
		iput(dir);
		return -1;
	});

	c = dir_f->c;

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_create(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		iput(dir);
		return -ENOSPC;
	}

	/* If there already exists a file or directory with the same name,
	   then this operation should fail. I originally thought that VFS
	   should take care of this issue.  */
	if (jffs_find_child(dir_f, name, len)) {
		D(printk("jffs_create(): There already exists a file or "
			 "directory named \"%s\"!\n", name));
		iput(dir);
		return -EEXIST;
	}

	/* Create a node and initialize as much as needed.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_create(): Allocation failed: node == 0\n"));
		iput(dir);
		return -ENOMEM;
	}
	DJM(no_jffs_node++);
	node->data_offset = 0;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = c->next_ino++;
	raw_inode.pino = dir_f->ino;
	raw_inode.version = 1;
	raw_inode.mode = mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = raw_inode.atime;
	raw_inode.offset = 0;
	raw_inode.dsize = 0;
	raw_inode.rsize = 0;
	raw_inode.nsize = len;
	raw_inode.nlink = 1;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* Write the new node to the flash.  */
	if (jffs_write_node(c, node, &raw_inode, name, 0) < 0) {
		D(printk("jffs_create(): jffs_write_node() failed.\n"));
		kfree(node);
		DJM(no_jffs_node--);
		iput(dir);
		return -1;
	}

	/* Insert the new node into the file system.  */
	if (jffs_insert_node(c, 0, &raw_inode, name, node) < 0) {
		iput(dir);
		return -1;
	}

	/* Initialize an inode.  */
	if (!(inode = get_empty_inode())) {
		iput(dir);
		return -1;
	}
	inode->i_dev = dir->i_sb->s_dev;
	inode->i_ino = raw_inode.ino;
	inode->i_mode = mode;
	inode->i_nlink = raw_inode.nlink;
	inode->i_uid = raw_inode.uid;
	inode->i_gid = raw_inode.gid;
	inode->i_rdev = 0;
	inode->i_size = 0;
	inode->i_atime = raw_inode.atime;
	inode->i_mtime = raw_inode.mtime;
	inode->i_ctime = raw_inode.ctime;
	inode->i_blksize = PAGE_SIZE; /* This is the optimal IO size (for
					 stat), not the fs block size.  */
	inode->i_blocks = 0;
	inode->i_version = 0;
	inode->i_nrpages = 0;
	/*inode->i_sem = 0;*/
	inode->i_op = &jffs_file_inode_operations;
	inode->i_sb = dir->i_sb;
	inode->i_wait = 0;
	inode->i_flock = 0;
	inode->i_count = 1;
	inode->i_flags = dir->i_sb->s_flags;
	inode->i_dirt = 1;
	inode->u.generic_ip = (void *)jffs_find_file(c, raw_inode.ino);

	iput(dir);
	*result = inode;
	return 0;
} /* jffs_create()  */


/* Write, append or rewrite data to an existing file.  */
static int
jffs_file_write(struct inode *inode, struct file *filp,
                const char *buf, int count)
{
	struct jffs_raw_inode raw_inode;
	struct jffs_control *c;
	struct jffs_file *f;
	struct jffs_node *node;
	int written = 0;
	int pos;

	D2(printk("***jffs_file_write(): inode: 0x%p (ino: %lu), "
		  "filp: 0x%p, buf: 0x%p, count: %d\n",
		  inode, inode->i_ino, filp, buf, count));

	if (!inode) {
		D(printk("jffs_file_write(): inode == NULL\n"));
		return -EINVAL;
	}

	if (inode->i_sb->s_flags & MS_RDONLY) {
		D(printk("jffs_file_write(): MS_RDONLY\n"));
		return -ENOSPC;
	}
	if (!S_ISREG(inode->i_mode)) {
		D(printk("jffs_file_write(): inode->i_mode == 0x%08x\n",
			 inode->i_mode));
		return -EINVAL;
	}

	if (!(f = (struct jffs_file *)inode->u.generic_ip)) {
		D(printk("jffs_file_write(): inode->u.generic_ip = 0x%p\n",
			 inode->u.generic_ip));
		return -EINVAL;
	}
	c = f->c;

	if (!JFFS_ENOUGH_SPACE(c->fmc)) {
		D1(printk("jffs_file_write(): Free size = %u\n",
			  jffs_free_size1(c->fmc) + jffs_free_size2(c->fmc)));
		D(printk(KERN_NOTICE "JFFS: No space left on device\n"));
		return -ENOSPC;
	}

	if (filp->f_flags & O_APPEND) {
		pos = inode->i_size;
	}
	else {
		pos = filp->f_pos;
	}

	/* Things are going to be written so we could allocate and
	   initialize the necessary data structures now.  */
	if (!(node = (struct jffs_node *) kmalloc(sizeof(struct jffs_node),
						  GFP_KERNEL))) {
		D(printk("jffs_file_write(): node == 0\n"));
		return -ENOMEM;
	}
	DJM(no_jffs_node++);
	node->data_offset = f->size;
	node->removed_size = 0;

	/* Initialize the raw inode.  */
	raw_inode.magic = JFFS_MAGIC_BITMASK;
	raw_inode.ino = f->ino;
	raw_inode.pino = f->pino;
	raw_inode.version = f->highest_version + 1;
	raw_inode.mode = f->mode;
	raw_inode.uid = current->fsuid;
	raw_inode.gid = current->fsgid;
	raw_inode.atime = CURRENT_TIME;
	raw_inode.mtime = raw_inode.atime;
	raw_inode.ctime = f->ctime;
	raw_inode.offset = f->size;
	raw_inode.dsize = count;
	raw_inode.rsize = 0;
	raw_inode.nsize = 0;
	raw_inode.nlink = f->nlink;
	raw_inode.spare = 0;
	raw_inode.rename = 0;
	raw_inode.deleted = 0;

	/* Write the new node to the flash.  */
	if ((written = jffs_write_node(c, node, &raw_inode, 0,
				       (const unsigned char *)buf)) < 0) {
		D(printk("jffs_file_write(): jffs_write_node() failed.\n"));
		kfree(node);
		DJM(no_jffs_node--);
		return -1;
	}

	/* Insert the new node into the file system.  */
	if (jffs_insert_node(c, f, &raw_inode, 0, node) < 0) {
		return -1;
	}
	pos += written;
	filp->f_pos = pos;

	D3(printk("jffs_file_write(): new f_pos %d.\n", pos));

	/* Fix things in the real inode.  */
	if (pos > inode->i_size) {
		inode->i_size = pos;
	}
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	inode->i_dirt = 1;

	/* simonk: fixes the O_APPEND bug, but forces a buffer flush.
	   Should probably use update_vm_cache() instead.  */
	invalidate_inode_pages(inode);

	return written;
} /* jffs_file_write()  */


/* This is our ioctl() routine.  */
static int
jffs_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
	   unsigned long arg)
{
	struct jffs_control *c;
	int err;

	D2(printk("***jffs_ioctl(): cmd = 0x%08x, arg = 0x%08lx\n",
		  cmd, arg));

	if (!(c = (struct jffs_control *)inode->i_sb->u.generic_sbp)) {
		printk(KERN_ERR "JFFS: Bad inode in ioctl() call. "
		       "(cmd = 0x%08x)\n", cmd);
		return -1;
	}

	switch (cmd) {
	case JFFS_PRINT_HASH:
		jffs_print_hash_table(c);
		break;
	case JFFS_PRINT_TREE:
		jffs_print_tree(c->root, 0);
		break;
	case JFFS_GET_STATUS:
		{
			struct jffs_flash_status fst;
			struct jffs_fmcontrol *fmc = c->fmc;
			printk("Flash status -- ");
			err = verify_area(VERIFY_WRITE,
					  (struct jffs_flash_status *)arg,
					  sizeof(struct jffs_flash_status));
			if (err) {
				D(printk("jffs_ioctl(): Bad arg in "
					 "JFFS_GET_STATUS ioctl!\n"));
				return err;
			}
			fst.size = fmc->flash_size;
			fst.used = fmc->used_size;
			fst.dirty = fmc->dirty_size;
			fst.begin = fmc->head->offset;
			fst.end = fmc->tail->offset + fmc->tail->size;
			printk("size: %d, used: %d, dirty: %d, "
			       "begin: %d, end: %d\n",
			       fst.size, fst.used, fst.dirty,
			       fst.begin, fst.end);
			memcpy_tofs((struct jffs_flash_status *)arg, &fst,
				    sizeof(struct jffs_flash_status));
		}
		break;
	default:
		return -ENOTTY;
	}

	return 0;
} /* jffs_ioctl()  */


static struct file_operations jffs_file_operations =
{
	NULL,                 /* lseek - default */
	generic_file_read,    /* read */
	jffs_file_write,      /* write */
	NULL,                 /* readdir */
	NULL,                 /* select - default */
	jffs_ioctl,           /* ioctl */
	generic_file_mmap,    /* mmap */
	NULL,                 /* open */
	NULL,                 /* release */
	NULL,                 /* fsync */
	NULL,                 /* fasync */
	NULL,                 /* check_media_change */
	NULL                  /* revalidate */
};

static struct inode_operations jffs_file_inode_operations =
{
	&jffs_file_operations,
	NULL,                 /* create */
	jffs_lookup,          /* lookup */
	NULL,                 /* link */
	NULL,                 /* unlink */
	NULL,                 /* symlink */
	NULL,                 /* mkdir */
	NULL,                 /* rmdir */
	NULL,                 /* mknod */
	NULL,                 /* rename */
	NULL,                 /* readlink */
	NULL,                 /* follow_link */
	jffs_readpage,        /* readpage */
	NULL,                 /* writepage */
	NULL,                 /* bmap -- not really */
	NULL,                 /* truncate */
	NULL,                 /* permission */
	NULL                  /* smap */
};


static struct file_operations jffs_dir_operations =
{
	NULL,                 /* lseek - default */
	NULL,                 /* read */
	NULL,                 /* write */
	jffs_readdir,         /* readdir */
	NULL,                 /* select - default */
	NULL,                 /* ioctl */
	NULL,                 /* mmap */
	NULL,                 /* open */
	NULL,                 /* release */
	NULL,                 /* fsync */
	NULL,                 /* fasync */
	NULL,                 /* check_media_change */
	NULL                  /* revalidate */
};

static struct inode_operations jffs_dir_inode_operations =
{
	&jffs_dir_operations,
	jffs_create,           /* create */
	jffs_lookup,           /* lookup */
	NULL,                  /* link */
	jffs_unlink,           /* unlink */
	jffs_symlink,          /* symlink */
	jffs_mkdir,            /* mkdir */
	jffs_rmdir,            /* rmdir */
	jffs_mknod,            /* mknod */
	jffs_rename,           /* rename */
	NULL,                  /* readlink */
	NULL,                  /* follow_link */
	NULL,                  /* readpage */
	NULL,                  /* writepage */
	NULL,                  /* bmap */
	NULL,                  /* truncate */
	NULL,                  /* permission */
	NULL                   /* smap */
};


static struct inode_operations jffs_symlink_inode_operations =
{
	NULL,                  /* No file operations.  */
	NULL,                  /* create */
	NULL,                  /* lookup */
	NULL,                  /* link */
	NULL,                  /* unlink */
	NULL,                  /* symlink */
	NULL,                  /* mkdir */
	NULL,                  /* rmdir */
	NULL,                  /* mknod */
	NULL,                  /* rename */
	jffs_readlink,         /* readlink */
	jffs_follow_link,      /* follow_link */
	NULL,                  /* readpage */
	NULL,                  /* writepage */
	NULL,                  /* bmap */
	NULL,                  /* truncate */
	NULL,                  /* permission */
	NULL                   /* smap */
};


/* Initialize an inode for the VFS.  */
static void
jffs_read_inode(struct inode *inode)
{
	struct jffs_file *f;
	struct jffs_control *c;

	D3(printk("jffs_read_inode(): inode->i_ino == %lu\n", inode->i_ino));

	if (!inode->i_sb) {
		D(printk("jffs_read_inode(): !inode->i_sb ==> "
			 "No super block!\n"));
		return;
	}
	c = (struct jffs_control *)inode->i_sb->u.generic_sbp;
	if (!(f = jffs_find_file(c, inode->i_ino))) {
		D(printk("jffs_read_inode(): No such inode (%lu).\n",
			 inode->i_ino));
		return;
	}
	inode->u.generic_ip = (void *)f;
	inode->i_mode = f->mode;
	inode->i_nlink = f->nlink;
	inode->i_uid = f->uid;
	inode->i_gid = f->gid;
	inode->i_size = f->size;
	inode->i_atime = f->atime;
	inode->i_mtime = f->mtime;
	inode->i_ctime = f->ctime;
	inode->i_blksize = PAGE_SIZE;
	inode->i_blocks = 0;
	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &jffs_file_inode_operations;
	}
	else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &jffs_dir_inode_operations;
	}
	else if (S_ISLNK(inode->i_mode)) {
                inode->i_op = &jffs_symlink_inode_operations;
	}
	else if (S_ISCHR(inode->i_mode)) {
		inode->i_op = &chrdev_inode_operations;
	}
	else if (S_ISBLK(inode->i_mode)) {
		inode->i_op = &blkdev_inode_operations;
	}

	/* If the node is a device of some sort, then the number of the
	   device should be read from the flash memory and then added
	   to the inode's i_rdev member.  */
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
		kdev_t rdev;
		jffs_read_data(f, (char *)&rdev, 0, sizeof(kdev_t));
		inode->i_rdev = kdev_t_to_nr(rdev);
	}
}


void
jffs_write_super(struct super_block *sb)
{
#ifdef USE_GC
	jffs_garbage_collect((struct jffs_control *)sb->u.generic_sbp);
#endif
}


static struct super_operations jffs_ops =
{
	jffs_read_inode,    /* read inode */
	jffs_notify_change, /* notify change */
	NULL,               /* write inode */
	NULL,               /* put inode */
	jffs_put_super,     /* put super */
	jffs_write_super,   /* write super */
	jffs_statfs,        /* statfs */
	NULL                /* remount */
};


static struct file_system_type jffs_fs_type =
{
	jffs_read_super,
	"jffs",
	1,
	NULL
};


int
init_jffs_fs(void)
{
	int procfs_status;
	printk("JFFS, version " JFFS_VERSION_STRING
	       ", (C) 1999, 2000  Axis Communications AB\n");

	/* Added by Simon Kågström to register an entry in the procfs.*/
#ifdef CONFIG_JFFS_PROC_FS
	/* We should do a :
	 for(i=0; i<NR_SUPER; i++)
	   proc_register...

	   to support more than one mounted jffs.
	 */


#ifdef KERNEL_VERSION /* Not defined for kernel 2.0.* */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
	procfs_status = proc_register(&proc_root, &jffs_proc_file);
#else
	procfs_status = proc_register_dynamic(&proc_root, &jffs_proc_file);
#endif /* Linux version*/
#else
	procfs_status = proc_register_dynamic(&proc_root, &jffs_proc_file);
#endif /* KERNEL_VERSION */
#endif /* CONFIG_JFFS_PROC_FS */

	/* Handle procfs_status here!  */
	return register_filesystem(&jffs_fs_type);
}


#ifdef MODULE
int
init_module(void)
{
	int status;

	if ((status = init_jffs_fs()) == 0) {
		register_symtab(0);
	}
	return status;
}

void
cleanup_module(void)
{
	unregister_filesystem(&jffs_fs_type);
}
#endif
