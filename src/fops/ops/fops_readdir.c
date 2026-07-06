#include "fops/include/fops.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "fops/internal/fops_error.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"

fs_error_t fops_readdirplus(const fuid_t *dir_fuid,
                            fops_dirent_t *entries,
                            uint32_t entry_cap,
                            uint32_t *out_entry_nr,
                            bool *out_eof)
{
    fs_error_t err;
    obj_meta_t *dir_meta;
    int dir_fd;
    lsa_dir_iter_t *iter;
    lsa_dirent_plus_t lsa_entry;
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    int32_t mount_id;
    uint32_t copied;
    fs_type_t type;

    dir_meta = NULL;
    dir_fd = -1;
    iter = NULL;
    copied = 0U;

    if (out_entry_nr != NULL) {
        *out_entry_nr = 0U;
    }
    if (out_eof != NULL) {
        *out_eof = false;
    }

    if ((dir_fuid == NULL) || (entries == NULL) ||
        (entry_cap == 0U) || (out_entry_nr == NULL) ||
        (out_eof == NULL)) {
        err = fops_error(FS_OP_READDIRPLUS, EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: invalid readdirplus args, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    if (!fuid_is_dir(dir_fuid)) {
        err = fops_error(FS_OP_READDIRPLUS, ENOTDIR);
        FS_LOG_DUMP_ERROR("readdirplus failed: object is not dir, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        goto out;
    }

    err = fops_open_object(dir_fuid,
                           O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                           &dir_meta,
                           &dir_fd,
                           FS_OP_READDIRPLUS);
    if (fs_failed(err)) {
        goto out;
    }

    err = lsa_dir_iter_open(dir_fd, 0U, &iter);
    if (fs_failed(err)) {
        goto out;
    }

    while (copied < entry_cap) {
        memset(&lsa_entry, 0, sizeof(lsa_entry));
        err = lsa_dir_iter_next_plus(iter, &lsa_entry);
        if (fs_failed(err)) {
            if (fs_err_errno(err) == FS_ERRNO_ENOENT) {
                *out_eof = true;
                err = FS_OK;
            }
            break;
        }

        if ((strcmp(lsa_entry.entry.name, ".") == 0) ||
            (strcmp(lsa_entry.entry.name, "..") == 0)) {
            continue;
        }

        memset(&lsa_handle, 0, sizeof(lsa_handle));
        mount_id = 0;
        err = lsa_name_to_handle_at(dir_fd,
                                    lsa_entry.entry.name,
                                    &lsa_handle,
                                    &mount_id,
                                    0);
        if (fs_failed(err)) {
            break;
        }

        err = fops_handle_from_lsa_checked(&handle, &lsa_handle, 
            mount_id, dir_meta, FS_OP_READDIRPLUS);
        if (fs_failed(err)) {
            break;
        }

        type = fops_type_from_mode(lsa_entry.st.st_mode);
        err = fops_fuid_from_handle(dir_fuid, &handle, type,
                                    &entries[copied].fuid,
                                    FS_OP_READDIRPLUS);
        if (fs_failed(err)) {
            break;
        }

        strncpy(entries[copied].name, lsa_entry.entry.name, 
                FS_MAX_NAME_LEN);
        entries[copied].name[FS_MAX_NAME_LEN] = '\0';
        fops_attr_from_stat(&entries[copied].attr, &lsa_entry.st);
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
