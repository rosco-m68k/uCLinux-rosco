#
# arch/arm/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995, 1996 Russell King
#

Image:	$(CONFIGURE) tools/build $(TOPDIR)/vmlinux
	tools/build $(TOPDIR)/vmlinux > Image
	sync

zImage:	$(CONFIGURE) tools/build compressed/vmlinux
	tools/build compressed/vmlinux > zImage

compressed/vmlinux: $(TOPDIR)/vmlinux dep
	@$(MAKE) -C compressed vmlinux

install: $(CONFIGURE) Image
	sh ./install.sh $(VERSION).$(PATCHLEVEL).$(SUBLEVEL) Image $(TOPDIR)/System.map "$(INSTALL_PATH)"

zinstall: $(CONFIGURE) zImage
	sh ./install.sh $(VERSION).$(PATCHLEVEL).$(SUBLEVEL) zImage $(TOPDIR)/System.map "$(INSTALL_PATH)"

tools/build: tools/build.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $< -I$(TOPDIR)/include

clean:
	rm -f Image zImage tools/build
	@$(MAKE) -C compressed clean

dep:

