/*
 *  Host Resources MIB - disk device group implementation - hr_disk.c
 *
 */

#include <net-snmp/net-snmp-config.h>
#include "host_res.h"
#include "hr_disk.h"
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_KVM_H
#include <kvm.h>
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#else
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_SYS_DKIO_H
#include <sys/dkio.h>
#endif
#if HAVE_SYS_DISKIO_H           /* HP-UX only ? */
#include <sys/diskio.h>
#endif
#if HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif
#if HAVE_SYS_DISKLABEL_H
#define DKTYPENAMES
#include <sys/disklabel.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef linux
/*
 * define BLKGETSIZE from <linux/fs.h>:
 * Note: cannot include this file completely due to errors with redefinition
 * of structures (at least with older linux versions) --jsf
 */
#define BLKGETSIZE _IO(0x12,96) /* return device size */
#endif

#include <net-snmp/agent/agent_read_config.h>
#include <net-snmp/library/read_config.h>

#define HRD_MONOTONICALLY_INCREASING

        /*********************
	 *
	 *  Kernel & interface information,
	 *   and internal forward declarations
	 *
	 *********************/

void            Init_HR_Disk(void);
int             Get_Next_HR_Disk(void);
int             Get_Next_HR_Disk_Partition(char *, int);
static void     Add_HR_Disk_entry(const char *, int, int, int, int,
                                  const char *, int, int);
static void     Save_HR_Disk_General(void);
static void     Save_HR_Disk_Specific(void);
static int      Query_Disk(int, const char *);
static int      Is_It_Writeable(void);
static int      What_Type_Disk(void);
static int      Is_It_Removeable(void);
static const char *describe_disk(int);

int             header_hrdisk(struct variable *, oid *, size_t *, int,
                              size_t *, WriteMethod **);

static int      HRD_type_index;
static int      HRD_index;
static char     HRD_savedModel[40];
static long     HRD_savedCapacity = 1044;
static int      HRD_savedFlags;
static time_t   HRD_history[HRDEV_TYPE_MASK];

#ifdef DIOC_DESCRIBE
static disk_describe_type HRD_info;
static capacity_type HRD_cap;

static int      HRD_savedIntf_type;
static int      HRD_savedDev_type;
#endif

#ifdef DKIOCINFO
static struct dk_cinfo HRD_info;
static struct dk_geom HRD_cap;

static int      HRD_savedCtrl_type;
#endif

#ifdef HAVE_LINUX_HDREG_H
static struct hd_driveid HRD_info;
#endif

#ifdef DIOCGDINFO
static struct disklabel HRD_info;
#endif

static void     parse_disk_config(const char *, char *);
static void     free_disk_config(void);

        /*********************
	 *
	 *  Initialisation & common implementation functions
	 *
	 *********************/

#define	HRDISK_ACCESS		1
#define	HRDISK_MEDIA		2
#define	HRDISK_REMOVEABLE	3
#define	HRDISK_CAPACITY		4

struct variable4 hrdisk_variables[] = {
    {HRDISK_ACCESS, ASN_INTEGER, RONLY, var_hrdisk, 2, {1, 1}},
    {HRDISK_MEDIA, ASN_INTEGER, RONLY, var_hrdisk, 2, {1, 2}},
    {HRDISK_REMOVEABLE, ASN_INTEGER, RONLY, var_hrdisk, 2, {1, 3}},
    {HRDISK_CAPACITY, ASN_INTEGER, RONLY, var_hrdisk, 2, {1, 4}}
};
oid             hrdisk_variables_oid[] = { 1, 3, 6, 1, 2, 1, 25, 3, 6 };


