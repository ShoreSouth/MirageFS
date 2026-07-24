#include "fsc/fsc_init.h"

#include "common/error/fs_sub.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/fsmgr/fsmgr.h"
#include "fsc/nspool/nspool.h"
#include "fsc/sysroot/sysroot.h"

#ifdef FS_TEST_FAULTS
#include <stdlib.h>
#include <string.h>

static bool fsc_test_fault_enabled(const char *point)
{
    const char *value = getenv("MIRAGEFS_FSC_INIT_FAIL");

    return (value != NULL) && (strcmp(value, point) == 0);
}
#endif

/*
 * ============================================================
 * FSC module bootstrap
 * ============================================================
 */

fs_error_t fsc_init(void)
{
    fs_error_t err;

    FS_LOG_DUMP_INFO("enter");

    fs_sub_register(FS_MODULE_FSC, (fs_sub_name_fn)fsc_sub_name);

    err = fsc_sysroot_init(NULL);
    if (fs_failed(err))
    {
        FS_LOG_DUMP_ERROR("fsc_sysroot_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto out;
    }

#ifdef FS_TEST_FAULTS
    if (fsc_test_fault_enabled("fsid"))
    {
        err = fsc_error(FSC_SUB_FSID, FS_ERRNO_ENOMEM);
    }
    else
#endif
    {
        err = fsid_init();
    }
    if (fs_failed(err))
    {
        FS_LOG_DUMP_ERROR("fsid_init failed, err=%s (0x%x)", fs_error_str(err),
                          err);
        goto err_sysroot;
    }

#ifdef FS_TEST_FAULTS
    if (fsc_test_fault_enabled("nspool"))
    {
        err = fsc_error(FSC_SUB_NSPOOL, FS_ERRNO_ENOMEM);
    }
    else
#endif
    {
        err = nspool_init();
    }
    if (fs_failed(err))
    {
        FS_LOG_DUMP_ERROR("nspool_init failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_fsid;
    }

#ifdef FS_TEST_FAULTS
    if (fsc_test_fault_enabled("fsmgr"))
    {
        err = fsc_error(FSC_SUB_INIT, FS_ERRNO_ENOMEM);
    }
    else
#endif
    {
        err = fsmgr_init();
    }
    if (fs_failed(err))
    {
        FS_LOG_DUMP_ERROR("fsmgr_init failed, err=%s (0x%x)", fs_error_str(err),
                          err);
        goto err_nspool;
    }

    err = fsmgr_recover();
    if (fs_failed(err))
    {
        FS_LOG_DUMP_ERROR("fsmgr_recover failed, err=%s (0x%x)",
                          fs_error_str(err), err);
        goto err_fsmgr;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    goto out;

err_fsmgr:
    fsmgr_deinit();

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
