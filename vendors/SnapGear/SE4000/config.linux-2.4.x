#
# Automatically generated make config: don't edit
#
CONFIG_ARM=y
# CONFIG_EISA is not set
# CONFIG_SBUS is not set
# CONFIG_MCA is not set
CONFIG_UID16=y
CONFIG_RWSEM_GENERIC_SPINLOCK=y
# CONFIG_RWSEM_XCHGADD_ALGORITHM is not set
# CONFIG_GENERIC_BUST_SPINLOCK is not set
# CONFIG_GENERIC_ISA_DMA is not set

#
# Code maturity level options
#
CONFIG_EXPERIMENTAL=y
# CONFIG_ADVANCED_OPTIONS is not set
# CONFIG_OBSOLETE is not set

#
# Loadable module support
#
CONFIG_MODULES=y
# CONFIG_MODVERSIONS is not set
# CONFIG_KMOD is not set

#
# System Type
#
# CONFIG_ARCH_ADIFCC is not set
# CONFIG_ARCH_ANAKIN is not set
# CONFIG_ARCH_ARCA5K is not set
# CONFIG_ARCH_CLPS7500 is not set
# CONFIG_ARCH_CLPS711X is not set
# CONFIG_ARCH_CO285 is not set
# CONFIG_ARCH_EBSA110 is not set
# CONFIG_ARCH_CAMELOT is not set
# CONFIG_ARCH_FOOTBRIDGE is not set
# CONFIG_ARCH_INTEGRATOR is not set
# CONFIG_ARCH_KS8695 is not set
# CONFIG_ARCH_IOP3XX is not set
# CONFIG_ARCH_IXP1200 is not set
# CONFIG_ARCH_IXP2000 is not set
CONFIG_ARCH_IXP425=y
# CONFIG_ARCH_OMAHA is not set
# CONFIG_ARCH_L7200 is not set
# CONFIG_ARCH_MX1ADS is not set
# CONFIG_ARCH_RPC is not set
# CONFIG_ARCH_RISCSTATION is not set
# CONFIG_ARCH_SA1100 is not set
# CONFIG_ARCH_SHARK is not set
# CONFIG_ARCH_AT91RM9200 is not set

#
# Archimedes/A5000 Implementations
#

#
# Archimedes/A5000 Implementations (select only ONE)
#

#
# Footbridge Implementations
#

#
# SA11x0 Implementations
#
# CONFIG_SA1100_H3XXX is not set

#
# IXP425 Implementation Options
#
# CONFIG_ARCH_IXDP425 is not set
# CONFIG_ARCH_IXCDP1100 is not set
# CONFIG_ARCH_PRPMC1100 is not set
# CONFIG_ARCH_ADI_COYOTE is not set
CONFIG_ARCH_SE4000=y

#
# AT91RM9200 Implementations
#

#
# CLPS711X/EP721X Implementations
#
# CONFIG_ARCH_EP7211 is not set
# CONFIG_ARCH_EP7212 is not set
# CONFIG_ARCH_ACORN is not set
# CONFIG_FOOTBRIDGE is not set
# CONFIG_FOOTBRIDGE_HOST is not set
# CONFIG_FOOTBRIDGE_ADDIN is not set

#
# Processor Type
#
CONFIG_CPU_32=y
# CONFIG_CPU_26 is not set
# CONFIG_CPU_ARM610 is not set
# CONFIG_CPU_ARM710 is not set
# CONFIG_CPU_ARM720T is not set
# CONFIG_CPU_ARM920T is not set
# CONFIG_CPU_ARM922T is not set
# CONFIG_PLD is not set
# CONFIG_CPU_ARM926T is not set
# CONFIG_CPU_ARM1020 is not set
# CONFIG_CPU_ARM1026 is not set
# CONFIG_CPU_SA110 is not set
# CONFIG_CPU_SA1100 is not set
# CONFIG_CPU_32v3 is not set
# CONFIG_CPU_32v4 is not set
CONFIG_CPU_32v5=y
CONFIG_CPU_XSCALE=y
CONFIG_ARM_THUMB=y

#
# Processor Features
#
# CONFIG_XSCALE_PMU_TIMER is not set
# CONFIG_XSCALE_CACHE_ERRATA is not set
# CONFIG_XSCALE_BDI2000 is not set
# CONFIG_DISCONTIGMEM is not set
CONFIG_CPU_BIG_ENDIAN=y

