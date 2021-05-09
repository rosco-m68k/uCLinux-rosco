/*
 *  Template MIB group interface - file.h
 *
 */
#ifndef _MIBGROUP_FILE_H
#define _MIBGROUP_FILE_H

#include "mibdefs.h"

void            init_file(void);

/*
 * config file parsing routines 
 */
void            file_free_config(void);
void            file_parse_config(const char *, char *);
extern FindVarMethod var_file_table;

struct filestat {
    char            name[256];
    int             size;
    int             max;
};

#define FILE_ERROR_MSG  "%s: size exceeds %dkb (= %dkb)"

#define FILE_INDEX      1
#define FILE_NAME       2
#define FILE_SIZE       3
#define FILE_MAX        4
#define FILE_ERROR      100
#define FILE_MSG        101

#endif                          /* _MIBGROUP_FILE_H */