void
init_hr_disk(void)
{
    int             i;

    init_device[HRDEV_DISK] = Init_HR_Disk;
    next_device[HRDEV_DISK] = Get_Next_HR_Disk;
    save_device[HRDEV_DISK] = Save_HR_Disk_General;
#ifdef HRD_MONOTONICALLY_INCREASING
    dev_idx_inc[HRDEV_DISK] = 1;
#endif

#if defined(linux)
    Add_HR_Disk_entry("/dev/hd%c%d", -1, -1, 'a', 'l', "/dev/hd%c", 1, 15);
    Add_HR_Disk_entry("/dev/sd%c%d", -1, -1, 'a', 'p', "/dev/sd%c", 1, 15);
    Add_HR_Disk_entry("/dev/md%d", -1, -1, 0, 3, "/dev/md%d", 0, 0);
    Add_HR_Disk_entry("/dev/fd%d", -1, -1, 0, 1, "/dev/fd%d", 0, 0);
#elif defined(hpux)
#if defined(hpux10) || defined(hpux11)
    Add_HR_Disk_entry("/dev/rdsk/c%dt%xd%d", 0, 1, 0, 15,
                      "/dev/rdsk/c%dt%xd0", 0, 4);
#else                           /* hpux9 */
    Add_HR_Disk_entry("/dev/rdsk/c%dd%xs%d", 201, 201, 0, 15,
                      "/dev/rdsk/c%dd%xs0", 0, 4);
#endif
#elif defined(solaris2)
    Add_HR_Disk_entry("/dev/rdsk/c%dt%dd0s%d", 0, 1, 0, 15,
                      "/dev/rdsk/c%dt%dd0s0", 0, 7);
    Add_HR_Disk_entry("/dev/rdsk/c%dd%ds%d", 0, 1, 0, 15,
                      "/dev/rdsk/c%dd%ds0", 0, 7);
#elif defined(freebsd4) || defined(freebsd5)
    Add_HR_Disk_entry("/dev/ad%ds%d%c", 0, 1, 1, 4, "/dev/ad%ds%d", 'a', 'h');
    Add_HR_Disk_entry("/dev/da%ds%d%c", 0, 1, 1, 4, "/dev/da%ds%d", 'a', 'h');
#elif defined(freebsd3)
    Add_HR_Disk_entry("/dev/wd%ds%d%c", 0, 1, 1, 4, "/dev/wd%ds%d", 'a',
                      'h');
    Add_HR_Disk_entry("/dev/sd%ds%d%c", 0, 1, 1, 4, "/dev/sd%ds%d", 'a',
                      'h');
#elif defined(freebsd2)
    Add_HR_Disk_entry("/dev/wd%d%c", -1, -1, 0, 3, "/dev/wd%d", 'a', 'h');
    Add_HR_Disk_entry("/dev/sd%d%c", -1, -1, 0, 3, "/dev/sd%d", 'a', 'h');
#elif defined(netbsd1)
    Add_HR_Disk_entry("/dev/wd%d%c", -1, -1, 0, 3, "/dev/wd%dc", 'a', 'h');
    Add_HR_Disk_entry("/dev/sd%d%c", -1, -1, 0, 3, "/dev/sd%dc", 'a', 'h');
#endif

    device_descr[HRDEV_DISK] = describe_disk;
    HRD_savedModel[0] = '\0';
    HRD_savedCapacity = 0;

    for (i = 0; i < HRDEV_TYPE_MASK; ++i)
        HRD_history[i] = -1;

    REGISTER_MIB("host/hr_disk", hrdisk_variables, variable4,
                 hrdisk_variables_oid);

    snmpd_register_config_handler("ignoredisk", parse_disk_config,
                                  free_disk_config, "name");
}

#define ITEM_STRING	1
#define ITEM_SET	2
#define ITEM_STAR	3
#define ITEM_ANY	4

typedef unsigned char details_set[32];

typedef struct _conf_disk_item {
    int             item_type;  /* ITEM_STRING, ITEM_SET, ITEM_STAR, ITEM_ANY */
    void           *item_details;       /* content depends upon item_type */
    struct _conf_disk_item *item_next;
} conf_disk_item;

typedef struct _conf_disk_list {
    conf_disk_item *list_item;
    struct _conf_disk_list *list_next;
} conf_disk_list;
static conf_disk_list *conf_list;

static int      match_disk_config(const char *);
static int      match_disk_config_item(const char *, conf_disk_item *);

