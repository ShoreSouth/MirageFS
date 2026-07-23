#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>

void msh_print_prompt(void)
{
    const char *fs_name;
    char cwd[FS_MAX_PATH_LEN + 1U];
    fs_error_t err;

    fs_name = runtime_fs_current();
    if (fs_name == NULL)
    {
        printf("msh> ");
        fflush(stdout);
        return;
    }

    err = runtime_getcwd(cwd, sizeof(cwd));
    if (fs_failed(err))
    {
        printf("msh:%s:? > ", fs_name);
        fflush(stdout);
        return;
    }

    printf("msh:%s:%s> ", fs_name, cwd);
    fflush(stdout);
}

int msh_run_line(msh_context_t *ctx, char *line)
{
    msh_argv_t args;
    int rc;

    rc = msh_parse_line(line, &args);
    if (rc != 0)
    {
        return rc;
    }

    return msh_dispatch(ctx, &args);
}

int msh_repl(msh_context_t *ctx)
{
    char line[MSH_LINE_MAX];
    int rc;

    if (ctx == NULL)
    {
        return 1;
    }

    while (!ctx->should_exit)
    {
        if (ctx->interactive)
        {
            msh_print_prompt();
        }

        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            break;
        }

        if (strchr(line, '\n') == NULL && !feof(stdin))
        {
            fprintf(stderr, "msh: input line too long\n");
            while (fgets(line, sizeof(line), stdin) != NULL)
            {
                if (strchr(line, '\n') != NULL)
                {
                    break;
                }
            }
            continue;
        }

        rc = msh_run_line(ctx, line);
        if (rc != 0 && !ctx->interactive)
        {
            return rc;
        }
    }

    return 0;
}