#
# General setup
#
CONFIG_PCI=y
CONFIG_PCI_AUTOCONFIG=y
# CONFIG_ISA is not set
# CONFIG_ISA_DMA is not set
CONFIG_KERNEL_START=0xc0000000
# CONFIG_ZBOOT_ROM is not set
CONFIG_ZBOOT_ROM_TEXT=0
CONFIG_ZBOOT_ROM_BSS=0
# CONFIG_PCI_NAMES is not set
# CONFIG_HOTPLUG is not set
# CONFIG_PCMCIA is not set
CONFIG_NET=y
CONFIG_SYSVIPC=y
# CONFIG_BSD_PROCESS_ACCT is not set
CONFIG_SYSCTL=y

#
# At least one math emulation must be selected
#
CONFIG_FPE_NWFPE=y
# CONFIG_FPE_NWFPE_XP is not set
# CONFIG_FPE_FASTFPE is not set
CONFIG_KCORE_ELF=y
# CONFIG_KCORE_AOUT is not set
# CONFIG_BINFMT_AOUT is not set
CONFIG_BINFMT_ELF=y
# CONFIG_BINFMT_MISC is not set
# CONFIG_PM is not set
# CONFIG_ARTHUR is not set
CONFIG_CMDLINE="console=ttyS0,115200 root=/dev/ram0 initrd=0x00800000,8M mem=32M@0x00000000"
CONFIG_ALIGNMENT_TRAP=y

#
# Parallel port support
#
# CONFIG_PARPORT is not set

#
# Memory Technology Devices (MTD)
#
CONFIG_MTD=y
# CONFIG_MTD_DEBUG is not set
CONFIG_MTD_PARTITIONS=y
# CONFIG_MTD_CONCAT is not set
CONFIG_MTD_REDBOOT_PARTS=y
# CONFIG_MTD_CMDLINE_PARTS is not set
# CONFIG_MTD_AFS_PARTS is not set

#
# User Modules And Translation Layers
#
CONFIG_MTD_CHAR=y
CONFIG_MTD_BLOCK=y
# CONFIG_FTL is not set
# CONFIG_NFTL is not set
# CONFIG_INFTL is not set

#
# RAM/ROM/Flash chip drivers
#
CONFIG_MTD_CFI=y
# CONFIG_MTD_JEDECPROBE is not set
CONFIG_MTD_GEN_PROBE=y
# CONFIG_MTD_CFI_ADV_OPTIONS is not set
CONFIG_MTD_CFI_INTELEXT=y
# CONFIG_MTD_CFI_AMDSTD is not set
# CONFIG_MTD_CFI_STAA is not set
# CONFIG_MTD_RAM is not set
# CONFIG_MTD_ROM is not set
# CONFIG_MTD_ABSENT is not set
# CONFIG_MTD_OBSOLETE_CHIPS is not set

#
# Mapping drivers for chip access
#
# CONFIG_MTD_PHYSMAP is not set
# CONFIG_MTD_NETtel is not set
# CONFIG_MTD_SNAPGEODE is not set
# CONFIG_MTD_NETteluC is not set
# CONFIG_MTD_MBVANILLA is not set
# CONFIG_MTD_KeyTechnology is not set
# CONFIG_MTD_NORA is not set
# CONFIG_MTD_ARM_INTEGRATOR is not set
# CONFIG_MTD_IQ80310 is not set
# CONFIG_MTD_FORTUNET is not set
# CONFIG_MTD_IXP425 is not set
CONFIG_MTD_SE4000=y
# CONFIG_MTD_SNAPARM is not set
# CONFIG_MTD_EDB7312 is not set
# CONFIG_MTD_M5272C3 is not set
# CONFIG_MTD_PCI is not set

#
# Self-contained MTD device drivers
#
# CONFIG_MTD_PMC551 is not set
# CONFIG_MTD_SLRAM is not set
# CONFIG_MTD_MTDRAM is not set
# CONFIG_MTD_BLKMTD is not set

#
# Disk-On-Chip Device Drivers
#
# CONFIG_MTD_DOC1000 is not set
# CONFIG_MTD_DOC2000 is not set
# CONFIG_MTD_DOC2001 is not set
# CONFIG_MTD_DOC2001PLUS is not set
# CONFIG_MTD_DOCPROBE is not set

