#pragma once

#include <errno.h>

#include <sys/stat.h>

#include "lsa/include/lsa_api.h"
#include "lsa/internal/lsa_common.h"

/*
 * ============================================================
 * Type Helper
 * ============================================================
 */

static inline fs_type_t lsa_type_from_mode(mode_t mode)
{
    if (S_ISREG(mode)) {
        return FS_TYPE_REG;
    }

    if (S_ISDIR(mode)) {
        return FS_TYPE_DIR;
    }

    if (S_ISLNK(mode)) {
        return FS_TYPE_LNK;
    }

    if (S_ISFIFO(mode)) {
        return FS_TYPE_FIFO;
    }

    if (S_ISSOCK(mode)) {
        return FS_TYPE_SOCK;
    }

    if (S_ISBLK(mode)) {
        return FS_TYPE_BLK;
    }

    if (S_ISCHR(mode)) {
        return FS_TYPE_CHR;
    }

    return FS_TYPE_UNKNOWN;
}
