#
# Makefile for JFFS (Journaling Flash File System), Linux implementation.
#
# Copyright (C) 1999, 2000  Axis Communications, Inc.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...
#
# $Id: Makefile,v 1.2 2000/10/06 08:39:31 davidm Exp $
#

O_TARGET := jffs.o
O_OBJS   := jffs_fm.o intrep.o inode.o

ifeq ($(CONFIG_JFFS_PROC_FS),y)
O_OBJS   += jffs_proc.o
endif

M_OBJS   := $(O_TARGET)

include $(TOPDIR)/Rules.make
