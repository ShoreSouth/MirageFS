#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "object/fuid/fuid.h"

#define FOPS_READDIR_MAX_BATCH 1024U

typedef struct fops_attr {

    fs_type_t type;
    mode_t mode;
    uid_t uid;
    gid_t gid;

    uint64_t size;
    uint64_t nlink;

    uint64_t atime_sec;
    uint64_t mtime_sec;
    uint64_t ctime_sec;

} fops_attr_t;

typedef struct fops_dirent {

    char name[FS_MAX_NAME_LEN + 1U];
    fuid_t fuid;
    fops_attr_t attr;

} fops_dirent_t;
