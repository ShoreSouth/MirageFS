#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================
 * Common Flags
 * ============================================================
 */

typedef uint32_t fs_flags_t;

/*
 * ============================================================
 * Generic
 * ============================================================
 */

#define FS_FLAG_NONE           0U

/*
 * ============================================================
 * Namespace
 * ============================================================
 */

/* replace existing entry */
#define FS_FLAG_REPLACE        (1U << 0)

/* recursive operation */
#define FS_FLAG_RECURSIVE      (1U << 1)

/* do not follow symlink */
#define FS_FLAG_NOFOLLOW       (1U << 2)

/* create parent automatically */
#define FS_FLAG_PARENTS        (1U << 3)

/*
 * ============================================================
 * IO
 * ============================================================
 */

/* sync write */
#define FS_FLAG_SYNC           (1U << 4)

/* direct io */
#define FS_FLAG_DIRECT         (1U << 5)

/*
 * ============================================================
 * Helper
 * ============================================================
 */

static inline bool fs_flag_test(
                    fs_flags_t flags,
                    fs_flags_t flag)
{
    return (flags & flag) != 0U;
}