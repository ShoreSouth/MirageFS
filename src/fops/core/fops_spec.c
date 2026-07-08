#include "fops/internal/fops_internal.h"

#include <errno.h>
#include <string.h>

#include "fops/internal/fops_error.h"

#define FOPS_FLAG_TYPE_MASK (FS_FLAG_DIRECTORY | FS_FLAG_REGULAR)
#define FOPS_FLAG_REPLACE_MASK (FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE)

static const fops_op_spec_t g_fops_op_specs[] = {
    { FS_OP_LOOKUP, "lookup",
      FS_FLAG_NOFOLLOW | FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, true, false, true, true },
    { FS_OP_CREATE, "create",
      FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE | FS_FLAG_NOFOLLOW |
      FS_FLAG_SYNC | FS_FLAG_DIRECT | FS_FLAG_REGULAR |
      FS_FLAG_TRUNCATE | FS_FLAG_APPEND,
      FS_FLAG_REPLACE, FS_FLAG_EXCLUSIVE, false, false, true, true },
    { FS_OP_MKDIR, "mkdir",
      FS_FLAG_EXCLUSIVE | FS_FLAG_DIRECTORY,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_MKNOD, "mknod",
      FS_FLAG_EXCLUSIVE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_UNLINK, "unlink",
      FS_FLAG_NOFOLLOW | FS_FLAG_REGULAR,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_RMDIR, "rmdir",
      FS_FLAG_DIRECTORY,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_RENAME, "rename",
      FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
      FS_FLAG_REPLACE, FS_FLAG_EXCLUSIVE, false, false, true, true },
    { FS_OP_LINK, "link",
      FS_FLAG_EXCLUSIVE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_SYMLINK, "symlink",
      FS_FLAG_EXCLUSIVE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, true, true },
    { FS_OP_OPEN, "open",
      FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_SYNC | FS_FLAG_DIRECT |
      FS_FLAG_APPEND | FS_FLAG_TRUNCATE | FS_FLAG_DIRECTORY |
      FS_FLAG_REGULAR,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, true, false, false },
    { FS_OP_CLOSE, "close",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, false, false },
    { FS_OP_GETHANDLE, "gethandle",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
    { FS_OP_OPENHANDLE, "openhandle",
      FS_FLAG_READ | FS_FLAG_WRITE | FS_FLAG_SYNC | FS_FLAG_DIRECT |
      FS_FLAG_APPEND | FS_FLAG_TRUNCATE | FS_FLAG_DIRECTORY |
      FS_FLAG_REGULAR,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, false, false, false },
    { FS_OP_READDIR, "readdir",
      FS_FLAG_DIRECTORY,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
    { FS_OP_READDIRPLUS, "readdirplus",
      FS_FLAG_DIRECTORY,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
    { FS_OP_GETATTR, "getattr",
      FOPS_FLAG_TYPE_MASK,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, true, false, false },
    { FS_OP_SETATTR, "setattr",
      FOPS_FLAG_TYPE_MASK,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, true, false, false },
    { FS_OP_ACCESS, "access",
      FOPS_FLAG_TYPE_MASK,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, true, false, false },
    { FS_OP_READ, "read",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, false, false },
    { FS_OP_WRITE, "write",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, false, false, false },
    { FS_OP_TRUNCATE, "truncate",
      FOPS_FLAG_TYPE_MASK,
      FS_FLAG_DIRECTORY, FS_FLAG_REGULAR, false, true, false, false },
    { FS_OP_GETXATTR, "getxattr",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, true },
    { FS_OP_SETXATTR, "setxattr",
      FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
      FS_FLAG_REPLACE, FS_FLAG_EXCLUSIVE, false, true, false, true },
    { FS_OP_LISTXATTR, "listxattr",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
    { FS_OP_REMOVEXATTR, "removexattr",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, true },
    { FS_OP_STATFS, "statfs",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
    { FS_OP_SYNCFS, "syncfs",
      FS_FLAG_NONE,
      FS_FLAG_NONE, FS_FLAG_NONE, false, true, false, false },
};

const fops_op_spec_t *fops_op_spec_get(fs_op_t op)
{
    size_t i;

    for (i = 0U; i < (sizeof(g_fops_op_specs) / sizeof(g_fops_op_specs[0])); i++) {
        if (g_fops_op_specs[i].op == op) {
            return &g_fops_op_specs[i];
        }
    }

    return NULL;
}

fs_error_t fops_validate_flags(fs_op_t op,
                               fs_flags_t flags,
                               fs_op_t sub)
{
    const fops_op_spec_t *spec;
    fs_error_t err;

    spec = fops_op_spec_get(op);
    if (spec == NULL) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS flag check failed: unsupported op=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)op, fs_error_str(err), err);
        return err;
    }

    if ((flags & ~spec->allowed_flags) != 0U) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS flag check failed: op=%s flags=0x%x "
                          "allowed=0x%x, err=%s (0x%x)",
                          spec->name, flags, spec->allowed_flags,
                          fs_error_str(err), err);
        return err;
    }

    if ((spec->conflict_a != FS_FLAG_NONE) &&
        spec->conflict_b != FS_FLAG_NONE &&
        ((flags & spec->conflict_a) != 0U) &&
        ((flags & spec->conflict_b) != 0U)) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS flag check failed: op=%s conflict flags, "
                          "err=%s (0x%x)",
                          spec->name, fs_error_str(err), err);
        return err;
    }

    return FS_OK;
}

fs_error_t fops_validate_args(const fops_args_t *args)
{
    const fops_op_spec_t *spec;
    fs_error_t err;

    if (args == NULL) {
        err = fops_error(FS_OP_NONE, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS args check failed: args is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }

    spec = fops_op_spec_get(args->op);
    if (spec == NULL) {
        err = fops_error(args->op, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS args check failed: unsupported op=%u, "
                          "err=%s (0x%x)",
                          (unsigned int)args->op, fs_error_str(err), err);
        return err;
    }

    err = fops_validate_flags(args->op, args->flags, args->op);
    if (fs_failed(err)) {
        return err;
    }

    if (spec->need_fuid && ((args->fuid == NULL) || !fuid_is_valid(args->fuid))) {
        err = fops_error(args->op, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS args check failed: invalid fuid, op=%s, "
                          "err=%s (0x%x)",
                          spec->name, fs_error_str(err), err);
        return err;
    }

    if (spec->need_parent &&
        ((args->parent_fuid == NULL) || !fuid_is_valid(args->parent_fuid))) {
        err = fops_error(args->op, EINVAL);
        FS_LOG_DUMP_ERROR("FOPS args check failed: invalid parent, op=%s, "
                          "err=%s (0x%x)",
                          spec->name, fs_error_str(err), err);
        return err;
    }

    if (spec->need_name) {
        err = fops_validate_name(args->name, args->op, spec->allow_dot_name);
        if (fs_failed(err)) {
            return err;
        }
    }

    return FS_OK;
}