#
# NAND Flash Device Drivers
#
# CONFIG_MTD_NAND is not set

#
# Plug and Play configuration
#
# CONFIG_PNP is not set

#
# Block devices
#
# CONFIG_BLK_DEV_FD is not set
# CONFIG_BLK_CPQ_DA is not set
# CONFIG_BLK_CPQ_CISS_DA is not set
# CONFIG_BLK_DEV_DAC960 is not set
# CONFIG_BLK_DEV_UMEM is not set
# CONFIG_BLK_DEV_LOOP is not set
# CONFIG_BLK_DEV_NBD is not set
CONFIG_BLK_DEV_RAM=y
CONFIG_BLK_DEV_RAM_SIZE=16384
CONFIG_BLK_DEV_INITRD=y
# CONFIG_BLK_DEV_RAMDISK_DATA is not set
# CONFIG_BLK_DEV_BLKMEM is not set
# CONFIG_BLK_STATS is not set

#
# Multi-device support (RAID and LVM)
#
# CONFIG_MD is not set

#
# Networking options
#
CONFIG_PACKET=y
# CONFIG_PACKET_MMAP is not set
# CONFIG_NETLINK_DEV is not set
CONFIG_NETFILTER=y
# CONFIG_NETFILTER_DEBUG is not set
CONFIG_FILTER=y
CONFIG_UNIX=y
CONFIG_INET=y
CONFIG_IP_MULTICAST=y
CONFIG_IP_ADVANCED_ROUTER=y
CONFIG_IP_MULTIPLE_TABLES=y
CONFIG_IP_ROUTE_FWMARK=y
# CONFIG_IP_ROUTE_NAT is not set
# CONFIG_IP_ROUTE_MULTIPATH is not set
CONFIG_IP_ROUTE_TOS=y
# CONFIG_IP_ROUTE_VERBOSE is not set
# CONFIG_IP_PNP is not set
# CONFIG_NET_ARP_LIMIT is not set
# CONFIG_NET_IPIP is not set
# CONFIG_NET_IPGRE is not set
# CONFIG_IP_MROUTE is not set
# CONFIG_ARPD is not set
# CONFIG_INET_ECN is not set
CONFIG_SYN_COOKIES=y

#
#   IP: Netfilter Configuration
#
CONFIG_IP_NF_CONNTRACK=y
CONFIG_IP_NF_FTP=y
# CONFIG_IP_NF_CONNTRACK_MARK is not set
CONFIG_IP_NF_H323=y
# CONFIG_IP_NF_AMANDA is not set
# CONFIG_IP_NF_TFTP is not set
CONFIG_IP_NF_IRC=y
CONFIG_IP_NF_CT_PROTO_GRE=y
CONFIG_IP_NF_PPTP=y
# CONFIG_IP_NF_QUEUE is not set
CONFIG_IP_NF_IPTABLES=y
CONFIG_IP_NF_MATCH_LIMIT=y
# CONFIG_IP_NF_MATCH_IPRANGE is not set
CONFIG_IP_NF_MATCH_MAC=y
# CONFIG_IP_NF_MATCH_PKTTYPE is not set
CONFIG_IP_NF_MATCH_MARK=y
CONFIG_IP_NF_MATCH_MULTIPORT=y
CONFIG_IP_NF_MATCH_TOS=y
# CONFIG_IP_NF_MATCH_TIME is not set
# CONFIG_IP_NF_MATCH_RECENT is not set
CONFIG_IP_NF_MATCH_ECN=y
CONFIG_IP_NF_MATCH_DSCP=y
# CONFIG_IP_NF_MATCH_AH_ESP is not set
CONFIG_IP_NF_MATCH_LENGTH=y
CONFIG_IP_NF_MATCH_TTL=y
CONFIG_IP_NF_MATCH_TCPMSS=y
CONFIG_IP_NF_MATCH_HELPER=y
CONFIG_IP_NF_MATCH_STATE=y
# CONFIG_IP_NF_MATCH_CONNTRACK is not set
# CONFIG_IP_NF_MATCH_UNCLEAN is not set
# CONFIG_IP_NF_MATCH_STRING is not set
# CONFIG_IP_NF_MATCH_OWNER is not set
CONFIG_IP_NF_FILTER=y
CONFIG_IP_NF_TARGET_REJECT=y
# CONFIG_IP_NF_TARGET_MIRROR is not set
CONFIG_IP_NF_NAT=y
CONFIG_IP_NF_NAT_NEEDED=y
CONFIG_IP_NF_TARGET_MASQUERADE=y
CONFIG_IP_NF_TARGET_REDIRECT=y
# CONFIG_IP_NF_TARGET_NETMAP is not set
CONFIG_IP_NF_NAT_H323=y
CONFIG_IP_NF_NAT_LOCAL=y
# CONFIG_IP_NF_NAT_SNMP_BASIC is not set
CONFIG_IP_NF_NAT_IRC=y
CONFIG_IP_NF_NAT_FTP=y
CONFIG_IP_NF_NAT_PPTP=y
CONFIG_IP_NF_NAT_PROTO_GRE=y
CONFIG_IP_NF_MANGLE=y
CONFIG_IP_NF_TARGET_TOS=y
CONFIG_IP_NF_TARGET_ECN=y
CONFIG_IP_NF_TARGET_DSCP=y
CONFIG_IP_NF_TARGET_MARK=y
CONFIG_IP_NF_TARGET_LOG=y
# CONFIG_IP_NF_TARGET_ULOG is not set
CONFIG_IP_NF_TARGET_TCPMSS=y
# CONFIG_IP_NF_ARPTABLES is not set

