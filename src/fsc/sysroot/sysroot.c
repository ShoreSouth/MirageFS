#include <fsc/sysroot/sysroot.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <fsc/fsc_error.h>
#include <lsa/include/lsa_api.h>

typedef struct fsc_sysroot
{
    fs_mutex_t lock;
    bool lock_inited;
    fuid_t root_fuid;
    obj_handle_t root_handle;
    char path[FSC_SYSROOT_PATH_MAX];
    uint32_t state;
} fsc_sysroot_t;

static fsc_sysroot_t g_sysroot;
static const char g_default_path[] = "./miragefs.root";

static fs_error_t fsc_sysroot_make_abs_path(const char *path, char *out,
                                            size_t out_size);

static fs_error_t fsc_sysroot_handle_from_lsa(obj_handle_t *out,
                                              const lsa_file_handle_t *handle,
                                              int32_t mount_id);

/* 将用户路径或默认路径归一为绝对路径，供唯一启动入口使用。 */
static fs_error_t fsc_sysroot_make_abs_path(const char *path, char *out,
                                            size_t out_size)
{
    fs_error_t err;
    char cwd[FSC_SYSROOT_PATH_MAX];
    const char *src;
    size_t cwd_len;
    size_t src_len;

    src = (path != NULL) ? path : g_default_path;

    if ((src == NULL) || (out == NULL) || (out_size == 0U))
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    src_len = strlen(src);
    if (src_len == 0U)
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    if (src[0] == '/')
    {
        if (src_len >= out_size)
        {
            return fsc_error(FSC_SUB_INIT, FS_ERRNO_ENOSPC);
        }

        memcpy(out, src, src_len + 1U);
        return FS_OK;
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        err = fsc_error(FSC_SUB_INIT, errno);
        return err;
    }

    cwd_len = strlen(cwd);
    if (cwd_len + 1U + src_len >= out_size)
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_ENOSPC);
    }

    memcpy(out, cwd, cwd_len);
    out[cwd_len] = '/';
    memcpy(out + cwd_len + 1U, src, src_len + 1U);

    return FS_OK;
}

/* 把 LSA file handle 转换为 OBJECT 通用 obj_handle_t。 */
static fs_error_t fsc_sysroot_handle_from_lsa(obj_handle_t *out,
                                              const lsa_file_handle_t *handle,
                                              int32_t mount_id)
{
    if ((out == NULL) || (handle == NULL))
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    if (OBJMETA_MAX_HANDLE_SIZE < handle->handle_bytes)
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EOVERFLOW);
    }

    if ((handle->handle_type < 0) ||
        (UINT16_MAX < (uint32_t)handle->handle_type))
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EOVERFLOW);
    }

    memset(out, 0, sizeof(*out));

    out->mount_id = mount_id;
    out->type = (uint16_t)handle->handle_type;
    out->len = (uint16_t)handle->handle_bytes;

    memcpy(out->data, handle->data, handle->handle_bytes);

    return FS_OK;
}

fs_error_t fsc_sysroot_init(const char *path)
{
    fs_error_t err;
    lsa_file_handle_t lsa_handle;
    int32_t mount_id;
    char abs_path[FSC_SYSROOT_PATH_MAX];

    memset(&g_sysroot, 0, sizeof(g_sysroot));

    err = fs_mutex_init(&g_sysroot.lock, NULL, 0);
    if (fs_failed(err))
    {
        return err;
    }

    g_sysroot.lock_inited = true;
    g_sysroot.state = FSC_SYSROOT_STATE_INIT;

    err = fsc_sysroot_make_abs_path(path, abs_path, sizeof(abs_path));
    if (fs_failed(err))
    {
        fsc_sysroot_deinit();
        return err;
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    mount_id = 0;

    err = lsa_bootstrap_root(abs_path, &lsa_handle, &mount_id);
    if (fs_failed(err))
    {
        fsc_sysroot_deinit();
        return err;
    }

    err = fsc_sysroot_handle_from_lsa(&g_sysroot.root_handle, &lsa_handle,
                                      mount_id);
    if (fs_failed(err))
    {
        fsc_sysroot_deinit();
        return err;
    }

    g_sysroot.root_fuid = fuid_make(FSC_SYSROOT_FSID, FSC_SYSROOT_OBJECT_ID,
                                    FSC_SYSROOT_GEN, FUID_TYPE_DIR);

    memcpy(g_sysroot.path, abs_path, strlen(abs_path) + 1U);
    g_sysroot.state = FSC_SYSROOT_STATE_ACTIVE;

    return FS_OK;
}

void fsc_sysroot_deinit(void)
{
    if (g_sysroot.state == FSC_SYSROOT_STATE_ACTIVE)
    {
        g_sysroot.state = FSC_SYSROOT_STATE_DELETING;
        (void)lsa_release_mount(g_sysroot.root_handle.mount_id);
    }

    if (g_sysroot.lock_inited)
    {
        fs_mutex_destroy(&g_sysroot.lock);
    }

    memset(&g_sysroot, 0, sizeof(g_sysroot));
}

bool fsc_sysroot_is_active(void)
{
    return g_sysroot.state == FSC_SYSROOT_STATE_ACTIVE;
}

fs_error_t fsc_sysroot_get_fuid(fuid_t *root_out)
{
    if (root_out == NULL)
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    if (!fsc_sysroot_is_active())
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    *root_out = g_sysroot.root_fuid;
    return FS_OK;
}

fs_error_t fsc_sysroot_get_handle(obj_handle_t *handle_out)
{
    if (handle_out == NULL)
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    if (!fsc_sysroot_is_active())
    {
        return fsc_error(FSC_SUB_INIT, FS_ERRNO_EINVAL);
    }

    *handle_out = g_sysroot.root_handle;
    return FS_OK;
}

const char *fsc_sysroot_get_path(void)
{
    if (!fsc_sysroot_is_active())
    {
        return NULL;
    }

    return g_sysroot.path;
}
