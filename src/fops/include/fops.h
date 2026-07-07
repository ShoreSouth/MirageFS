#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"

/*
 * Initialize FOPS module state and register FOPS sub-error names.
 *
 * Return:
 *      FS_OK       : success
 *      fs_error_t  : failure
 */
fs_error_t fops_init(void);

/*
 * Deinitialize FOPS module state.
 *
 * The caller must ensure no FOPS operation is still running.
 */
void fops_deinit(void);

/*
 * Lookup one name under a parent directory and return the child FUID.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : single path component; "." and ".." are allowed
 *      [IN]  flags       : FS_FLAG_NOFOLLOW / DIRECTORY / REGULAR
 *      [OUT] out_fuid    : resolved child FUID
 *
 * Return:
 *      FS_OK       : success
 *      fs_error_t  : invalid argument, type mismatch, or backend error
 */
fs_error_t fops_lookup(const fuid_t *parent_fuid,
                       const char *name,
                       fs_flags_t flags,
                       fuid_t *out_fuid);

/*
 * Lookup one name and return FUID plus attributes.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : single path component; "." and ".." are allowed
 *      [IN]  flags       : FS_FLAG_NOFOLLOW / DIRECTORY / REGULAR
 *      [OUT] out         : resolved FUID and attribute snapshot
 *
 * Return:
 *      FS_OK       : success
 *      fs_error_t  : invalid argument, type mismatch, or backend error
 */
fs_error_t fops_lookup_plus(const fuid_t *parent_fuid,
                            const char *name,
                            fs_flags_t flags,
                            fops_object_result_t *out);

/*
 * Create or reuse a regular file and return its FUID.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : new file name; "." and ".." are rejected
 *      [IN]  attr        : create-time attributes, NULL means defaults
 *      [IN]  flags       : REPLACE / EXCLUSIVE / TRUNCATE / NOFOLLOW /
 *                          SYNC / DIRECT / APPEND / REGULAR
 *      [OUT] out_fuid    : created or reused file FUID
 *
 * Return:
 *      FS_OK       : success
 *      fs_error_t  : invalid argument, conflict, type mismatch, or backend error
 */
fs_error_t fops_create(const fuid_t *parent_fuid,
                       const char *name,
                       const fops_create_attr_t *attr,
                       fs_flags_t flags,
                       fuid_t *out_fuid);

/*
 * Create or reuse a regular file and return FUID plus attributes.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : new file name; "." and ".." are rejected
 *      [IN]  attr        : create-time attributes, NULL means defaults
 *      [IN]  flags       : same as fops_create()
 *      [OUT] out         : created/reused FUID and post-create attributes
 */
fs_error_t fops_create_plus(const fuid_t *parent_fuid,
                            const char *name,
                            const fops_create_attr_t *attr,
                            fs_flags_t flags,
                            fops_object_result_t *out);

/*
 * Create a directory and return its FUID.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : new directory name; "." and ".." are rejected
 *      [IN]  attr        : create-time attributes, NULL means defaults;
 *                          SIZE is not supported for mkdir
 *      [IN]  flags       : FS_FLAG_EXCLUSIVE / DIRECTORY / NONE
 *      [OUT] out_fuid    : created directory FUID
 */
fs_error_t fops_mkdir(const fuid_t *parent_fuid,
                      const char *name,
                      const fops_create_attr_t *attr,
                      fs_flags_t flags,
                      fuid_t *out_fuid);

/*
 * Create a directory and return FUID plus attributes.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : new directory name; "." and ".." are rejected
 *      [IN]  attr        : create-time attributes, NULL means defaults
 *      [IN]  flags       : same as fops_mkdir()
 *      [OUT] out         : created directory FUID and post-create attributes
 */
fs_error_t fops_mkdir_plus(const fuid_t *parent_fuid,
                           const char *name,
                           const fops_create_attr_t *attr,
                           fs_flags_t flags,
                           fops_object_result_t *out);

/*
 * Read attributes for an existing FUID.
 *
 * Parameters:
 *      [IN]  fuid     : object FUID
 *      [IN]  flags    : FS_FLAG_DIRECTORY / REGULAR / NONE
 *      [OUT] out_attr : attribute snapshot
 */
fs_error_t fops_getattr(const fuid_t *fuid,
                        fs_flags_t flags,
                        fops_attr_t *out_attr);

/*
 * Read directory entries without attributes.
 *
 * Parameters:
 *      [IN]  dir_fuid     : directory FUID
 *      [IN]  flags        : FS_FLAG_DIRECTORY / NONE
 *      [OUT] entries      : caller-owned entry array
 *      [IN]  entry_cap    : number of elements in entries
 *      [OUT] out_entry_nr : number of entries written
 *      [OUT] out_eof      : true when iterator reached backend EOF
 */
fs_error_t fops_readdir(const fuid_t *dir_fuid,
                        fs_flags_t flags,
                        fops_dirent_t *entries,
                        uint32_t entry_cap,
                        uint32_t *out_entry_nr,
                        bool *out_eof);

/*
 * Read directory entries with attributes.
 *
 * Parameters:
 *      [IN]  dir_fuid     : directory FUID
 *      [IN]  flags        : FS_FLAG_DIRECTORY / NONE
 *      [OUT] entries      : caller-owned entry+attribute array
 *      [IN]  entry_cap    : number of elements in entries
 *      [OUT] out_entry_nr : number of entries written
 *      [OUT] out_eof      : true when iterator reached backend EOF
 */
fs_error_t fops_readdirplus(const fuid_t *dir_fuid,
                            fs_flags_t flags,
                            fops_dirent_plus_t *entries,
                            uint32_t entry_cap,
                            uint32_t *out_entry_nr,
                            bool *out_eof);

/*
 * Remove a non-directory child.
 *
 * Parameters:
 *      [IN] parent_fuid : parent directory FUID
 *      [IN] name        : child name; "." and ".." are rejected
 *      [IN] flags       : FS_FLAG_NOFOLLOW / REGULAR / NONE
 */
fs_error_t fops_unlink(const fuid_t *parent_fuid,
                       const char *name,
                       fs_flags_t flags);

/*
 * Remove an empty directory child.
 *
 * Parameters:
 *      [IN] parent_fuid : parent directory FUID
 *      [IN] name        : child name; "." and ".." are rejected
 *      [IN] flags       : FS_FLAG_DIRECTORY / NONE
 */
fs_error_t fops_rmdir(const fuid_t *parent_fuid,
                      const char *name,
                      fs_flags_t flags);