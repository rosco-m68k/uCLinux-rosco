#
# Makefile for m68k-specific library files..
#

.S.o:
	$(CC) $(AFLAGS) -D__ASSEMBLY__ -traditional -c $< -o $@

L_TARGET = lib.a
L_OBJS  = ashrdi3.o bzero.o checksum.o memcmp.o memcpy.o memset.o semaphore.o

include $(TOPDIR)/Rules.make
