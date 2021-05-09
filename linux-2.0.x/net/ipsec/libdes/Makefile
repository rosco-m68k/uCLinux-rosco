
.EXPORT_ALL_VARIABLES:

EXTRA_CFLAGS += -I$(ROOTDIR)/freeswan/libdes -I$(ROOTDIR)/lib/libc/include

VPATH = $(ROOTDIR)/freeswan/libdes

#ifdef CONFIG_COLDFIRE
COLDFIRE_KERNEL = y # get the ASM functions
#endif

CONFIG_HW_ASSIST := y

include $(ROOTDIR)/freeswan/libdes/Makefile

