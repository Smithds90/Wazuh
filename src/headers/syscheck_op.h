/*
 * Shared functions for Syscheck events decoding
 * Copyright (C) 2016 Wazuh Inc.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef __SYSCHECK_OP_H
#define __SYSCHECK_OP_H

#include "analysisd/eventinfo.h"

#ifndef WIN32

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#else

#include "shared.h"
#include "aclapi.h"

#define BUFFER_LEN 1024

#endif

/* Fields for rules */
#define SK_FILE     0
#define SK_SIZE     1
#define SK_PERM     2
#define SK_UID      3
#define SK_GID      4
#define SK_MD5      5
#define SK_SHA1     6
#define SK_UNAME    7
#define SK_GNAME    8
#define SK_INODE    9
#define SK_SHA256   10
#define SK_MTIME    11
#define SK_CHFIELDS 12
#define SK_NFIELDS  13

typedef struct __sdb {
    char buf[OS_MAXSTR + 1];
    char comment[OS_MAXSTR + 1];

    char size[OS_FLSIZE + 1];
    char perm[OS_FLSIZE + 1];
    char owner[OS_FLSIZE + 1];
    char gowner[OS_FLSIZE + 1];
    char md5[OS_FLSIZE + 1];
    char sha1[OS_FLSIZE + 1];
    char sha256[OS_FLSIZE + 1];
    char mtime[OS_FLSIZE + 1];
    char inode[OS_FLSIZE + 1];

    char agent_cp[MAX_AGENTS + 1][1];
    char *agent_ips[MAX_AGENTS + 1];
    FILE *agent_fps[MAX_AGENTS + 1];

    int db_err;

    /* Ids for decoder */
    int id1;
    int id2;
    int id3;
    int idn;
    int idd;

    /* Syscheck rule */
    OSDecoderInfo  *syscheck_dec;

    /* File search variables */
    fpos_t init_pos;

} _sdb; /* syscheck db information */

/* File sum structure */
typedef struct sk_sum_t {
    char *size;
    int perm;
    char *uid;
    char *gid;
    char *md5;
    char *sha1;
    char *sha256;
    char *uname;
    char *gname;
    long mtime;
    long inode;
} sk_sum_t;

extern _sdb sdb;

/* Parse c_sum string. Returns 0 if success, 1 when c_sum denotes a deleted file
   or -1 on failure. */
int sk_decode_sum(sk_sum_t *sum, char *c_sum);

void sk_fill_event(Eventinfo *lf, const char *f_name, const sk_sum_t *sum);

int sk_build_sum(const sk_sum_t * sum, char * output, size_t size);

/* Delete from path to parent all empty folders */
int remove_empty_folders(char *path);

/* Delete path file and all empty folders above */
int delete_target_file(const char *path);

#ifndef WIN32

    const char *get_user(__attribute__((unused)) const char *path, int uid);
const char* get_group(int gid);

#else

    const char *get_user(const char *path, __attribute__((unused)) int uid);
const char *get_group(__attribute__((unused)) int gid);

#endif

#endif
