#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"

/* LSA UT 按 syscall 能力族拆分，组件编号参与 0xMMCCLIII 用例编号。 */
typedef enum test_lsa_component
{
    TEST_LSA_COMPONENT_FILE = 0x01,
    TEST_LSA_COMPONENT_LOOKUP = 0x02,
    TEST_LSA_COMPONENT_CREATE = 0x03,
    TEST_LSA_COMPONENT_NAMESPACE = 0x04,
    TEST_LSA_COMPONENT_DIR = 0x05,
    TEST_LSA_COMPONENT_ATTR = 0x06,
    TEST_LSA_COMPONENT_XATTR = 0x07,
    TEST_LSA_COMPONENT_FS = 0x08,
    TEST_LSA_COMPONENT_HANDLE = 0x09,
    TEST_LSA_COMPONENT_ERROR = 0x0a,
} test_lsa_component_t;

int test_lsa_make_tmpdir(char *path, size_t size);
void test_lsa_cleanup_tmpdir(const char *path);
int test_lsa_open_tmp_root(char *path, size_t size, int *dirfd);
int test_lsa_assert_errno(fs_error_t err, uint8_t expected);
int test_lsa_assert_failed(fs_error_t err);
int test_lsa_create_file(int dirfd, const char *name, int *fd);

extern const test_case_t LSA_FILE_CASES[];
extern const size_t LSA_FILE_CASE_COUNT;
extern const test_case_t LSA_ERROR_CASES[];
extern const size_t LSA_ERROR_CASE_COUNT;
extern const test_case_t LSA_LOOKUP_CASES[];
extern const size_t LSA_LOOKUP_CASE_COUNT;
extern const test_case_t LSA_CREATE_CASES[];
extern const size_t LSA_CREATE_CASE_COUNT;
extern const test_case_t LSA_NAMESPACE_CASES[];
extern const size_t LSA_NAMESPACE_CASE_COUNT;
extern const test_case_t LSA_DIR_CASES[];
extern const size_t LSA_DIR_CASE_COUNT;
extern const test_case_t LSA_ATTR_CASES[];
extern const size_t LSA_ATTR_CASE_COUNT;
extern const test_case_t LSA_XATTR_CASES[];
extern const size_t LSA_XATTR_CASE_COUNT;
extern const test_case_t LSA_HANDLE_CASES[];
extern const size_t LSA_HANDLE_CASE_COUNT;
