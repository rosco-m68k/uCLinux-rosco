#
# LEON-2.x/Rules.make
#
# This file is included by the global makefile so that you can add your own
# platform-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (c) 2001 by The LEOX team <team@leox.org>
#
# Based on arch/m68knommu/platform/5307/Makefile:
# Copyright (C) 1999 by Greg Ungerer (gerg@snapgear.com)
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@lineo.ca>
# Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
# Copyright (C) 1994 by Hamish Macdonald
# Copyright (C) 2000  Lineo Inc. (www.lineo.com) 
#

GCC_DIR = $(shell $(CC) -v 2>&1 | grep specs | sed -e 's/.* \(.*\)specs/\1\./')

INCGCC = $(GCC_DIR)/include
LIBGCC = $(GCC_DIR)/libgcc.a

CFLAGS := $(CFLAGS) -g -fno-builtin -msoft-float -pipe -DNO_MM -DNO_FPU -mv8 -DMAGIC_ROM_PTR -DNO_FORGET $(VENDOR_CFLAGS) -DUTS_SYSNAME='"uClinux"'
AFLAGS := $(CFLAGS) $(VENDOR_CFLAGS)


LINKFLAGS = -T arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/$(MODEL).ld

HEAD := arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/crt0_$(MODEL).o

SUBDIRS := arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
           arch/$(ARCH)/platform/$(PLATFORM) $(SUBDIRS)

ARCHIVES := arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o \
            arch/$(ARCH)/platform/$(PLATFORM)/platform.o $(ARCHIVES)

LIBS += arch/$(ARCH)/lib/lib.a $(LIBGCC)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

linux.s19: linux System.map
	$(CROSS_COMPILE)objcopy -O srec \
	--adjust-section-vma=.data=0x`cat System.map | grep __data_rom_start | sed -e 's/ T __data_rom_start//'` linux linux.s19

archclean:
	@$(MAKEBOOT) clean
	rm -f linux.s19
