#include "common/error/fs_errno.h"
#include "common/error/fs_errno_table.h"

const char *fs_errno_name(fs_errno_t err)
{
    switch (err)
    {
#define FS_ERRNO_CASE(name, desc)                                              \
    case name:                                                                 \
        return #name;

        FS_ERRNO_TABLE(FS_ERRNO_CASE)

#undef FS_ERRNO_CASE

    default:
        return "UNKNOWN";
    }
}

const char *fs_errno_desc(fs_errno_t err)
{
    switch (err)
    {
#define FS_ERRNO_CASE(name, desc)                                              \
    case name:                                                                 \
        return desc;

        FS_ERRNO_TABLE(FS_ERRNO_CASE)

#undef FS_ERRNO_CASE

    default:
        return "Unknown errno";
    }
}

bool fs_errno_valid(int err)
{
    switch (err)
    {
#define FS_ERRNO_CASE(name, desc)                                              \
    case name:                                                                 \
        return true;

        FS_ERRNO_TABLE(FS_ERRNO_CASE)

#undef FS_ERRNO_CASE

    default:
        return false;
    }
}