#
#   IP: Virtual Server Configuration
#
# CONFIG_IP_VS is not set
# CONFIG_IPV6 is not set
# CONFIG_KHTTPD is not set

#
#    SCTP Configuration (EXPERIMENTAL)
#
CONFIG_IPV6_SCTP__=y
# CONFIG_IP_SCTP is not set
# CONFIG_ATM is not set
# CONFIG_VLAN_8021Q is not set

#
#  
#
# CONFIG_IPX is not set
# CONFIG_ATALK is not set

#
# Appletalk devices
#
# CONFIG_DECNET is not set
# CONFIG_BRIDGE is not set
# CONFIG_X25 is not set
# CONFIG_LAPB is not set
# CONFIG_LLC is not set
# CONFIG_NET_DIVERT is not set
# CONFIG_ECONET is not set
# CONFIG_WAN_ROUTER is not set
# CONFIG_NET_FASTROUTE is not set
# CONFIG_NET_HW_FLOWCONTROL is not set

#
# QoS and/or fair queueing
#
CONFIG_NET_SCHED=y
CONFIG_NET_SCH_CBQ=y
CONFIG_NET_SCH_HTB=y
CONFIG_NET_SCH_CSZ=y
CONFIG_NET_SCH_PRIO=y
CONFIG_NET_SCH_RED=y
CONFIG_NET_SCH_SFQ=y
CONFIG_NET_SCH_TEQL=y
CONFIG_NET_SCH_TBF=y
CONFIG_NET_SCH_GRED=y
CONFIG_NET_SCH_DSMARK=y
CONFIG_NET_SCH_INGRESS=y
CONFIG_NET_QOS=y
CONFIG_NET_ESTIMATOR=y
CONFIG_NET_CLS=y
CONFIG_NET_CLS_TCINDEX=y
CONFIG_NET_CLS_ROUTE4=y
CONFIG_NET_CLS_ROUTE=y
CONFIG_NET_CLS_FW=y
CONFIG_NET_CLS_U32=y
CONFIG_NET_CLS_RSVP=y
# CONFIG_NET_CLS_RSVP6 is not set
CONFIG_NET_CLS_POLICE=y
CONFIG_IPSEC=y

#
# IPSec options (FreeS/WAN)
#
CONFIG_IPSEC_IPIP=y
CONFIG_IPSEC_ALG=y
CONFIG_IPSEC_ALG_AES=y
CONFIG_IPSEC_AH=y
CONFIG_IPSEC_AUTH_HMAC_MD5=y
CONFIG_IPSEC_AUTH_HMAC_SHA1=y
CONFIG_IPSEC_ESP=y
CONFIG_IPSEC_ENC_DES=y
CONFIG_IPSEC_ENC_3DES=y
CONFIG_IPSEC_IPCOMP=y
# CONFIG_IPSEC_IPCOMP_LZS is not set
CONFIG_IPSEC_DEBUG=y
CONFIG_IPSEC_NAT_TRAVERSAL=y

#
# Network testing
#
# CONFIG_NET_PKTGEN is not set

#
# Network device support
#
CONFIG_NETDEVICES=y