static void
parse_disk_config(const char *token, char *cptr)
{
    conf_disk_list *d_new;
    conf_disk_item *di_curr;
    details_set    *d_set;
    char           *name, *p, *d_str, c;
    unsigned int    i, neg, c1, c2;

    name = strtok(cptr, " \t");
    if (!name) {
        config_perror("Missing NAME parameter");
        return;
    }
    d_new = (conf_disk_list *) malloc(sizeof(conf_disk_list));
    if (!d_new) {
        config_perror("Out of memory");
        return;
    }
    di_curr = (conf_disk_item *) malloc(sizeof(conf_disk_item));
    if (!di_curr) {
        config_perror("Out of memory");
        return;
    }
    d_new->list_item = di_curr;
    for (;;) {
        if (*name == '?') {
            di_curr->item_type = ITEM_ANY;
            di_curr->item_details = (void *) 0;
            name++;
        } else if (*name == '*') {
            di_curr->item_type = ITEM_STAR;
            di_curr->item_details = (void *) 0;
            name++;
        } else if (*name == '[') {
            d_set = (details_set *) malloc(sizeof(details_set));
            if (!d_set) {
                config_perror("Out of memory");
                return;
            }
            for (i = 0; i < sizeof(details_set); i++)
                (*d_set)[i] = (unsigned char) 0;
            name++;
            if (*name == '^' || *name == '!') {
                neg = 1;
                name++;
            } else {
                neg = 0;
            }
            while (*name && *name != ']') {
                c1 = ((unsigned int) *name++) & 0xff;
                if (*name == '-' && *(name + 1) != ']') {
                    name++;
                    c2 = ((unsigned int) *name++) & 0xff;
                } else {
                    c2 = c1;
                }
                for (i = c1; i <= c2; i++)
                    (*d_set)[i / 8] |= (unsigned char) (1 << (i % 8));
            }
            if (*name != ']') {
                config_perror
                    ("Syntax error in NAME: invalid set specified");
                return;
            }
            if (neg) {
                for (i = 0; i < sizeof(details_set); i++)
                    (*d_set)[i] = (*d_set)[i] ^ (unsigned char) 0xff;
            }
            di_curr->item_type = ITEM_SET;
            di_curr->item_details = (void *) d_set;
            name++;
        } else {
            for (p = name;
                 *p != '\0' && *p != '?' && *p != '*' && *p != '['; p++);
            c = *p;
            *p = '\0';
            d_str = (char *) malloc(strlen(name) + 1);
            if (!d_str) {
                config_perror("Out of memory");
                return;
            }
            strcpy(d_str, name);
            *p = c;
            di_curr->item_type = ITEM_STRING;
            di_curr->item_details = (void *) d_str;
            name = p;
        }
        if (!*name) {
            di_curr->item_next = (conf_disk_item *) 0;
            break;
        }
        di_curr->item_next =
            (conf_disk_item *) malloc(sizeof(conf_disk_item));
        if (!di_curr->item_next) {
            config_perror("Out of memory");
            return;
        }
        di_curr = di_curr->item_next;
    }
    d_new->list_next = conf_list;
    conf_list = d_new;
}

static void
free_disk_config(void)
{
    conf_disk_list *d_ptr = conf_list, *d_next;
    conf_disk_item *di_ptr, *di_next;

    while (d_ptr) {
        d_next = d_ptr->list_next;
        di_ptr = d_ptr->list_item;
        while (di_ptr) {
            di_next = di_ptr->item_next;
            if (di_ptr->item_details)
                free(di_ptr->item_details);
            free((void *) di_ptr);
            di_ptr = di_next;
        }
        free((void *) d_ptr);
        d_ptr = d_next;
    }
    conf_list = (conf_disk_list *) 0;
}

