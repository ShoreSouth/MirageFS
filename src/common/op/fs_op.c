#include "common/op/fs_op.h"

static const char *g_fs_op_names[] = {

#define FS_OP_NAME(name, str) [FS_OP_##name] = str,

        FS_OP_TABLE(FS_OP_NAME)

#undef FS_OP_NAME

};

const char *fs_op_name(fs_op_t op)
{
    if (!fs_op_valid(op))
    {
        return "UNKNOWN";
    }

    return g_fs_op_names[op];
}

bool fs_op_valid(uint32_t op)
{
    return op < FS_OP_MAX;
}
