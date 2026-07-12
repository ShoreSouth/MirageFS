#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>

#include "common/fs_common.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"

static int test_fsc_error_encodes_module_sub_errno(void)
{
    fs_error_t err = fsc_error(FSC_SUB_FSID, EINVAL);

    TEST_ASSERT_EQ_INT(FS_SEV_ERROR, fs_err_severity(err));
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(FSC_SUB_FSID, fs_err_sub(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));
    TEST_ASSERT_TRUE(fsc_sub_valid(FSC_SUB_FSID));
    TEST_ASSERT_STR_EQ("FSID", fsc_sub_name(FSC_SUB_FSID));
    return 0;
}

static int test_fsid_alloc_free_and_stale_guard(void)
{
    fs_error_t err;
    fsc_fsid_t fsid = FSID_INVALID;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_alloc(&fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);
    TEST_ASSERT_TRUE(fsid_is_valid(fsid));
    TEST_ASSERT_TRUE(fsid_hash(fsid) != 0);

    err = fsid_free(fsid);
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_free(fsid);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    fsid_deinit();
    return 0;
}

static int test_fsid_rejects_null_output(void)
{
    fs_error_t err;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_alloc(NULL);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    fsid_deinit();
    return 0;
}

static const test_case_t TEST_CASES[] = {
    TEST_CASE(test_fsc_error_encodes_module_sub_errno,
              "FSC 错误码布局",
              "构造 FSID 子模块错误",
              "severity/module/sub/errno 字段可正确解析"),
    TEST_CASE(test_fsid_alloc_free_and_stale_guard,
              "FSID 分配释放",
              "分配后释放，并重复释放旧 FSID",
              "第一次成功，重复释放被识别为非法/stale"),
    TEST_CASE(test_fsid_rejects_null_output,
              "FSID 参数校验",
              "fsid_alloc 传入 NULL 输出参数",
              "返回 FSC 模块 EINVAL"),
};

int main(void)
{
    return test_run_suite("fsc", TEST_CASES,
                          sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
}