static int
match_disk_config_item(const char *name, conf_disk_item * di_ptr)
{
    int             result = 0;
    size_t          len;
    details_set    *d_set;
    unsigned int    c;

    if (di_ptr) {
        switch (di_ptr->item_type) {
        case ITEM_STRING:
            len = strlen((const char *) di_ptr->item_details);
            if (!strncmp(name, (const char *) di_ptr->item_details, len))
                result = match_disk_config_item(name + len,
                                                di_ptr->item_next);
            break;
        case ITEM_SET:
            if (*name) {
                d_set = (details_set *) di_ptr->item_details;
                c = ((unsigned int) *name) & 0xff;
                if ((*d_set)[c / 8] & (unsigned char) (1 << (c % 8)))
                    result = match_disk_config_item(name + 1,
                                                    di_ptr->item_next);
            }
            break;
        case ITEM_STAR:
            if (di_ptr->item_next) {
                for (; !result && *name; name++)
                    result = match_disk_config_item(name,
                                                    di_ptr->item_next);
            } else {
                result = 1;
            }
            break;
        case ITEM_ANY:
            if (*name)
                result = match_disk_config_item(name + 1,
                                                di_ptr->item_next);
            break;
        }
    } else {
        if (*name == '\0')
            result = 1;
    }

    return result;
}

static int
match_disk_config(const char *name)
{
    conf_disk_list *d_ptr = conf_list;

    while (d_ptr) {
        if (match_disk_config_item(name, d_ptr->list_item))
            return 1;           /* match found in ignorelist */
        d_ptr = d_ptr->list_next;
    }

    /*
     * no match in ignorelist 
     */
    return 0;
}

/*
 * header_hrdisk(...
 * Arguments:
 * vp     IN      - pointer to variable entry that points here
 * name    IN/OUT  - IN/name requested, OUT/name found
 * length  IN/OUT  - length of IN/OUT oid's 
 * exact   IN      - TRUE if an exact match was requested
 * var_len OUT     - length of variable or 0 if function returned
 * write_method
 */

int
header_hrdisk(struct variable *vp,
              oid * name,
              size_t * length,
              int exact, size_t * var_len, WriteMethod ** write_method)
{
#define HRDISK_ENTRY_NAME_LENGTH	11
    oid             newname[MAX_OID_LEN];
    int             disk_idx, LowIndex = -1;
    int             result;

    DEBUGMSGTL(("host/hr_disk", "var_hrdisk: "));
    DEBUGMSGOID(("host/hr_disk", name, *length));
    DEBUGMSG(("host/hr_disk", " %d\n", exact));

    memcpy((char *) newname, (char *) vp->name,
           (int) vp->namelen * sizeof(oid));
    /*
     * Find "next" disk entry 
     */

    Init_HR_Disk();
    for (;;) {
        disk_idx = Get_Next_HR_Disk();
        if (disk_idx == -1)
            break;
        newname[HRDISK_ENTRY_NAME_LENGTH] = disk_idx;
        result =
            snmp_oid_compare(name, *length, newname,
                             (int) vp->namelen + 1);
        if (exact && (result == 0)) {
            LowIndex = disk_idx;
            Save_HR_Disk_Specific();
            break;
        }
        if ((!exact && (result < 0)) &&
            (LowIndex == -1 || disk_idx < LowIndex)) {
            LowIndex = disk_idx;
            Save_HR_Disk_Specific();
#ifdef HRD_MONOTONICALLY_INCREASING
            break;
#endif
        }
    }

    if (LowIndex == -1) {
        DEBUGMSGTL(("host/hr_disk", "... index out of range\n"));
        return (MATCH_FAILED);
    }

    newname[HRDISK_ENTRY_NAME_LENGTH] = LowIndex;
    memcpy((char *) name, (char *) newname,
           ((int) vp->namelen + 1) * sizeof(oid));
    *length = vp->namelen + 1;
    *write_method = 0;
    *var_len = sizeof(long);    /* default to 'long' results */

    DEBUGMSGTL(("host/hr_disk", "... get disk stats "));
    DEBUGMSGOID(("host/hr_disk", name, *length));
    DEBUGMSG(("host/hr_disk", "\n"));

    return LowIndex;
}


        /*********************
	 *
	 *  System specific implementation functions
	 *
	 *********************/


