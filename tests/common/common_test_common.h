#pragma once

#include "framework/test_framework.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common/fs_common.h"

/* Common UT 按 src/common 基础设施组件拆分，编号组件位与本 enum 对齐。 */
typedef enum test_common_component
{
    TEST_COMMON_COMPONENT_ERROR = 0x01,
    TEST_COMMON_COMPONENT_MODULE = 0x02,
    TEST_COMMON_COMPONENT_FLAG = 0x03,
    TEST_COMMON_COMPONENT_PATH = 0x04,
    TEST_COMMON_COMPONENT_ATOMIC = 0x05,
    TEST_COMMON_COMPONENT_LOCK = 0x06,
    TEST_COMMON_COMPONENT_OS = 0x07,
    TEST_COMMON_COMPONENT_MEMPOOL = 0x08,
    TEST_COMMON_COMPONENT_HASH = 0x09,
    TEST_COMMON_COMPONENT_TYPE = 0x0a,
    TEST_COMMON_COMPONENT_METRICS = 0x0b,
} test_common_component_t;

extern const test_case_t COMMON_ERROR_CASES[];
extern const size_t COMMON_ERROR_CASE_COUNT;
extern const test_case_t COMMON_MODULE_CASES[];
extern const size_t COMMON_MODULE_CASE_COUNT;
extern const test_case_t COMMON_FLAG_CASES[];
extern const size_t COMMON_FLAG_CASE_COUNT;
extern const test_case_t COMMON_TYPE_CASES[];
extern const size_t COMMON_TYPE_CASE_COUNT;
extern const test_case_t COMMON_PATH_CASES[];
extern const size_t COMMON_PATH_CASE_COUNT;
extern const test_case_t COMMON_ATOMIC_CASES[];
extern const size_t COMMON_ATOMIC_CASE_COUNT;
extern const test_case_t COMMON_LOCK_CASES[];
extern const size_t COMMON_LOCK_CASE_COUNT;
extern const test_case_t COMMON_OS_CASES[];
extern const size_t COMMON_OS_CASE_COUNT;
extern const test_case_t COMMON_MEMPOOL_CASES[];
extern const size_t COMMON_MEMPOOL_CASE_COUNT;
extern const test_case_t COMMON_HASH_CASES[];
extern const size_t COMMON_HASH_CASE_COUNT;
extern const test_case_t COMMON_METRICS_CASES[];
extern const size_t COMMON_METRICS_CASE_COUNT;