#
# ARCnet devices
#
# CONFIG_ARCNET is not set
# CONFIG_DUMMY is not set
# CONFIG_BONDING is not set
# CONFIG_EQUALIZER is not set
# CONFIG_TUN is not set
# CONFIG_ETHERTAP is not set

#
# Ethernet (10 or 100Mbit)
#
CONFIG_NET_ETHERNET=y
# CONFIG_ARM_CIRRUS is not set
# CONFIG_HAPPYMEAL is not set
# CONFIG_SUNGEM is not set
# CONFIG_NET_VENDOR_3COM is not set
# CONFIG_NET_VENDOR_SMC is not set
# CONFIG_NET_VENDOR_RACAL is not set
# CONFIG_HP100 is not set
# CONFIG_NET_PCI is not set
# CONFIG_NET_POCKET is not set
# CONFIG_FEC is not set
# CONFIG_CS89x0 is not set
# CONFIG_UCCS8900 is not set

#
# Ethernet (1000 Mbit)
#
# CONFIG_ACENIC is not set
# CONFIG_DL2K is not set
# CONFIG_E1000 is not set
# CONFIG_NS83820 is not set
# CONFIG_HAMACHI is not set
# CONFIG_YELLOWFIN is not set
# CONFIG_R8169 is not set
# CONFIG_SK98LIN is not set
# CONFIG_TIGON3 is not set
# CONFIG_FDDI is not set
# CONFIG_HIPPI is not set
CONFIG_PPP=y
# CONFIG_PPP_MULTILINK is not set
# CONFIG_PPP_FILTER is not set
CONFIG_PPP_ASYNC=y
# CONFIG_PPP_SYNC_TTY is not set
CONFIG_PPP_DEFLATE=y
CONFIG_PPP_BSDCOMP=y
CONFIG_PPPOE=y
CONFIG_SLIP=y
CONFIG_SLIP_COMPRESSED=y
# CONFIG_SLIP_SMART is not set
# CONFIG_SLIP_MODE_SLIP6 is not set

#
# Wireless LAN (non-hamradio)
#
# CONFIG_NET_RADIO is not set

#
# Token Ring devices
#
# CONFIG_TR is not set
# CONFIG_NET_FC is not set
# CONFIG_RCPCI is not set
# CONFIG_SHAPER is not set

#
# Wan interfaces
#
# CONFIG_WAN is not set

#
# Amateur Radio support
#
# CONFIG_HAMRADIO is not set

#
# IrDA (infrared) support
#
# CONFIG_IRDA is not set

#
# ATA/ATAPI/MFM/RLL support
#
# CONFIG_IDE is not set
# CONFIG_BLK_DEV_IDE_MODES is not set
# CONFIG_BLK_DEV_HD is not set

#
# SCSI support
#
# CONFIG_SCSI is not set

#
# IEEE 1394 (FireWire) support (EXPERIMENTAL)
#
# CONFIG_IEEE1394 is not set

#
# I2O device support
#
# CONFIG_I2O is not set

#
# ISDN subsystem
#
# CONFIG_ISDN is not set

#
# Input core support
#
# CONFIG_INPUT is not set

#
# Character devices
#
CONFIG_LEDMAN=y
# CONFIG_DS1302 is not set
# CONFIG_VT is not set
CONFIG_SERIAL=y
CONFIG_SERIAL_CONSOLE=y
# CONFIG_SERIAL_EXTENDED is not set
# CONFIG_SERIAL_NONSTANDARD is not set
# CONFIG_UNIX98_PTYS is not set

#
# I2C support
#
# CONFIG_I2C is not set

#
# Mice
#
# CONFIG_BUSMOUSE is not set
# CONFIG_MOUSE is not set

#
# Joysticks
#
# CONFIG_INPUT_GAMEPORT is not set

#
# Input core support is needed for gameports
#

#
# Input core support is needed for joysticks
#
# CONFIG_QIC02_TAPE is not set
# CONFIG_IPMI_HANDLER is not set

#
# Controller Area Network Cards/Chips
#
# CONFIG_CAN4LINUX is not set

#
# Watchdog Cards
#
# CONFIG_WATCHDOG is not set
# CONFIG_SCx200_GPIO is not set
# CONFIG_AMD_PM768 is not set
# CONFIG_NVRAM is not set
# CONFIG_RTC is not set
# CONFIG_DTLK is not set
# CONFIG_R3964 is not set
# CONFIG_APPLICOM is not set