u_char         *
var_hrdisk(struct variable * vp,
           oid * name,
           size_t * length,
           int exact, size_t * var_len, WriteMethod ** write_method)
{
    int             disk_idx;

    disk_idx =
        header_hrdisk(vp, name, length, exact, var_len, write_method);
    if (disk_idx == MATCH_FAILED)
        return NULL;


    switch (vp->magic) {
    case HRDISK_ACCESS:
        long_return = Is_It_Writeable();
        return (u_char *) & long_return;
    case HRDISK_MEDIA:
        long_return = What_Type_Disk();
        return (u_char *) & long_return;
    case HRDISK_REMOVEABLE:
        long_return = Is_It_Removeable();
        return (u_char *) & long_return;
    case HRDISK_CAPACITY:
        long_return = HRD_savedCapacity;
        return (u_char *) & long_return;
    default:
        DEBUGMSGTL(("snmpd", "unknown sub-id %d in var_hrdisk\n",
                    vp->magic));
    }
    return NULL;
}


        /*********************
	 *
	 *  Internal implementation functions
	 *
	 *********************/

#define MAX_NUMBER_DISK_TYPES	16      /* probably should be a variable */
#define MAX_DISKS_PER_TYPE	15      /* SCSI disks - not a hard limit */
#define	HRDISK_TYPE_SHIFT	4       /* log2 (MAX_DISKS_PER_TYPE+1) */

typedef struct {
    const char     *disk_devpart_string;        /* printf() format disk part. name */
    short           disk_controller;    /* controller id or -1 if NA */
    short           disk_device_first;  /* first device id */
    short           disk_device_last;   /* last device id */
    const char     *disk_devfull_string;        /* printf() format full disk name */
    short           disk_partition_first;       /* first partition id */
    short           disk_partition_last;        /* last partition id */
} HRD_disk_t;

static HRD_disk_t disk_devices[MAX_NUMBER_DISK_TYPES];
static int      HR_number_disk_types = 0;

static void
Add_HR_Disk_entry(const char *devpart_string,
                  int first_ctl,
                  int last_ctl,
                  int first_dev,
                  int last_dev,
                  const char *devfull_string,
                  int first_partn, int last_partn)
{
    while (first_ctl <= last_ctl) {
        disk_devices[HR_number_disk_types].disk_devpart_string =
            devpart_string;
        disk_devices[HR_number_disk_types].disk_controller = first_ctl;
        disk_devices[HR_number_disk_types].disk_device_first = first_dev;
        disk_devices[HR_number_disk_types].disk_device_last = last_dev;
        disk_devices[HR_number_disk_types].disk_devfull_string =
            devfull_string;
        disk_devices[HR_number_disk_types].disk_partition_first =
            first_partn;
        disk_devices[HR_number_disk_types].disk_partition_last =
            last_partn;

        /*
         * Split long runs of disks into separate "types"
         */
        while (last_dev - first_dev > MAX_DISKS_PER_TYPE) {
            first_dev = first_dev + MAX_DISKS_PER_TYPE;
            disk_devices[HR_number_disk_types].disk_device_last =
                first_dev - 1;
            HR_number_disk_types++;

            disk_devices[HR_number_disk_types].disk_devpart_string =
                devpart_string;
            disk_devices[HR_number_disk_types].disk_controller = first_ctl;
            disk_devices[HR_number_disk_types].disk_device_first =
                first_dev;
            disk_devices[HR_number_disk_types].disk_device_last = last_dev;
            disk_devices[HR_number_disk_types].disk_devfull_string =
                devfull_string;
            disk_devices[HR_number_disk_types].disk_partition_first =
                first_partn;
            disk_devices[HR_number_disk_types].disk_partition_last =
                last_partn;
        }

        first_ctl++;
        HR_number_disk_types++;
    }
}

void
Init_HR_Disk(void)
{
    HRD_type_index = 0;
    HRD_index = -1;
    DEBUGMSGTL(("host/hr_disk", "Init_Disk\n"));
}

