#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "lsa/include/lsa_api.h"
#include "object/objmeta/objmeta.h"

#define FOPS_OBJECT_GEN_DEFAULT ((GenId_t)1U)

typedef struct fops_context {
    uint32_t inited;
} fops_context_t;

extern fops_context_t g_fops_ctx;

fs_error_t fops_validate_name(const char *name, fs_op_t sub);

fs_error_t fops_open_object(const fuid_t *fuid,
                            int flags,
                            obj_meta_t **out_meta,
                            int *out_fd,
                            fs_op_t sub);

void fops_close_object(obj_meta_t *meta, int fd);

fs_type_t fops_type_from_mode(mode_t mode);
fuid_type_t fops_fuid_type_from_fs_type(fs_type_t type);
void fops_attr_from_stat(fops_attr_t *attr, const struct stat *st);

fuid_t fops_make_child_fuid(const fuid_t *parent_fuid,
                            ObjectId_t objectid,
                            GenId_t gen,
                            fs_type_t type);

fs_error_t fops_fuid_from_handle(const fuid_t *parent_fuid,
                                 const obj_handle_t *handle,
                                 fs_type_t type,
                                 fuid_t *out_fuid,
                                 fs_op_t sub);

fs_error_t fops_handle_from_lsa_checked(obj_handle_t *out,
                                        const lsa_file_handle_t *lsa_handle,
                                        int32_t mount_id,
                                        const obj_meta_t *parent_meta,
                                        fs_op_t sub);
