#
# Makefile for or32-specific library files..
#

.S.o:
	$(CC) $(AFLAGS) -D__ASSEMBLY__ -traditional -c $< -o $@

L_TARGET = lib.a
L_OBJS  = bzero.o checksum.o memcmp.o

include $(TOPDIR)/Rules.make
