#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -c $< -o $*.o

all: bootup.o
O_TARGET := bootup.o
O_OBJS := boot.o gdb_output.o timer.o

boot.o: boot.S

timer.o: timer.c

clean:
	rm -f *.[oa]

include $(TOPDIR)/Rules.make