int
Get_Next_HR_Disk(void)
{
    char            string[100];
    int             fd, result;
    int             iindex;
    int             max_disks;
    time_t          now;

    HRD_index++;
    (void *) time(&now);
    DEBUGMSGTL(("host/hr_disk", "Next_Disk type %d of %d\n",
                HRD_type_index, HR_number_disk_types));
    while (HRD_type_index < HR_number_disk_types) {
        max_disks = disk_devices[HRD_type_index].disk_device_last -
            disk_devices[HRD_type_index].disk_device_first + 1;
        DEBUGMSGTL(("host/hr_disk", "Next_Disk max %d of type %d\n",
                    max_disks, HRD_type_index));

        while (HRD_index < max_disks) {
            iindex = (HRD_type_index << HRDISK_TYPE_SHIFT) + HRD_index;

            /*
             * Check to see whether this device
             *   has been probed for 'recently'
             *   and skip if so.
             * This has a *major* impact on run
             *   times (by a factor of 10!)
             */
            if ((HRD_history[iindex] > 0) &&
                ((now - HRD_history[iindex]) < 60)) {
                HRD_index++;
                continue;
            }

            /*
             * Construct the full device name in "string" 
             */
            if (disk_devices[HRD_type_index].disk_controller != -1) {
                snprintf(string, sizeof(string),
                        disk_devices[HRD_type_index].disk_devfull_string,
                        disk_devices[HRD_type_index].disk_controller,
                        disk_devices[HRD_type_index].disk_device_first +
                        HRD_index);
            } else {
                snprintf(string, sizeof(string),
                        disk_devices[HRD_type_index].disk_devfull_string,
                        disk_devices[HRD_type_index].disk_device_first +
                        HRD_index);
            }
            string[ sizeof(string)-1 ] = 0;

            DEBUGMSGTL(("host/hr_disk", "Get_Next_HR_Disk: %s (%d/%d)\n",
                        string, HRD_type_index, HRD_index));

            if (HRD_history[iindex] == -1) {
                /*
                 * check whether this device is in the "ignoredisk" list in
                 * the config file. if yes this device will be marked as
                 * invalid for the future, i.e. it won't ever be checked
                 * again.
                 */
                if (match_disk_config(string)) {
                    /*
                     * device name matches entry in ignoredisk list 
                     */
                    DEBUGMSGTL(("host/hr_disk",
                                "Get_Next_HR_Disk: %s ignored\n", string));
                    HRD_history[iindex] = LONG_MAX;
                    HRD_index++;
                    continue;
                }
            }

            /*
             * use O_NDELAY to avoid CDROM spin-up and media detection
             * * (too slow) --okir 
             */
            /*
             * at least with HP-UX 11.0 this doesn't seem to work properly
             * * when accessing an empty CDROM device --jsf 
             */
#ifdef O_NDELAY                 /* I'm sure everything has it, but just in case...  --Wes */
            fd = open(string, O_RDONLY | O_NDELAY);
#else
            fd = open(string, O_RDONLY);
#endif
            if (fd != -1) {
                result = Query_Disk(fd, string);
                close(fd);
                if (result != -1) {
                    HRD_history[iindex] = 0;
                    return ((HRDEV_DISK << HRDEV_TYPE_SHIFT) + iindex);
                }
            }
            HRD_history[iindex] = now;
            HRD_index++;
        }
        HRD_type_index++;
        HRD_index = 0;
    }
    HRD_index = -1;
    return -1;
}

int
Get_Next_HR_Disk_Partition(char *string, int HRP_index)
{
    DEBUGMSGTL(("host/hr_disk", "Next_Partition type %d/%d:%d\n",
                HRD_type_index, HRD_type_index, HRP_index));

    /*
     * no more partition names => return -1 
     */
    if (disk_devices[HRD_type_index].disk_partition_last -
        disk_devices[HRD_type_index].disk_partition_first + 1
        <= HRP_index) {
        return -1;
    }

    /*
     * Construct the partition name in "string" 
     */
    if (disk_devices[HRD_type_index].disk_controller != -1) {
        snprintf(string, sizeof(string),
                disk_devices[HRD_type_index].disk_devpart_string,
                disk_devices[HRD_type_index].disk_controller,
                disk_devices[HRD_type_index].disk_device_first + HRD_index,
                disk_devices[HRD_type_index].disk_partition_first +
                HRP_index);
    } else {
        snprintf(string, sizeof(string),
                disk_devices[HRD_type_index].disk_devpart_string,
                disk_devices[HRD_type_index].disk_device_first + HRD_index,
                disk_devices[HRD_type_index].disk_partition_first +
                HRP_index);
    }
    string[ sizeof(string)-1 ] = 0;

    DEBUGMSGTL(("host/hr_disk",
                "Get_Next_HR_Disk_Partition: %s (%d/%d:%d)\n", string,
                HRD_type_index, HRD_index, HRP_index));

    return 0;
}

