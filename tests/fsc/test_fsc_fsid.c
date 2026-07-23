#include "fsc_test_common.h"

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


static int test_fsid_reports_exhaustion_and_invalid_free(void)
{
    enum { TEST_FSID_MAX_ALLOC = 4096 };
    fsc_fsid_t fsids[TEST_FSID_MAX_ALLOC];
    fsc_fsid_t extra = FSID_INVALID;
    fs_error_t err;
    size_t i;

    err = fsid_init();
    TEST_ASSERT_EQ_INT(FS_OK, err);

    err = fsid_free(FSID_INVALID);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(EINVAL, fs_err_errno(err));

    for (i = 0; i < TEST_FSID_MAX_ALLOC; i++) {
        err = fsid_alloc(&fsids[i]);
        TEST_ASSERT_EQ_INT(FS_OK, err);
    }

    err = fsid_alloc(&extra);
    TEST_ASSERT_EQ_INT(FS_MODULE_FSC, fs_err_module(err));
    TEST_ASSERT_EQ_INT(ENOSPC, fs_err_errno(err));

    for (i = 0; i < TEST_FSID_MAX_ALLOC; i++) {
        err = fsid_free(fsids[i]);
        TEST_ASSERT_EQ_INT(FS_OK, err);
    }
    fsid_deinit();
    return 0;
}


const test_case_t FSC_FSID_CASES[] = {
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1,
                             0x001),
                  test_fsid_alloc_free_and_stale_guard,
                  "FSID 分配释放",
                  "分配后释放，并重复释放旧 FSID",
                  "第一次成功，重复释放被识别为非法/stale"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1,
                             0x002),
                  test_fsid_rejects_null_output,
                  "FSID 参数校验",
                  "fsid_alloc 传入 NULL 输出参数",
                  "返回 FSC 模块 EINVAL"),
    TEST_CASE(UT_LIST_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1),
                  UT_CASE_NO(UT_MOD_FSC,
                             TEST_FSC_COMPONENT_FSID,
                             0x1,
                             0x003),
                  test_fsid_reports_exhaustion_and_invalid_free,
                  "FSID 容量边界",
                  "释放非法 FSID，并分配满所有 slot 后继续申请",
                  "非法释放返回 EINVAL，容量耗尽返回 ENOSPC"),
};

const size_t FSC_FSID_CASE_COUNT = sizeof(FSC_FSID_CASES) / sizeof(FSC_FSID_CASES[0]);
