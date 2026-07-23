#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "common/fs_common.h"
#include "runtime/include/runtime.h"

#define MSH_LINE_MAX 1024U
#define MSH_ARG_MAX 32U
#define MSH_DIR_BATCH 256U

typedef struct msh_context
{
    bool should_exit;
    bool interactive;

} msh_context_t;

typedef struct msh_argv
{
    int argc;
    char *argv[MSH_ARG_MAX];

} msh_argv_t;

int msh_repl(msh_context_t *ctx);
int msh_run_line(msh_context_t *ctx, char *line);
int msh_parse_line(char *line, msh_argv_t *out);
int msh_dispatch(msh_context_t *ctx, const msh_argv_t *args);

int msh_cmd_meta(msh_context_t *ctx, const msh_argv_t *args);
int msh_cmd_fs(msh_context_t *ctx, const msh_argv_t *args);
int msh_cmd_file(msh_context_t *ctx, const msh_argv_t *args);

void msh_print_prompt(void);
void msh_print_error(const char *op, fs_error_t err);
const char *msh_arg_or_default(const msh_argv_t *args, int index,
                               const char *fallback);
