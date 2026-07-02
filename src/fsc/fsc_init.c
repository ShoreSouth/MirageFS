#include "fsc/fsc_init.h"

#include "common/error/fs_sub.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/fsmgr/fsmgr.h"
#include "fsc/nspool/nspool.h"

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

    err = fsid_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsid_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    err = nspool_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("nspool_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        fsid_deinit();
        return err;
    }

    err = fsmgr_init();
    if (fs_failed(err)) {
        FS_LOG_DUMP_ERROR("fsmgr_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        nspool_deinit();
        fsid_deinit();
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}

void fsc_deinit(void)
{
    FS_LOG_DUMP_INFO("enter");

    fsmgr_deinit();
    nspool_deinit();
    fsid_deinit();

    FS_LOG_DUMP_INFO("exit: done");
}
