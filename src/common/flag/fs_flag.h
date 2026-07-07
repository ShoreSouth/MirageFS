#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================
 * Common Flags
 * ============================================================
 */

typedef uint32_t fs_flags_t;

/* no extra operation constraint */
#define FS_FLAG_NONE           0U

/*
 * Creation constraints.
 *
 * REPLACE means an existing regular file may be reused.
 * EXCLUSIVE means the target must not exist.
 * These two flags conflict with each other.
 */
#define FS_FLAG_REPLACE        (1U << 0)
#define FS_FLAG_EXCLUSIVE      (1U << 1)

/* do not follow a symlink at the final path component */
#define FS_FLAG_NOFOLLOW       (1U << 2)

/* write/open synchronization and direct-io hints */
#define FS_FLAG_SYNC           (1U << 3)
#define FS_FLAG_DIRECT         (1U << 4)

/* result type constraints for lookup/open/delete style operations */
#define FS_FLAG_DIRECTORY      (1U << 5)
#define FS_FLAG_REGULAR        (1U << 6)

/* creation/open modifiers */
#define FS_FLAG_TRUNCATE       (1U << 7)
#define FS_FLAG_APPEND         (1U << 8)

/* open/access direction flags */
#define FS_FLAG_READ           (1U << 9)
#define FS_FLAG_WRITE          (1U << 10)

static inline bool fs_flag_test(
                    fs_flags_t flags,
                    fs_flags_t flag)
{
    return (flags & flag) != 0U;
}
