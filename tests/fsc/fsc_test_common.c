#include "fsc_test_common.h"

obj_handle_t test_fsc_make_root_handle(void)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = 9;
    handle.type = 1;
    handle.len = 4;
    handle.data[0] = 1;
    handle.data[1] = 2;
    handle.data[2] = 3;
    handle.data[3] = 4;
    return handle;
}

fuid_t test_fsc_make_root_fuid(fsc_fsid_t fsid)
{
    return fuid_make(fsid, 1, 1, FUID_TYPE_DIR);
}

void test_fsc_prepare_temp_dir(void)
{
    (void)mkdir("../output", 0755);
    (void)mkdir("../output/tests", 0755);
    (void)mkdir("../output/tests/fsc", 0755);
}

void test_fsc_cleanup_sysroot(const char *path)
{
    if (path != NULL) {
        (void)rmdir(path);
    }

    (void)rmdir("../output/tests/fsc");
    (void)rmdir("../output/tests");
    (void)rmdir("../output");
}
