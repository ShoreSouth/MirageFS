#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>

static void msh_print_help(void)
{
    printf("Mirage Shell (msh)\n");
    printf("\n");
    printf("shell:\n");
    printf("  help                 show this help\n");
    printf("  version              show version\n");
    printf("  exit | quit          leave msh\n");
    printf("\n");
    printf("filesystem:\n");
    printf("  fs create NAME       create filesystem\n");
    printf("  fs use NAME          enter filesystem\n");
    printf("  fs leave             leave current filesystem\n");
    printf("  fs current           show current filesystem\n");
    printf("  pwd                  show current directory\n");
    printf("\n");
    printf("files:\n");
    printf("  cd PATH              change directory\n");
    printf("  ls [PATH]            list directory\n");
    printf("  ll [PATH]            list directory with attributes\n");
    printf("  mkdir PATH           create directory\n");
    printf("  rmdir PATH           remove empty directory\n");
    printf("  touch PATH           create or reuse file\n");
    printf("  stat PATH            show attributes\n");
    printf("  rm PATH              remove file\n");
    printf("  mv OLD NEW           rename or move object\n");
}

int msh_cmd_meta(msh_context_t *ctx, const msh_argv_t *args)
{
    const char *cmd;

    if ((ctx == NULL) || (args == NULL) || (args->argc == 0))
    {
        return 1;
    }

    cmd = args->argv[0];
    if (strcmp(cmd, "help") == 0)
    {
        msh_print_help();
        return 0;
    }

    if (strcmp(cmd, "version") == 0)
    {
        printf("MirageFS msh 0.1\n");
        return 0;
    }

    if ((strcmp(cmd, "exit") == 0) || (strcmp(cmd, "quit") == 0))
    {
        ctx->should_exit = true;
        return 0;
    }

    return 1;
}
