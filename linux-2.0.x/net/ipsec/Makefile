
.EXPORT_ALL_VARIABLES:

EXTRA_CFLAGS += -I$(ROOTDIR)/freeswan -I$(ROOTDIR)/freeswan/lib \
			-I$(ROOTDIR)/lib/libc/include \
			-I$(ROOTDIR)/freeswan/libdes

VPATH = $(ROOTDIR)/freeswan/klips/net/ipsec

ARCH = echo $(MACHINE)

include $(ROOTDIR)/freeswan/klips/net/ipsec/Makefile

