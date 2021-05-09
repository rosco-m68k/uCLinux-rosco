#
# Makefile for the LEON-2.x platform
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Copyright (c) 2001 by The LEOX team <team@leox.org>
#
# Based on arch/m68knommu/platform/5307/Makefile
#
.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -ansi -c $< -o $*.o

all: $(BOARD)/crt0_$(MODEL).o entry.o platform.o

O_TARGET := platform.o
O_OBJS := entry.o systbls.o start.o

$(BOARD)/crt0_$(MODEL).o: $(BOARD)/crt0_$(MODEL).S

entry.o: entry.S
systbls.o: systbls.S

include $(TOPDIR)/Rules.make
