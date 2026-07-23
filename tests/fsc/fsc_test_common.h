#pragma once

#include "framework/test_framework.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "fsc/fsc_error.h"
#include "fsc/fsc_init.h"
#include "fsc/fsmgr/fsmgr.h"
#include "fsc/fsc_sub.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"
#include "fsc/fstable/fstable.h"
#include "fsc/nspool/nspool.h"
#include "fsc/sysroot/sysroot.h"
#include "object/object_init.h"

/* FSC UT 按 src/fsc 子组件拆分，组件编号参与 0xMMCCLIII 用例编号。 */
typedef enum test_fsc_component
{
    TEST_FSC_COMPONENT_ERROR = 0x01,
    TEST_FSC_COMPONENT_FSID = 0x02,
    TEST_FSC_COMPONENT_NAMESPACE = 0x03,
    TEST_FSC_COMPONENT_FSTABLE = 0x04,
    TEST_FSC_COMPONENT_NSPOOL = 0x05,
    TEST_FSC_COMPONENT_SYSROOT = 0x06,
    TEST_FSC_COMPONENT_FSMGR = 0x07,
    TEST_FSC_COMPONENT_INIT = 0x08,
} test_fsc_component_t;

obj_handle_t test_fsc_make_root_handle(void);
fuid_t test_fsc_make_root_fuid(fsc_fsid_t fsid);
void test_fsc_prepare_temp_dir(void);
void test_fsc_cleanup_sysroot(const char *path);

extern const test_case_t FSC_ERROR_CASES[];
extern const size_t FSC_ERROR_CASE_COUNT;
extern const test_case_t FSC_FSID_CASES[];
extern const size_t FSC_FSID_CASE_COUNT;
extern const test_case_t FSC_NAMESPACE_CASES[];
extern const size_t FSC_NAMESPACE_CASE_COUNT;
extern const test_case_t FSC_FSTABLE_CASES[];
extern const size_t FSC_FSTABLE_CASE_COUNT;
extern const test_case_t FSC_NSPOOL_CASES[];
extern const size_t FSC_NSPOOL_CASE_COUNT;
extern const test_case_t FSC_SYSROOT_CASES[];
extern const size_t FSC_SYSROOT_CASE_COUNT;
extern const test_case_t FSC_FSMGR_CASES[];
extern const size_t FSC_FSMGR_CASE_COUNT;
extern const test_case_t FSC_INIT_CASES[];
extern const size_t FSC_INIT_CASE_COUNT;