#
# Ftape, the floppy tape device driver
#
# CONFIG_FTAPE is not set
# CONFIG_AGP is not set

#
# Direct Rendering Manager (XFree86 DRI support)
#
# CONFIG_DRM is not set

#
# Multimedia devices
#
# CONFIG_VIDEO_DEV is not set

#
# File systems
#
# CONFIG_QUOTA is not set
# CONFIG_AUTOFS_FS is not set
# CONFIG_AUTOFS4_FS is not set
# CONFIG_REISERFS_FS is not set
# CONFIG_ADFS_FS is not set
# CONFIG_AFFS_FS is not set
# CONFIG_HFS_FS is not set
# CONFIG_HFSPLUS_FS is not set
# CONFIG_BEFS_FS is not set
# CONFIG_BFS_FS is not set
# CONFIG_EXT3_FS is not set
# CONFIG_JBD is not set
# CONFIG_FAT_FS is not set
# CONFIG_EFS_FS is not set
# CONFIG_JFFS_FS is not set
# CONFIG_JFFS2_FS is not set
CONFIG_CRAMFS=y
# CONFIG_TMPFS is not set
CONFIG_RAMFS=y
# CONFIG_ISO9660_FS is not set
# CONFIG_JFS_FS is not set
# CONFIG_MINIX_FS is not set
# CONFIG_VXFS_FS is not set
# CONFIG_NTFS_FS is not set
# CONFIG_HPFS_FS is not set
CONFIG_PROC_FS=y
# CONFIG_DEVFS_FS is not set
# CONFIG_QNX4FS_FS is not set
# CONFIG_ROMFS_FS is not set
CONFIG_EXT2_FS=y
# CONFIG_SYSV_FS is not set
# CONFIG_UDF_FS is not set
# CONFIG_UFS_FS is not set

#
# Network File Systems
#
# CONFIG_CODA_FS is not set
# CONFIG_INTERMEZZO_FS is not set
# CONFIG_NFS_FS is not set
# CONFIG_NFSD is not set
# CONFIG_SUNRPC is not set
# CONFIG_LOCKD is not set
# CONFIG_SMB_FS is not set
# CONFIG_NCP_FS is not set
# CONFIG_ZISOFS_FS is not set
# CONFIG_COREDUMP_PRINTK is not set

#
# Partition Types
#
CONFIG_PARTITION_ADVANCED=y
# CONFIG_ACORN_PARTITION is not set
# CONFIG_OSF_PARTITION is not set
# CONFIG_AMIGA_PARTITION is not set
# CONFIG_ATARI_PARTITION is not set
# CONFIG_MAC_PARTITION is not set
# CONFIG_MSDOS_PARTITION is not set
# CONFIG_LDM_PARTITION is not set
# CONFIG_SGI_PARTITION is not set
# CONFIG_ULTRIX_PARTITION is not set
# CONFIG_SUN_PARTITION is not set
# CONFIG_EFI_PARTITION is not set
# CONFIG_SMB_NLS is not set
# CONFIG_NLS is not set

#
# Sound
#
# CONFIG_SOUND is not set

#
# Misc devices
#

#
# USB support
#
# CONFIG_USB is not set

#
# Support for USB gadgets
#
# CONFIG_USB_GADGET is not set

#
# Bluetooth support
#
# CONFIG_BLUEZ is not set

#
# Kernel hacking
#
CONFIG_FRAME_POINTER=y
# CONFIG_DEBUG_USER is not set
# CONFIG_DEBUG_INFO is not set
CONFIG_DEBUG_KERNEL=y
# CONFIG_DEBUG_SLAB is not set
# CONFIG_MAGIC_SYSRQ is not set
# CONFIG_DEBUG_SPINLOCK is not set
# CONFIG_DEBUG_WAITQ is not set
# CONFIG_DEBUG_BUGVERBOSE is not set
# CONFIG_DEBUG_ERRORS is not set
CONFIG_DEBUG_LL=y
CONFIG_LOG_BUF_SHIFT=0

#
# Cryptographic options
#
# CONFIG_CRYPTO is not set

#
# Library routines
#
# CONFIG_CRC32 is not set
CONFIG_ZLIB_INFLATE=y
CONFIG_ZLIB_DEFLATE=y
