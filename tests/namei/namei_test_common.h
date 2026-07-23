#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "namei/include/namei.h"
#include "object/fuid/fuid.h"
#include "runtime/include/runtime.h"

/* NAMEI UT 按生命周期、上下文、lookup 和真实路径流程拆分。 */
typedef enum test_namei_component {
    TEST_NAMEI_COMPONENT_LIFECYCLE = 0x01,
    TEST_NAMEI_COMPONENT_CTX = 0x02,
    TEST_NAMEI_COMPONENT_LOOKUP = 0x03,
    TEST_NAMEI_COMPONENT_FLOW = 0x04,
} test_namei_component_t;

void test_namei_cleanup_root(void);

extern const test_case_t NAMEI_LIFECYCLE_CASES[];
extern const size_t NAMEI_LIFECYCLE_CASE_COUNT;
extern const test_case_t NAMEI_CTX_CASES[];
extern const size_t NAMEI_CTX_CASE_COUNT;
extern const test_case_t NAMEI_LOOKUP_CASES[];
extern const size_t NAMEI_LOOKUP_CASE_COUNT;
extern const test_case_t NAMEI_FLOW_CASES[];
extern const size_t NAMEI_FLOW_CASE_COUNT;
