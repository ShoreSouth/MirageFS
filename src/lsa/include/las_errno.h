#pragma once

#include "common/error/fs_error.h"

/* ============================================================
 * LSA errno
 * ============================================================ */

typedef enum {

    FS_LSA_ERR_NONE = 0,

    /* lookup */

    FS_LSA_ERR_NOT_FOUND,
    FS_LSA_ERR_ALREADY_EXIST,

    /* path */

    FS_LSA_ERR_INVALID_NAME,
    FS_LSA_ERR_INVALID_PARENT,

    /* object */

    FS_LSA_ERR_OBJ_CREATE,
    FS_LSA_ERR_OBJ_DELETE,

    /* dirent */

    FS_LSA_ERR_DIRENT_CREATE,
    FS_LSA_ERR_DIRENT_DELETE,

    /* rename */

    FS_LSA_ERR_RENAME_CONFLICT,

    /* internal */

    FS_LSA_ERR_INTERNAL,

} fs_lsa_errno_t;