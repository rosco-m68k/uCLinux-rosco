#
# h8300/Makefile
#
# This file is included by the global makefile so that you can add your own
# platform-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Yosinori Sato <qzb04471@nifty.ne.jp>
#
# Based on m68knommu/Rules.make
#
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@rt-control.com>
# Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
# Copyright (C) 1994 by Hamish Macdonald
#

CROSS_COMPILE = h8300-hitachi-hms-

LIBGCC = `$(CC) -v 2>&1 | grep specs | sed -e "s/Reading specs from //" | sed -e s/specs/h8300h\\\/libgcc.a/`

#CFLAGS := $(CFLAGS) -g -pipe -DNO_MM -DNO_FPU -mh -mint32 \
#-fsigned-char -DMAGIC_ROM_PTR -DNO_FORGET -DUTS_SYSNAME='"uClinux"'
CFLAGS := $(CFLAGS) -g -pipe -DNO_MM -DNO_FPU -mh -mint32 \
-fsigned-char -DNO_FORGET -DUTS_SYSNAME='"uClinux"'
AFLAGS := $(AFLAGS) -pipe -DNO_MM -DNO_FPU -mh -DMAGIC_ROM_PTR -DUTS_SYSNAME='"uClinux"'  -Wa,-gstabs
LINKFLAGS = -T arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/$(MODEL).ld
EXTRA_LDFLAGS = -mh8300h

SUBDIRS := arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
           arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD) $(SUBDIRS)
ARCHIVES := arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o \
            arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/bootup.o $(ARCHIVES)
LIBS += arch/$(ARCH)/lib/lib.a $(LIBGCC)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)

archclean:
	@$(MAKEBOOT) clean
