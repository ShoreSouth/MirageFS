#include "fsc/fsc_init.h"

#include "common/error/fs_sub.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/fsmgr/fsmgr.h"
#include "fsc/nspool/nspool.h"
#include "fsc/sysroot/sysroot.h"

/*
 * ============================================================
 * FSC module bootstrap
 * ============================================================
 */

fs_error_t fsc_init(void)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter");

    fs_sub_register(FS_MODULE_FSC,
                    (fs_sub_name_fn)fsc_sub_name);

    err = fsc_sysroot_init(NULL);
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsc_sysroot_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

    err = fsid_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsid_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_sysroot;
    }

    err = nspool_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("nspool_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_fsid;
    }

    err = fsmgr_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsmgr_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_nspool;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    goto out;

err_nspool:
    nspool_deinit();

err_fsid:
    fsid_deinit();

err_sysroot:
    fsc_sysroot_deinit();

out:
    return err;
}

void fsc_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();
    fsc_sysroot_deinit();

    FS_LOG_DUMP_INFO("exit: done");
}
