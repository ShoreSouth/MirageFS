#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "common/fs_common.h"
#include "common/metrics/fs_metrics.h"
#include "fops/include/fops_types.h"
#include "lsa/include/lsa_api.h"
#include "object/objmeta/objmeta.h"

typedef struct fops_context
{
    uint32_t inited;
} fops_context_t;

struct fops_file
{
    obj_meta_t *meta;
    int fd;
    fuid_t fuid;
    fs_flags_t flags;
};

extern fops_context_t g_fops_ctx;
extern fs_metric_id_t g_fops_metric_ids[FS_OP_MAX];

typedef struct fops_op_spec
{
    fs_op_t op;               /* 操作字 */
    const char *name;         /* 操作名，用于日志/调试 */
    fs_flags_t allowed_flags; /* 允许出现的 FS_FLAG_* */
    fs_flags_t conflict_a;    /* 与 conflict_b 冲突的一组 flag */
    fs_flags_t conflict_b;    /* 与 conflict_a 冲突的一组 flag */
    bool allow_dot_name;      /* name 是否允许 "." / ".." */
    bool need_fuid;           /* args->fuid 必须有效 */
    bool need_parent;         /* args->parent_fuid 必须有效 */
    bool need_name;           /* args->name 必须非空且合法 */
} fops_op_spec_t;

const fops_op_spec_t *fops_op_spec_get(fs_op_t op);

fs_error_t fops_validate_flags(fs_op_t op, fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_args(const fops_args_t *args);

fs_error_t fops_validate_name(const char *name, fs_op_t sub,
                              bool allow_dot_names);

fs_error_t fops_validate_lookup_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_create_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_mkdir_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_getattr_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_readdir_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_unlink_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_rmdir_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_open_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_setattr_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_xattr_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_replace_flags(fs_flags_t flags, fs_op_t sub);

fs_error_t fops_validate_new_name_flags(fs_flags_t flags, fs_op_t sub);

int fops_linux_open_flags(fs_flags_t flags);

fs_error_t fops_file_check(const fops_file_t *file, fs_op_t sub);

fs_error_t fops_check_type_flags(fs_type_t type, fs_flags_t flags, fs_op_t sub);

mode_t fops_create_mode(const fops_create_attr_t *attr, mode_t default_mode);

fs_error_t fops_validate_create_attr(const fops_create_attr_t *attr,
                                     uint32_t supported_mask, fs_op_t sub);

fs_error_t fops_apply_create_attr(int fd, const fops_create_attr_t *attr,
                                  bool allow_size, fs_op_t sub);

fs_error_t fops_open_object(const fuid_t *fuid, int flags,
                            obj_meta_t **out_meta, int *out_fd, fs_op_t sub);

fs_error_t fops_open_parent_dir(const fuid_t *fuid, obj_meta_t **out_meta,
                                int *out_fd, fs_op_t sub);

void fops_close_object(obj_meta_t *meta, int fd);

fs_type_t fops_type_from_mode(mode_t mode);
fuid_type_t fops_fuid_type_from_fs_type(fs_type_t type);
void fops_attr_from_stat(fops_attr_t *attr, const struct stat *st);
void fops_attr_from_statx(fops_attr_t *attr, const struct statx *stx);

fuid_t fops_make_child_fuid(const fuid_t *parent_fuid, ObjectId_t objectid,
                            GenId_t gen, fs_type_t type);

fs_error_t fops_fuid_from_handle(const fuid_t *parent_fuid,
                                 const obj_handle_t *handle, fs_type_t type,
                                 fuid_t *out_fuid, fs_op_t sub);

fs_error_t fops_handle_from_lsa_checked(obj_handle_t *out,
                                        const lsa_file_handle_t *lsa_handle,
                                        int32_t mount_id,
                                        const obj_meta_t *parent_meta,
                                        fs_op_t sub);
