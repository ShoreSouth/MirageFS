#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"
#include "object/fuid/fuid.h"
#include "object/object_init.h"
#include "object/obj_error.h"
#include "object/objkey/objkey.h"
#include "object/objmeta/objmeta.h"
#include "object/objruntime/objruntime.h"
#include "object/objtable/objtable.h"
#include "object/objmgr/objmgr.h"
#include "object/objmgr/objmgr_internal.h"
#include "object/objpool/objpool.h"

/* Object UT 按 src/object 子组件拆分，组件编号参与 0xMMCCLIII 用例编号。 */
typedef enum test_object_component
{
    TEST_OBJECT_COMPONENT_FUID = 0x01,
    TEST_OBJECT_COMPONENT_OBJKEY = 0x02,
    TEST_OBJECT_COMPONENT_OBJMETA = 0x03,
    TEST_OBJECT_COMPONENT_OBJRUNTIME = 0x04,
    TEST_OBJECT_COMPONENT_OBJTABLE = 0x05,
    TEST_OBJECT_COMPONENT_OBJPOOL = 0x06,
    TEST_OBJECT_COMPONENT_OBJMGR = 0x07,
} test_object_component_t;

/* 共享构造器只提供稳定的身份/handle 数据，避免每个组件重复铺测试样板。 */
obj_handle_t test_object_make_handle_with_seed(uint8_t seed);
obj_handle_t test_object_make_handle(void);
fuid_t test_object_make_fuid(ObjectId_t objectid);

#define TEST_ASSERT_OBJECT_ERRNO(err, posix_errno)                             \
    do                                                                         \
    {                                                                          \
        fs_error_t test_err__ = (err);                                         \
        TEST_ASSERT_EQ_INT(FS_MODULE_OBJECT, fs_err_module(test_err__));       \
        TEST_ASSERT_EQ_INT((posix_errno), fs_err_errno(test_err__));           \
    } while (0)

extern const test_case_t OBJECT_FUID_CASES[];
extern const size_t OBJECT_FUID_CASE_COUNT;
extern const test_case_t OBJECT_OBJKEY_CASES[];
extern const size_t OBJECT_OBJKEY_CASE_COUNT;
extern const test_case_t OBJECT_OBJMETA_CASES[];
extern const size_t OBJECT_OBJMETA_CASE_COUNT;
extern const test_case_t OBJECT_OBJRUNTIME_CASES[];
extern const size_t OBJECT_OBJRUNTIME_CASE_COUNT;
extern const test_case_t OBJECT_OBJTABLE_CASES[];
extern const size_t OBJECT_OBJTABLE_CASE_COUNT;
extern const test_case_t OBJECT_OBJPOOL_CASES[];
extern const size_t OBJECT_OBJPOOL_CASE_COUNT;
extern const test_case_t OBJECT_OBJMGR_CASES[];
extern const size_t OBJECT_OBJMGR_CASE_COUNT;
