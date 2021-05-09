#
# Makefile for m68k-specific library files..
#

.S.o:
	$(CC) $(AFLAGS) -D__ASSEMBLY__ -traditional -c $< -o $@

L_TARGET = lib.a
L_OBJS  = abs.o ashrdi3.o checksum.o __adddi3.o

include $(TOPDIR)/Rules.make
