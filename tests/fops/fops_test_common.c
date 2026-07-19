#include "fops_test_common.h"

#include <stdio.h>
#include <stdlib.h>

fuid_t test_fops_make_fuid(fuid_type_t type)
{
    fuid_t fuid;

    fuid = fuid_make(7U, 100U, 1U, type);
    fuid.qtreeid = 11U;
    fuid.snapid = 13U;
    fuid.shardid = 17U;
    return fuid;
}

obj_handle_t test_fops_make_handle(int32_t mount_id)
{
    obj_handle_t handle;

    memset(&handle, 0, sizeof(handle));
    handle.mount_id = mount_id;
    handle.type = 1U;
    handle.len = 4U;
    handle.data[0] = 0xaaU;
    handle.data[1] = 0xbbU;
    handle.data[2] = 0xccU;
    handle.data[3] = 0xddU;
    return handle;
}


static int test_fops_mkdir_if_missing(const char *path)
{
    if (mkdir(path, 0775) == 0) {
        return 0;
    }
    return errno == EEXIST ? 0 : -1;
}

static int test_fops_make_tmp_root(char *path, size_t size)
{
    const char *base;
    int written;

    base = "../output/tests/tmp";
    if (access("../output", F_OK) != 0) {
        base = "output/tests/tmp";
    }

    if (strcmp(base, "../output/tests/tmp") == 0) {
        if ((test_fops_mkdir_if_missing("../output") != 0) ||
            (test_fops_mkdir_if_missing("../output/tests") != 0) ||
            (test_fops_mkdir_if_missing("../output/tests/tmp") != 0)) {
            return -1;
        }
    } else {
        if ((test_fops_mkdir_if_missing("output") != 0) ||
            (test_fops_mkdir_if_missing("output/tests") != 0) ||
            (test_fops_mkdir_if_missing("output/tests/tmp") != 0)) {
            return -1;
        }
    }

    written = snprintf(path,
                       size,
                       "%s/miragefs-fops-test-%ld-XXXXXX",
                       base,
                       (long)getpid());
    if ((written < 0) || ((size_t)written >= size)) {
        return -1;
    }

    return mkdtemp(path) == NULL ? -1 : 0;
}

int test_fops_env_setup(test_fops_env_t *env)
{
    lsa_file_handle_t lsa_handle;
    obj_handle_t handle;
    fs_error_t err;

    if (env == NULL) {
        return -1;
    }

    memset(env, 0, sizeof(*env));
    if (test_fops_make_tmp_root(env->path, sizeof(env->path)) != 0) {
        return -1;
    }

    err = object_init();
    if (fs_failed(err)) {
        (void)rmdir(env->path);
        return -1;
    }
    err = fops_init();
    if (fs_failed(err)) {
        object_deinit();
        (void)rmdir(env->path);
        return -1;
    }

    memset(&lsa_handle, 0, sizeof(lsa_handle));
    err = lsa_bootstrap_root(env->path, &lsa_handle, &env->mount_id);
    if (fs_failed(err)) {
        fops_deinit();
        object_deinit();
        (void)rmdir(env->path);
        return -1;
    }

    err = objmeta_handle_from_lsa(&handle, &lsa_handle, env->mount_id);
    if (fs_failed(err)) {
        (void)lsa_release_mount(env->mount_id);
        fops_deinit();
        object_deinit();
        (void)rmdir(env->path);
        return -1;
    }

    env->root_fuid = fuid_make(21U, 9000001U, 1U, FUID_TYPE_DIR);
    if (objmgr_create(&env->root_fuid, &handle) == NULL) {
        (void)lsa_release_mount(env->mount_id);
        fops_deinit();
        object_deinit();
        (void)rmdir(env->path);
        return -1;
    }

    return 0;
}

void test_fops_env_teardown(test_fops_env_t *env)
{
    if (env == NULL) {
        return;
    }

    (void)objmgr_delete(&env->root_fuid);
    (void)lsa_release_mount(env->mount_id);
    fops_deinit();
    object_deinit();
    (void)rmdir(env->path);
}
