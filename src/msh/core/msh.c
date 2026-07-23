#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int msh_run_command_arg(msh_context_t *ctx, int argc, char **argv,
                               int start)
{
    char line[MSH_LINE_MAX];
    size_t used;
    int i;

    if ((argv == NULL) || (start >= argc))
    {
        fprintf(stderr, "msh: missing command\n");
        return 1;
    }

    used = 0U;
    line[0] = 0;
    for (i = start; i < argc; i++)
    {
        int n;

        n = snprintf(line + used, sizeof(line) - used, "%s%s",
                     (i == start) ? "" : " ", argv[i]);
        if ((n < 0) || ((size_t)n >= (sizeof(line) - used)))
        {
            fprintf(stderr, "msh: command too long\n");
            return 1;
        }
        used += (size_t)n;
    }

    return msh_run_line(ctx, line);
}

int msh_main(int argc, char **argv)
{
    fs_error_t err;
    msh_context_t ctx;
    int rc;

    ctx.should_exit = false;
    ctx.interactive = isatty(STDIN_FILENO);

    err = runtime_init(NULL);
    if (fs_failed(err))
    {
        msh_print_error("runtime init", err);
        return 1;
    }

    if ((argc >= 3) && (strcmp(argv[1], "-c") == 0))
    {
        rc = msh_run_command_arg(&ctx, argc, argv, 2);
    }
    else if (argc == 1)
    {
        rc = msh_repl(&ctx);
    }
    else
    {
        fprintf(stderr, "usage: %s [-c command]\n", argv[0]);
        rc = 1;
    }

    runtime_deinit();
    return rc;
}

int msh_dispatch(msh_context_t *ctx, const msh_argv_t *args)
{
    if ((ctx == NULL) || (args == NULL) || (args->argc == 0))
    {
        return 0;
    }

    if (msh_cmd_meta(ctx, args) == 0)
    {
        return 0;
    }

    if (strcmp(args->argv[0], "fs") == 0)
    {
        return msh_cmd_fs(ctx, args);
    }

    return msh_cmd_file(ctx, args);
}

void msh_print_error(const char *op, fs_error_t err)
{
    if (fs_succeeded(err))
    {
        return;
    }

    fprintf(stderr, "msh: %s: %s (0x%x)\n", op, fs_error_str(err), err);
}

const char *msh_arg_or_default(const msh_argv_t *args, int index,
                               const char *fallback)
{
    if ((args == NULL) || (index < 0) || (index >= args->argc))
    {
        return fallback;
    }

    return args->argv[index];
}
