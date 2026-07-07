#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/statfs.h>

#include "common/fs_common.h"
#include "object/fuid/fuid.h"

typedef struct fops_file fops_file_t;

#define FOPS_READDIR_MAX_BATCH 1024U

#define FOPS_CREATE_ATTR_MODE  (1U << 0)
#define FOPS_CREATE_ATTR_UID   (1U << 1)
#define FOPS_CREATE_ATTR_GID   (1U << 2)
#define FOPS_CREATE_ATTR_SIZE  (1U << 3)

/*
 * fops_attr_t
 *
 * Stable attribute snapshot returned by FOPS. This is an output value,
 * not a create/setattr request. Fields such as nlink and ctime are owned
 * by the backend filesystem and must not be supplied by callers.
 */
typedef struct fops_attr {

    fs_type_t type;      /* object type */
    mode_t mode;         /* full Linux mode bits */
    uid_t uid;           /* owner uid */
    gid_t gid;           /* owner gid */

    uint64_t size;       /* byte size */
    uint64_t nlink;      /* link count */

    uint64_t atime_sec;  /* access time, seconds */
    uint64_t mtime_sec;  /* modify time, seconds */
    uint64_t ctime_sec;  /* change time, seconds */

} fops_attr_t;

/*
 * fops_create_attr_t
 *
 * Create-time attribute request. valid_mask controls which fields are
 * applied. Unsupported bits are rejected by the target operation.
 */
typedef struct fops_create_attr {

    uint32_t valid_mask; /* FOPS_CREATE_ATTR_* */

    mode_t mode;         /* valid when FOPS_CREATE_ATTR_MODE is set */
    uid_t uid;           /* valid when FOPS_CREATE_ATTR_UID is set */
    gid_t gid;           /* valid when FOPS_CREATE_ATTR_GID is set */
    uint64_t size;       /* valid for regular file create only */

} fops_create_attr_t;

#define FOPS_SETATTR_MODE  FOPS_CREATE_ATTR_MODE
#define FOPS_SETATTR_UID   FOPS_CREATE_ATTR_UID
#define FOPS_SETATTR_GID   FOPS_CREATE_ATTR_GID
#define FOPS_SETATTR_SIZE  FOPS_CREATE_ATTR_SIZE

/* Attribute update request. valid_mask controls which fields are applied. */
typedef struct fops_setattr {

    uint32_t valid_mask; /* FOPS_SETATTR_* */

    mode_t mode;         /* permission bits when FOPS_SETATTR_MODE is set */
    uid_t uid;           /* owner uid when FOPS_SETATTR_UID is set */
    gid_t gid;           /* owner gid when FOPS_SETATTR_GID is set */
    uint64_t size;       /* regular-file size when FOPS_SETATTR_SIZE is set */

} fops_setattr_t;

/* Device identifier for mknod. Valid for block/character devices. */
typedef struct fops_device {

    uint32_t major_id;
    uint32_t minor_id;

} fops_device_t;

typedef struct statfs fops_statfs_t;

/* Directory entry without attributes. */
typedef struct fops_dirent {

    char name[FS_MAX_NAME_LEN + 1U];
    fuid_t fuid;

} fops_dirent_t;

/* Directory entry with attributes. */
typedef struct fops_dirent_plus {

    fops_dirent_t entry;
    fops_attr_t attr;

} fops_dirent_plus_t;

/* Common result for lookup/create-style plus operations. */
typedef struct fops_object_result {

    fuid_t fuid;
    fops_attr_t attr;

} fops_object_result_t;