static void
Save_HR_Disk_Specific(void)
{
#ifdef DIOC_DESCRIBE
    HRD_savedIntf_type = HRD_info.intf_type;
    HRD_savedDev_type = HRD_info.dev_type;
    HRD_savedFlags = HRD_info.flags;
    HRD_savedCapacity = HRD_cap.lba / 2;
#endif
#ifdef DKIOCINFO
    HRD_savedCtrl_type = HRD_info.dki_ctype;
    HRD_savedFlags = HRD_info.dki_flags;
    HRD_savedCapacity = HRD_cap.dkg_ncyl * HRD_cap.dkg_nhead * HRD_cap.dkg_nsect / 2;   /* ??? */
#endif
#ifdef HAVE_LINUX_HDREG_H
    HRD_savedCapacity = HRD_info.lba_capacity / 2;
    HRD_savedFlags = HRD_info.config;
#endif
#ifdef DIOCGDINFO
    HRD_savedCapacity = HRD_info.d_secperunit / 2;
#endif
}

static void
Save_HR_Disk_General(void)
{
#ifdef DIOC_DESCRIBE
    strncpy(HRD_savedModel, HRD_info.model_num, sizeof(HRD_savedModel)-1);
    HRD_savedModel[ sizeof(HRD_savedModel)-1 ] = 0;
#endif
#ifdef DKIOCINFO
    strncpy(HRD_savedModel, HRD_info.dki_dname, sizeof(HRD_savedModel)-1);
    HRD_savedModel[ sizeof(HRD_savedModel)-1 ] = 0;
#endif
#ifdef HAVE_LINUX_HDREG_H
    strncpy(HRD_savedModel, (const char *) HRD_info.model,
                    sizeof(HRD_savedModel)-1);
    HRD_savedModel[ sizeof(HRD_savedModel)-1 ] = 0;
#endif
#ifdef DIOCGDINFO
    strncpy(HRD_savedModel, dktypenames[HRD_info.d_type],
                    sizeof(HRD_savedModel)-1);
    HRD_savedModel[ sizeof(HRD_savedModel)-1 ] = 0;
#endif
}

static const char *
describe_disk(int idx)
{
    if (HRD_savedModel[0] == '\0')
        return ("some sort of disk");
    else
        return (HRD_savedModel);
}


static int
Query_Disk(int fd, const char *devfull)
{
    int             result = -1;

#ifdef DIOC_DESCRIBE
    result = ioctl(fd, DIOC_DESCRIBE, &HRD_info);
    if (result != -1)
        result = ioctl(fd, DIOC_CAPACITY, &HRD_cap);
#endif

#ifdef DKIOCINFO
    result = ioctl(fd, DKIOCINFO, &HRD_info);
    if (result != -1)
        result = ioctl(fd, DKIOCGGEOM, &HRD_cap);
#endif

#ifdef HAVE_LINUX_HDREG_H
    if (HRD_type_index == 0)    /* IDE hard disk */
        result = ioctl(fd, HDIO_GET_IDENTITY, &HRD_info);
    else if (HRD_type_index <= 2) {     /* SCSI hard disk and md devices */
        long            h;
        result = ioctl(fd, BLKGETSIZE, &h);
        if (result != -1 && HRD_type_index == 2 && h == 0L)
            result = -1;        /* ignore empty md devices */
        if (result != -1) {
            HRD_info.lba_capacity = h;
            if (HRD_type_index == 1)
                snprintf( HRD_info.model, sizeof(HRD_info.model)-1,
                         "SCSI disk (%s)", devfull);
            else
                snprintf( HRD_info.model, sizeof(HRD_info.model)-1,
                        "RAID disk (%s)", devfull);
            HRD_info.model[ sizeof(HRD_info.model)-1 ] = 0;
            HRD_info.config = 0;
        }
    }
#endif

#ifdef DIOCGDINFO
    result = ioctl(fd, DIOCGDINFO, &HRD_info);
#endif

    return (result);
}


