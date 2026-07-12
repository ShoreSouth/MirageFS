#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "namei/include/namei_types.h"
#include "object/fuid/fuid.h"

fs_error_t namei_init(void);
void namei_deinit(void);

void namei_ctx_make(namei_ctx_t *ctx,
                    const fuid_t *root_fuid,
                    const fuid_t *cwd_fuid);

fs_error_t namei_lookup(const namei_ctx_t *ctx,
                        const char *path,
                        fs_flags_t flags,
                        fuid_t *out_fuid);

fs_error_t namei_lookup_plus(const namei_ctx_t *ctx,
                             const char *path,
                             fs_flags_t flags,
                             fops_object_result_t *out);

fs_error_t namei_lookup_parent(const namei_ctx_t *ctx,
                               const char *path,
                               fs_flags_t flags,
                               namei_parent_result_t *out);

fs_error_t namei_create(const namei_ctx_t *ctx,
                        const char *path,
                        const fops_create_attr_t *attr,
                        fs_flags_t flags,
                        fops_object_result_t *out);

fs_error_t namei_mkdir(const namei_ctx_t *ctx,
                       const char *path,
                       const fops_create_attr_t *attr,
                       fs_flags_t flags,
                       fops_object_result_t *out);

fs_error_t namei_symlink(const namei_ctx_t *ctx,
                         const char *target,
                         const char *linkpath,
                         fs_flags_t flags,
                         fops_object_result_t *out);

fs_error_t namei_readlink(const namei_ctx_t *ctx,
                          const char *path,
                          fs_flags_t flags,
                          char *buf,
                          size_t size,
                          size_t *actual);

fs_error_t namei_readdir(const namei_ctx_t *ctx,
                         const char *path,
                         fs_flags_t flags,
                         const namei_readdir_args_t *args);

fs_error_t namei_readdirplus(const namei_ctx_t *ctx,
                             const char *path,
                             fs_flags_t flags,
                             const namei_readdirplus_args_t *args);

fs_error_t namei_unlink(const namei_ctx_t *ctx,
                        const char *path,
                        fs_flags_t flags);

fs_error_t namei_rmdir(const namei_ctx_t *ctx,
                       const char *path,
                       fs_flags_t flags);

fs_error_t namei_rename(const namei_ctx_t *ctx,
                        const char *old_path,
                        const char *new_path,
                        fs_flags_t flags);

fs_error_t namei_getattr(const namei_ctx_t *ctx,
                         const char *path,
                         fs_flags_t flags,
                         fops_attr_t *out_attr);

fs_error_t namei_open(const namei_ctx_t *ctx,
                      const char *path,
                      fs_flags_t flags,
                      fops_file_t **out_file);
