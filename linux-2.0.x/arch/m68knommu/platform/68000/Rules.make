#
# 68000/Rules.make
#
# This file is included by the global makefile so that you can add your own
# platform-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2002,2003  Guido Classen <classeng@clagi.de>
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@uclinux.org>,
# Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
# Copyright (C) 1994 by Hamish Macdonald
#

CROSS_COMPILE = m68k-coff-

LIBGCC = `$(CC) -v 2>&1 | grep specs | sed -e "s/Reading specs from //" | sed -e s/specs/m68000\\\/libgcc.a/`

CFLAGS := $(CFLAGS) -pipe -DNO_MM -DNO_FPU -m68000 -D__COFF__ -DMAGIC_ROM_PTR -DNO_FORGET -DUTS_SYSNAME='"uClinux"'
AFLAGS := $(AFLAGS) -pipe -DNO_MM -DNO_FPU -m68000 -D__COFF__ -DMAGIC_ROM_PTR -DUTS_SYSNAME='"uClinux"'  -Wa,--bitwise-or

LINKFLAGS = -T arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/$(MODEL).ld

HEAD := arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/crt0_$(MODEL).o

INIT_B := arch/$(ARCH)/platform/$(PLATFORM)/$(BOARD)/init_$(MODEL).b
STOB   := arch/$(ARCH)/platform/$(PLATFORM)/tools/stob

SUBDIRS := arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
           arch/$(ARCH)/platform/$(PLATFORM) $(SUBDIRS)
ARCHIVES := arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o \
            arch/$(ARCH)/platform/$(PLATFORM)/platform.o $(ARCHIVES)
LIBS += arch/$(ARCH)/lib/lib.a $(LIBGCC)

ifdef CONFIG_FRAMEBUFFER
SUBDIRS := $(SUBDIRS) arch/$(ARCH)/console
ARCHIVES := $(ARCHIVES) arch/$(ARCH)/console/console.a
endif

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

romfs.s19: romfs.img arch/$(ARCH)/empty.o
	$(CROSS_COMPILE)objcopy -v -R .text -R .data -R .bss --add-section=.fs=romfs.img --adjust-section-vma=.fs=$(ROMFS_LOAD_ADDR) arch/$(ARCH)/empty.o romfs.s19
	$(CROSS_COMPILE)objcopy -O srec romfs.s19

romfs.b: romfs.s19
	$(STOB) romfs.s19 > romfs.b

linux.data: linux
	#$(CROSS_COMPILE)objcopy -O binary --remove-section=.romvec --remove-section=.text --remove-section=.ramvec --remove-section=.bss --remove-section=.eram linux linux.data
	#2002-05-14 gc: 
	$(CROSS_COMPILE)objcopy -O binary -j .data linux linux.data

linux.text: linux
	#$(CROSS_COMPILE)objcopy -O binary --remove-section=.ramvec --remove-section=.bss --remove-section=.data --remove-section=.eram --set-section-flags=.romvec=CONTENTS,ALLOC,LOAD,READONLY,CODE linux linux.text
	#2002-05-14 gc: 
	$(CROSS_COMPILE)objcopy -O binary -j .text linux linux.text


romfs.img:
	echo creating a vmlinux rom image without root filesystem!

linux.bin: linux.text linux.data romfs.img
	if [ -f romfs.img ]; then\
		cat linux.text linux.data romfs.img > linux.bin;\
	else\
		cat linux.text linux.data > linux.bin;\
	fi


#2002-05-14 gc: 
linux.srec: linux.bin
	@perl -e '$$s=sprintf "%d", (((-s "linux.bin")+1023) / 1024); \
	   die "image to large ($${s}k)\n" if ($$s > 512);'
	perl arch/m68knommu/platform/68000/srec.pl linux.bin >linux.srec



flash.s19: linux.bin arch/$(ARCH)/empty.o
#	$(CROSS_COMPILE)objcopy -v -R .text -R .data -R .bss --add-section=.fs=linux.bin --adjust-section-vma=.fs=$(FLASH_LOAD_ADDR) arch/$(ARCH)/empty.o flash.s19
	$(CROSS_COMPILE)objcopy -O srec flash.s19

flash.b: flash.s19
	$(STOB) flash.s19 > flash.b

linux.trg linux.rom: linux.bin
	perl arch/$(ARCH)/platform/$(PLATFORM)/tools/fixup.pl

linux.s19: linux
	$(CROSS_COMPILE)objcopy -O srec --adjust-section-vma=.data=0x`$(CROSS_COMPILE)nm linux | awk '/__data_rom_start/ {printf $$1}'` linux linux.s19

	$(CROSS_COMPILE)objcopy -O srec linux.s19
	
linux.b: linux.s19
	if [ -f $(INIT_B) ]; then\
		cp $(INIT_B) linux.b;\
	fi
	$(STOB) linux.s19 >> linux.b

archclean:
	@$(MAKEBOOT) clean
	rm -f linux.text linux.data linux.bin linux.rom linux.trg
	rm -f linux.s19 romfs.s19 flash.s19
	rm -f linux.img romdisk.img
	rm -f linux.b romfs.b flash.b
