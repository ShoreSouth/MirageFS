#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "fops/include/fops_types.h"
#include "object/objmeta/objmeta.h"

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

/*
 * Rename one child between parent directories.
 *
 * Parameters:
 *      [IN] old_parent_fuid : source parent directory FUID
 *      [IN] old_name        : source child name
 *      [IN] new_parent_fuid : destination parent directory FUID
 *      [IN] new_name        : destination child name
 *      [IN] flags           : FS_FLAG_REPLACE / EXCLUSIVE / NONE
 */
fs_error_t fops_rename(const fuid_t *old_parent_fuid,
                       const char *old_name,
                       const fuid_t *new_parent_fuid,
                       const char *new_name,
                       fs_flags_t flags);

/*
 * Create a hard link and return the linked object's FUID.
 *
 * Parameters:
 *      [IN]  old_parent_fuid : source parent directory FUID
 *      [IN]  old_name        : existing regular-file name
 *      [IN]  new_parent_fuid : destination parent directory FUID
 *      [IN]  new_name        : new link name; must not exist
 *      [IN]  flags           : FS_FLAG_EXCLUSIVE / NONE
 *      [OUT] out_fuid        : linked object FUID
 */
fs_error_t fops_link(const fuid_t *old_parent_fuid,
                     const char *old_name,
                     const fuid_t *new_parent_fuid,
                     const char *new_name,
                     fs_flags_t flags,
                     fuid_t *out_fuid);

/* Same as fops_link(), but also returns post-link attributes. */
fs_error_t fops_link_plus(const fuid_t *old_parent_fuid,
                          const char *old_name,
                          const fuid_t *new_parent_fuid,
                          const char *new_name,
                          fs_flags_t flags,
                          fops_object_result_t *out);

/*
 * Create a symbolic link and return its FUID.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : symlink name; must not exist
 *      [IN]  target      : symlink payload
 *      [IN]  flags       : FS_FLAG_EXCLUSIVE / NONE
 *      [OUT] out_fuid    : symlink FUID
 */
fs_error_t fops_symlink(const fuid_t *parent_fuid,
                        const char *name,
                        const char *target,
                        fs_flags_t flags,
                        fuid_t *out_fuid);

/* Same as fops_symlink(), but also returns symlink attributes. */
fs_error_t fops_symlink_plus(const fuid_t *parent_fuid,
                             const char *name,
                             const char *target,
                             fs_flags_t flags,
                             fops_object_result_t *out);

/*
 * Create a special object and return its FUID.
 *
 * Parameters:
 *      [IN]  parent_fuid : parent directory FUID
 *      [IN]  name        : new object name; must not exist
 *      [IN]  type        : FS_TYPE_FIFO / BLK / CHR
 *      [IN]  attr        : create-time attributes; currently MODE is supported
 *      [IN]  device      : required for BLK/CHR, ignored for FIFO
 *      [IN]  flags       : FS_FLAG_EXCLUSIVE / NONE
 *      [OUT] out_fuid    : created object FUID
 */
fs_error_t fops_mknod(const fuid_t *parent_fuid,
                      const char *name,
                      fs_type_t type,
                      const fops_create_attr_t *attr,
                      const fops_device_t *device,
                      fs_flags_t flags,
                      fuid_t *out_fuid);

/* Same as fops_mknod(), but also returns post-create attributes. */
fs_error_t fops_mknod_plus(const fuid_t *parent_fuid,
                           const char *name,
                           fs_type_t type,
                           const fops_create_attr_t *attr,
                           const fops_device_t *device,
                           fs_flags_t flags,
                           fops_object_result_t *out);

/*
 * Update object attributes.
 *
 * Parameters:
 *      [IN] fuid  : target object FUID
 *      [IN] attr  : attribute update request
 *      [IN] flags : FS_FLAG_DIRECTORY / REGULAR / NONE type constraint
 */
fs_error_t fops_setattr(const fuid_t *fuid,
                        const fops_setattr_t *attr,
                        fs_flags_t flags);

/*
 * Check access for an object using R_OK/W_OK/X_OK/F_OK style mask.
 *
 * Parameters:
 *      [IN] fuid  : target object FUID
 *      [IN] mask  : POSIX access mask
 *      [IN] flags : FS_FLAG_DIRECTORY / REGULAR / NONE type constraint
 */
fs_error_t fops_access(const fuid_t *fuid,
                       int mask,
                       fs_flags_t flags);

/*
 * Copy the backend object handle registered for a FUID.
 *
 * Parameters:
 *      [IN]  fuid       : target object FUID
 *      [OUT] out_handle : backend handle copy
 */
fs_error_t fops_gethandle(const fuid_t *fuid,
                          obj_handle_t *out_handle);

/*
 * Open an object and return an opaque FOPS file handle.
 *
 * Parameters:
 *      [IN]  fuid     : target object FUID
 *      [IN]  flags    : READ / WRITE / APPEND / TRUNCATE / SYNC / DIRECT /
 *                       DIRECTORY / REGULAR
 *      [OUT] out_file : opened FOPS file handle; close with fops_close()
 */
fs_error_t fops_open(const fuid_t *fuid,
                     fs_flags_t flags,
                     fops_file_t **out_file);

/*
 * Open an already registered backend handle and return a FOPS file handle.
 *
 * Parameters:
 *      [IN]  handle   : backend handle previously registered in ObjMgr
 *      [IN]  flags    : same as fops_open()
 *      [OUT] out_file : opened FOPS file handle
 */
fs_error_t fops_openhandle(const obj_handle_t *handle,
                           fs_flags_t flags,
                           fops_file_t **out_file);

/*
 * Close a FOPS file handle returned by fops_open/openhandle.
 *
 * Parameters:
 *      [IN/OUT] file : FOPS file handle; invalid after this call
 */
fs_error_t fops_close(fops_file_t *file);

fs_error_t fops_read(fops_file_t *file,
                     void *buf,
                     size_t size,
                     size_t *actual);

fs_error_t fops_write(fops_file_t *file,
                      const void *buf,
                      size_t size,
                      size_t *actual);

fs_error_t fops_pread(fops_file_t *file,
                      void *buf,
                      size_t size,
                      off_t offset,
                      size_t *actual);

fs_error_t fops_pwrite(fops_file_t *file,
                       const void *buf,
                       size_t size,
                       off_t offset,
                       size_t *actual);

fs_error_t fops_truncate(const fuid_t *fuid,
                         uint64_t size,
                         fs_flags_t flags);

fs_error_t fops_getxattr(const fuid_t *fuid,
                         const char *name,
                         void *value,
                         size_t size,
                         size_t *actual);

fs_error_t fops_setxattr(const fuid_t *fuid,
                         const char *name,
                         const void *value,
                         size_t size,
                         fs_flags_t flags);

fs_error_t fops_listxattr(const fuid_t *fuid,
                          char *list,
                          size_t size,
                          size_t *actual);

fs_error_t fops_removexattr(const fuid_t *fuid,
                            const char *name);

fs_error_t fops_statfs(const fuid_t *fuid,
                       fops_statfs_t *out_statfs);

fs_error_t fops_syncfs(const fuid_t *fuid);