static int
Is_It_Writeable(void)
{
#ifdef DIOC_DESCRIBE
    if ((HRD_savedFlags & WRITE_PROTECT_FLAG) ||
        (HRD_savedDev_type == CDROM_DEV_TYPE))
        return (2);             /* read only */
#endif

#ifdef DKIOCINFO
    if (HRD_savedCtrl_type == DKC_CDROM)
        return (2);             /* read only */
#endif

    return (1);                 /* read-write */
}

static int
What_Type_Disk(void)
{
#ifdef DIOC_DESCRIBE
    switch (HRD_savedDev_type) {
    case DISK_DEV_TYPE:
        if (HRD_savedIntf_type == PC_FDC_INTF)
            return (4);         /* Floppy Disk */
        else
            return (3);         /* Hard Disk */
        break;
    case CDROM_DEV_TYPE:
        return (5);             /* Optical RO */
        break;
    case WORM_DEV_TYPE:
        return (6);             /* Optical WORM */
        break;
    case MO_DEV_TYPE:
        return (7);             /* Optical R/W */
        break;
    default:
        return (2);             /* Unknown */
        break;
    }
#endif

#ifdef DKIOCINFO
    switch (HRD_savedCtrl_type) {
    case DKC_WDC2880:
    case DKC_DSD5215:
#ifdef DKC_XY450
    case DKC_XY450:
#endif
    case DKC_ACB4000:
    case DKC_MD21:
#ifdef DKC_XD7053
    case DKC_XD7053:
#endif
    case DKC_SCSI_CCS:
#ifdef DKC_PANTHER
    case DKC_PANTHER:
#endif
#ifdef DKC_CDC_9057
    case DKC_CDC_9057:
#endif
#ifdef DKC_FJ_M1060
    case DKC_FJ_M1060:
#endif
    case DKC_DIRECT:
    case DKC_PCMCIA_ATA:
        return (3);             /* Hard Disk */
        break;
    case DKC_NCRFLOPPY:
    case DKC_SMSFLOPPY:
    case DKC_INTEL82077:
        return (4);             /* Floppy Disk */
        break;
    case DKC_CDROM:
        return (5);             /* Optical RO */
        break;
    case DKC_PCMCIA_MEM:
        return (8);             /* RAM disk */
        break;
    case DKC_MD:               /* "meta-disk" driver */
        return (1);             /* Other */
        break;
    }
#endif


    return (2);                 /* Unknown */
}

static int
Is_It_Removeable(void)
{
#ifdef DIOC_DESCRIBE
    if ((HRD_savedIntf_type == PC_FDC_INTF) ||
        (HRD_savedDev_type == WORM_DEV_TYPE) ||
        (HRD_savedDev_type == MO_DEV_TYPE) ||
        (HRD_savedDev_type == CDROM_DEV_TYPE))
        return (1);             /* true */
#endif

#ifdef DKIOCINFO
    if ((HRD_savedCtrl_type == DKC_CDROM) ||
        (HRD_savedCtrl_type == DKC_NCRFLOPPY) ||
        (HRD_savedCtrl_type == DKC_SMSFLOPPY) ||
        (HRD_savedCtrl_type == DKC_INTEL82077) ||
        (HRD_savedCtrl_type == DKC_PCMCIA_MEM) ||
        (HRD_savedCtrl_type == DKC_PCMCIA_ATA))
        return (1);             /* true */
#endif

#ifdef HAVE_LINUX_HDREG_H
    if (HRD_savedFlags & 0x80)
        return (1);             /* true */
#endif

    return (2);                 /* false */
}
