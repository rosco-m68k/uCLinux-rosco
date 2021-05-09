#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note! The CFLAGS definitions are now in the main makefile...

.S.o:
	$(CC) -g -D__ASSEMBLY__ $(AFLAGS) -traditional -c $< -o $*.o


all: $(BOARD)/crt0_$(MODEL).o entry.o platform.o 

O_TARGET := platform.o

O_OBJS := entry.o signal.o config.o ints.o 

$(BOARD)/crt0_$(MODEL).o: $(BOARD)/crt0_$(MODEL).S

entry.o: entry.S

include $(TOPDIR)/Rules.make
