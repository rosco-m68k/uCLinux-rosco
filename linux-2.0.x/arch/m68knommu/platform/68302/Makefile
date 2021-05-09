#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -traditional -c $< -o $*.o

all: $(BOARD)-head.o entry.o platform.o
O_TARGET := platform.o
O_OBJS := entry.o config.o signal.o ints.o

$(BOARD)-head.o: $(BOARD)-head.S
entry.o: entry.S

include $(TOPDIR)/Rules.make
