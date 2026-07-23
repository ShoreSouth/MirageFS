#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "runtime/internal/runtime_internal.h"
#include "runtime/internal/runtime_sub.h"
#include "runtime/include/runtime.h"

/* Runtime UT 按生命周期、会话和主流程拆分，组件编号参与 0xMMCCLIII 用例编号。 */
typedef enum test_runtime_component
{
    TEST_RUNTIME_COMPONENT_LIFECYCLE = 0x01,
    TEST_RUNTIME_COMPONENT_SESSION = 0x02,
    TEST_RUNTIME_COMPONENT_GETTER = 0x03,
    TEST_RUNTIME_COMPONENT_FLOW = 0x04,
    TEST_RUNTIME_COMPONENT_SUB = 0x05,
} test_runtime_component_t;

void test_runtime_cleanup_root(void);

#define TEST_RUNTIME_EXPECT_ERRNO(err, expected_errno)                         \
    do                                                                         \
    {                                                                          \
        TEST_ASSERT_EQ_INT(FS_MODULE_RUNTIME, fs_err_module((err)));           \
        TEST_ASSERT_EQ_INT((expected_errno), fs_err_errno((err)));             \
    } while (0)

extern const test_case_t RUNTIME_SUB_CASES[];
extern const size_t RUNTIME_SUB_CASE_COUNT;
extern const test_case_t RUNTIME_LIFECYCLE_CASES[];
extern const size_t RUNTIME_LIFECYCLE_CASE_COUNT;
extern const test_case_t RUNTIME_SESSION_CASES[];
extern const size_t RUNTIME_SESSION_CASE_COUNT;
extern const test_case_t RUNTIME_GETTER_CASES[];
extern const size_t RUNTIME_GETTER_CASE_COUNT;
extern const test_case_t RUNTIME_FLOW_CASES[];
extern const size_t RUNTIME_FLOW_CASE_COUNT;
