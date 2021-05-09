#
# linux/arch/arm/boot/compressed/Makefile
#
# create a compressed vmlinuz image from the original vmlinux
#
# With this config, max compressed image size = 640k
# Uncompressed image size = 1.3M (text+data)


HEAD = head.o
SYSTEM = $(TOPDIR)/vmlinux
OBJS = $(HEAD) misc.o $(COMPRESSED_EXTRA)

CFLAGS = -O2 -DSTDC_HEADERS $(CFLAGS_PROC)
ARFLAGS = rc

all:		vmlinux

vmlinux:	piggy.o $(OBJS)
		$(LD) $(ZLINKFLAGS) -o vmlinux $(OBJS) piggy.o

head.o: 	head.S
		$(CC) -traditional -DLOADADDR=$(ZRELADDR) -c head.S

piggy.o:	$(SYSTEM) xtract piggyback
		./xtract $(SYSTEM) | gzip -9 | ./piggyback > $@

# rules for extracting & piggybacking the kernel

xtract: 	xtract.c
		$(HOSTCC) $(HOSTCFLAGS) -o xtract xtract.c

piggyback:	piggyback.c
		$(HOSTCC) $(HOSTCFLAGS) -o piggyback piggyback.c

clean:
		rm -f xtract piggyback vmlinux core

