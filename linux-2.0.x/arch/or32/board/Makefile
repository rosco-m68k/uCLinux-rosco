#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -c $< -o $*.o

all: board.o

O_TARGET := board.o
O_OBJS := config.o

../tools/bintoc: ../tools/bintoc.c
	$(HOSTCC) $(HOSTCFLAGS) -o ../tools/bintoc ../tools/bintoc.c

include $(TOPDIR)/Rules.make
