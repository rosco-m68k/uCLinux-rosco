#
# 68360/Makefile
#
# This file is included by the global makefile so that you can add your own
# platform-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2001	   SED Systems, A Division of Calian Ltd.
# Copyright (C) 2000       Michael Leslie <mleslie@lineo.com>,
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@uclinux.org>,
# Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
# Copyright (C) 1994 by Hamish Macdonald
#

ifndef CROSS_COMPILE
CROSS_COMPILE = m68k-elf-
endif

GCC_DIR = $(shell $(CC) -v 2>&1 | grep specs | sed -e 's/.* \(.*\)specs/\1\./')

INCGCC = $(GCC_DIR)/include
LIBGCC = $(GCC_DIR)/m68000/libgcc.a

CFLAGS := $(CFLAGS) -I$(INCGCC) -pipe -DNO_MM -DNO_FPU -m68332 -Wa,-S -Wa,-m68332 -D__ELF__ -DMAGIC_ROM_PTR -DUTS_SYSNAME='"uClinux"'
AFLAGS := $(CFLAGS)

LINKFLAGS = -T arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/$(MODEL).ld

HEAD := arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/crt0_$(MODEL).o

SUBDIRS := arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
           arch/$(ARCH)/platform/$(PLATFORM) $(SUBDIRS)
ARCHIVES := arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o \
            arch/$(ARCH)/platform/$(PLATFORM)/platform.o $(ARCHIVES)
LIBS += arch/$(ARCH)/lib/lib.a $(LIBGCC)

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

romfs.s19: linux remake_romfs.s19

.PHONY : remake_romfs.s19
remake_romfs.s19: romfs.img
	$(CROSS_COMPILE)objcopy --adjust-section-vma=.data=0x`$(CROSS_COMPILE)nm linux | awk '/__data_rom_start/ {printf $$1}'` linux romfs.s19
	ADDR=`$(CROSS_COMPILE)objdump --headers romfs.s19 | \
	grep .data | cut -d' ' -f 13,15 | xargs printf "0x%s 0x%s\n" | \
	xargs printf "%d + %d\n" |xargs expr |xargs printf "0x%x\n"`;\
	$(CROSS_COMPILE)objcopy --add-section=.romfs=romfs.img \
	--adjust-section-vma=.romfs=$${ADDR} --no-adjust-warnings \
	--set-section-flags=.romfs=alloc,load,data   \
	romfs.s19 romfs.s19 2> /dev/null
	$(CROSS_COMPILE)objcopy -O srec romfs.s19 romfs.s19

linux.s19: linux
	$(CROSS_COMPILE)objcopy -O srec --adjust-section-vma=.data=0x`$(CROSS_COMPILE)nm linux | awk '/__data_rom_start/ {printf $$1}'` --adjust-section-vma=.text=0x`$(CROSS_COMPILE)nm linux | awk '/__kernel_image_start/ {printf $$1}'` linux linux.s19
	@if [ -e romfs.s19 ] ; then \
		rm -f romfs.s19 ; \
	fi
	ln -s linux.s19 romfs.s19

.PHONY : flash.s19
flash.s19:
	mkfs.jffs -d ../flash1 -a big -o flash1.img 2> /dev/null
	srec_cat flash1.img -BIN -offset 0x01480000 > flash1.s19
	mkfs.jffs -d ../flash2 -a big -o flash2.img 2> /dev/null
	srec_cat flash2.img -BIN -offset 0x01600000 > flash2.s19

.PHONY : all
all:	boot.s19 flash.s19

.PHONY : boot.s19
boot.s19: linux.s19
	srec_cat linux.s19 -Crop 0x0 0x00040000 > boot.s19
	srec_cat linux.s19 -Crop 0x01000000 0x01800000 > system.s19

.PHONY : romfs.img
romfs.img:
	genromfs -d ../romfs -f romfs.img -x CVS -x Makefile -x *.c -x *.h \
	-x *.cpp -x *.gdb -x *.elf -x *.tmp -x *.elf2flt -x core

archclean:
	@$(MAKEBOOT) clean
	rm -f linux.s19 romfs.s19 romfs.img boot.s19 system.s19 jffs.img \
			jffs.s19 jffs_pc.img flash1.s19 flash2.s19 flash1.img flash2.img
