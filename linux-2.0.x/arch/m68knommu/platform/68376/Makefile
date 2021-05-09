#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -c $< -o $*.o


all: $(BOARD)/crt0_$(MODEL).o entry.o platform.o
O_TARGET := platform.o
O_OBJS := entry.o config.o ints.o traps.o signal.o

entry.o: entry.S

$(BOARD)/crt0_$(MODEL).o: $(BOARD)/crt0_$(MODEL).o


include $(TOPDIR)/Rules.make
