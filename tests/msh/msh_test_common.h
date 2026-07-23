#pragma once

#include "framework/test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msh/include/msh.h"
#include "msh/internal/msh_internal.h"

/* MSH UT 按解析、参数 helper 和命令入口拆分，组件编号参与 0xMMCCLIII 用例编号。 */
typedef enum test_msh_component {
    TEST_MSH_COMPONENT_PARSE = 0x01,
    TEST_MSH_COMPONENT_ARGS = 0x02,
    TEST_MSH_COMPONENT_COMMAND = 0x03,
} test_msh_component_t;

void test_msh_cleanup_root(void);
int  test_msh_run_line(msh_context_t *ctx, const char *line);
int  test_msh_repl_with_stdin(const char *input, bool interactive);

extern const test_case_t MSH_PARSE_CASES[];
extern const size_t MSH_PARSE_CASE_COUNT;
extern const test_case_t MSH_ARGS_CASES[];
extern const size_t MSH_ARGS_CASE_COUNT;
extern const test_case_t MSH_COMMAND_CASES[];
extern const size_t MSH_COMMAND_CASE_COUNT;
