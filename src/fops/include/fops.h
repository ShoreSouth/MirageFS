#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"

fs_error_t fops_init(void);
void fops_deinit(void);

fs_error_t fops_lookup(const fuid_t *parent_fuid,
                       const char *name,
                       fuid_t *out_fuid);

fs_error_t fops_create(const fuid_t *parent_fuid,
                       const char *name,
                       mode_t mode,
                       fs_flags_t flags,
                       fuid_t *out_fuid);

fs_error_t fops_mkdir(const fuid_t *parent_fuid,
                      const char *name,
                      mode_t mode,
                      fuid_t *out_fuid);

fs_error_t fops_getattr(const fuid_t *fuid,
                        fops_attr_t *out_attr);

fs_error_t fops_readdirplus(const fuid_t *dir_fuid,
                            fops_dirent_t *entries,
                            uint32_t entry_cap,
                            uint32_t *out_entry_nr,
                            bool *out_eof);

fs_error_t fops_unlink(const fuid_t *parent_fuid,
                       const char *name);

fs_error_t fops_rmdir(const fuid_t *parent_fuid,
                      const char *name);
