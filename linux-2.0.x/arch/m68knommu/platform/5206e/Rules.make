#
# ColdFire/Rules.make
#
# This file is included by the global makefile so that you can add your own
# platform-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1999 by Greg Ungerer (gerg@snapgear.com)
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@lineo.ca>
# Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
# Copyright (C) 1994 by Hamish Macdonald
# Copyright (C) 2000  Lineo Inc. (www.lineo.com) 
#

GCC_DIR = $(shell $(CC) -v 2>&1 | grep specs | sed -e 's/.* \(.*\)specs/\1\./')

INCGCC = $(GCC_DIR)/include
LIBGCC = $(GCC_DIR)/m5200/libgcc.a

CFLAGS := $(CFLAGS) -I$(INCGCC) -pipe -DNO_MM -DNO_FPU -m5200 -Wa,-S -Wa,-m5200 -D__ELF__ -DMAGIC_ROM_PTR -DUTS_SYSNAME='"uClinux"'
AFLAGS := $(CFLAGS)

ifdef CONFIG_MEMORY_PROTECT
	CFLAGS += -DMCF_MEMORY_PROTECT
endif
ifdef CONFIG_SMALL
	KSIZE = small_
endif

LINKFLAGS = -T arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/$(KSIZE)$(MODEL).ld

HEAD := arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/crt0_$(MODEL).o

SUBDIRS := arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
           arch/$(ARCH)/platform/$(PLATFORM) $(SUBDIRS)
ARCHIVES := arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o \
            arch/$(ARCH)/platform/$(PLATFORM)/platform.o $(ARCHIVES)
LIBS += arch/$(ARCH)/lib/lib.a $(LIBGCC)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

archclean:
	@$(MAKEBOOT) clean

