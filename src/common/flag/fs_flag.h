#pragma once

#include <stdbool.h>
#include <stdint.h>

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
 * Constraint
 * ============================================================
 */

/* replace existing object */
#define FS_FLAG_REPLACE        (1U << 0)

/* object must not exist */
#define FS_FLAG_EXCLUSIVE      (1U << 1)

/* do not follow symlink */
#define FS_FLAG_NOFOLLOW       (1U << 2)

/*
 * ============================================================
 * IO
 * ============================================================
 */

/* sync write */
#define FS_FLAG_SYNC           (1U << 3)

/* direct io */
#define FS_FLAG_DIRECT         (1U << 4)

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