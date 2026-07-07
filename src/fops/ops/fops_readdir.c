#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

static bool fops_readdir_skip_name(const char *name)
{
    return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

static fs_error_t fops_readdir_resolve_entry(const fuid_t *dir_fuid,
                                             obj_meta_t *dir_meta,
                                             int dir_fd,
                                             const char *name,
                                             fs_type_t type_hint,
                                             fuid_t *out_fuid,
                                             fs_op_t sub)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    struct stat st;
    fs_type_t type;

    type = type_hint;
    if (type == FS_TYPE_UNKNOWN) {
        err = lsa_fstatat(dir_fd, name, FS_FLAG_NOFOLLOW, &st);
        if (fs_failed(err)) {
            return err;
        }
        type = fops_type_from_mode(st.st_mode);
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;
    err = lsa_name_to_handle_at(dir_fd,
                                name,
                                &lsa_handle,
                                &mount_id,
                                0);
    if (fs_failed(err)) {
        return err;
    }

    err = fops_handle_from_lsa_checked(&handle,
                                       &lsa_handle,
                                       mount_id,
                                       dir_meta,
                                       sub);
    if (fs_failed(err)) {
        return err;
    }

    return fops_fuid_from_handle(dir_fuid, &handle, type, out_fuid, sub);
}

static fs_error_t fops_readdir_common(const fuid_t *dir_fuid,
                                      fs_flags_t flags,
                                      fops_dirent_t *entries,
                                      fops_dirent_plus_t *plus_entries,
                                      uint32_t entry_cap,
                                      uint32_t *out_entry_nr,
                                      bool *out_eof,
                                      bool need_attr,
                                      fs_op_t sub)
{
    fs_error_t err;
    obj_meta_t *dir_meta;
    int dir_fd;
    lsa_dir_iter_t *iter;
    lsa_dirent_t entry;
    lsa_dirent_plus_t plus_entry;
    uint32_t copied;
    fops_dirent_t *dst_entry;

    dir_meta = NULL;
    dir_fd = -1;
    iter = NULL;
    copied = 0U;
    err = FS_OK;

    if (out_entry_nr != NULL) {
        *out_entry_nr = 0U;
    }
    if (out_eof != NULL) {
        *out_eof = false;
    }

    if ((dir_fuid == NULL) ||
        (entry_cap == 0U) ||
        (out_entry_nr == NULL) ||
        (out_eof == NULL) ||
        (need_attr ? (plus_entries == NULL) : (entries == NULL))) {
        err = fops_error(sub, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid readdir args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    if (!fuid_is_dir(dir_fuid)) {
        err = fops_error(sub, ENOTDIR);
        FS_LOG_DUMP_ERROR("readdir failed: object is not dir, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    err = fops_validate_readdir_flags(flags, sub);
    if (fs_failed(err)) {
        goto out;
    }

    err = fops_open_object(dir_fuid,
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &dir_meta,
                           &dir_fd,
                           sub);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_dir_iter_open(dir_fd, 0U, &iter);
    if (fs_failed(err)) {
        goto out;
    }

    while (copied < entry_cap) {
        if (need_attr) {
            memset(&plus_entry, 0, sizeof(plus_entry));
            err = lsa_dir_iter_next_plus(iter, &plus_entry);
            if (fs_failed(err)) {
                if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
                    *out_eof = true;
                    err = FS_OK;
                }
                break;
            }
            if (fops_readdir_skip_name(plus_entry.entry.name)) {
                continue;
            }
            dst_entry = &plus_entries[copied].entry;
            err = fops_readdir_resolve_entry(dir_fuid,
                                             dir_meta,
                                             dir_fd,
                                             plus_entry.entry.name,
                                             fops_type_from_mode(plus_entry.st.st_mode),
                                             &dst_entry->fuid,
                                             sub);
            if (fs_failed(err)) {
                break;
            }
            strncpy(dst_entry->name, plus_entry.entry.name, FS_MAX_NAME_LEN);
            dst_entry->name[FS_MAX_NAME_LEN] = '\0';
            fops_attr_from_stat(&plus_entries[copied].attr, &plus_entry.st);
        } else {
            memset(&entry, 0, sizeof(entry));
            err = lsa_dir_iter_next(iter, &entry);
            if (fs_failed(err)) {
                if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
                    *out_eof = true;
                    err = FS_OK;
                }
                break;
            }
            if (fops_readdir_skip_name(entry.name)) {
                continue;
            }
            dst_entry = &entries[copied];
            err = fops_readdir_resolve_entry(dir_fuid,
                                             dir_meta,
                                             dir_fd,
                                             entry.name,
                                             entry.type,
                                             &dst_entry->fuid,
                                             sub);
            if (fs_failed(err)) {
                break;
            }
            strncpy(dst_entry->name, entry.name, FS_MAX_NAME_LEN);
            dst_entry->name[FS_MAX_NAME_LEN] = '\0';
        }

        copied++;
    }

    *out_entry_nr = copied;

out:
    if (iter != NULL) {
        (void)lsa_dir_iter_close(iter);
    }
    fops_close_object(dir_meta, dir_fd);
    return err;
}

fs_error_t fops_readdir(const fuid_t *dir_fuid,
                        fs_flags_t flags,
                        fops_dirent_t *entries,
                        uint32_t entry_cap,
                        uint32_t *out_entry_nr,
                        bool *out_eof)
{
    return fops_readdir_common(dir_fuid,
                               flags,
                               entries,
                               NULL,
                               entry_cap,
                               out_entry_nr,
                               out_eof,
                               false,
                               FS_OP_READDIR);
}

fs_error_t fops_readdirplus(const fuid_t *dir_fuid,
                            fs_flags_t flags,
                            fops_dirent_plus_t *entries,
                            uint32_t entry_cap,
                            uint32_t *out_entry_nr,
                            bool *out_eof)
{
    return fops_readdir_common(dir_fuid,
                               flags,
                               NULL,
                               entries,
                               entry_cap,
                               out_entry_nr,
                               out_eof,
                               true,
                               FS_OP_READDIRPLUS);
}