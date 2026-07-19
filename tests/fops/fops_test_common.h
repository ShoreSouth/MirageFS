#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "fops/include/fops.h"
#include "fops/include/fops_types.h"
#include "fops/internal/fops_internal.h"
#include "lsa/include/lsa_api.h"
#include "object/object_init.h"
#include "object/objmgr/objmgr.h"

typedef enum test_fops_component {
    TEST_FOPS_COMPONENT_CORE = 0x01,
    TEST_FOPS_COMPONENT_DISPATCH = 0x02,
    TEST_FOPS_COMPONENT_VALIDATE = 0x03,
    TEST_FOPS_COMPONENT_CREATE = 0x04,
    TEST_FOPS_COMPONENT_FILE = 0x05,
    TEST_FOPS_COMPONENT_ATTR = 0x06,
    TEST_FOPS_COMPONENT_IDENTITY = 0x07,
    TEST_FOPS_COMPONENT_HANDLE = 0x08,
    TEST_FOPS_COMPONENT_OPS = 0x09,
    TEST_FOPS_COMPONENT_BOUNDARY = 0x0a,
} test_fops_component_t;

fuid_t test_fops_make_fuid(fuid_type_t type);
obj_handle_t test_fops_make_handle(int32_t mount_id);

typedef struct test_fops_env {
    char path[160];
    int32_t mount_id;
    fuid_t root_fuid;
} test_fops_env_t;

int test_fops_env_setup(test_fops_env_t *env);
void test_fops_env_teardown(test_fops_env_t *env);
#define TEST_FOPS_EXPECT_FOPS_ERRNO(err, expected_errno) \
    do { \
        TEST_ASSERT_EQ_INT(fs_err_module((err)), FS_MODULE_FOPS); \
        TEST_ASSERT_EQ_INT(fs_err_errno((err)), (expected_errno)); \
    } while (0)

extern const test_case_t FOPS_CORE_CASES[];
extern const size_t FOPS_CORE_CASE_COUNT;
extern const test_case_t FOPS_DISPATCH_CASES[];
extern const size_t FOPS_DISPATCH_CASE_COUNT;
extern const test_case_t FOPS_VALIDATE_CASES[];
extern const size_t FOPS_VALIDATE_CASE_COUNT;
extern const test_case_t FOPS_CREATE_CASES[];
extern const size_t FOPS_CREATE_CASE_COUNT;
extern const test_case_t FOPS_FILE_CASES[];
extern const size_t FOPS_FILE_CASE_COUNT;
extern const test_case_t FOPS_ATTR_CASES[];
extern const size_t FOPS_ATTR_CASE_COUNT;
extern const test_case_t FOPS_IDENTITY_CASES[];
extern const size_t FOPS_IDENTITY_CASE_COUNT;
extern const test_case_t FOPS_HANDLE_CASES[];
extern const size_t FOPS_HANDLE_CASE_COUNT;
extern const test_case_t FOPS_OPS_CASES[];
extern const size_t FOPS_OPS_CASE_COUNT;
extern const test_case_t FOPS_BOUNDARY_CASES[];
extern const size_t FOPS_BOUNDARY_CASE_COUNT;
