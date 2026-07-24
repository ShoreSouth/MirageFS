#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>

#define MSH_FS_LIST_MAX 256U

static int msh_require_argc(const msh_argv_t *args, int argc, const char *usage)
{
    if ((args == NULL) || (args->argc != argc))
    {
        fprintf(stderr, "usage: %s\n", usage);
        return 1;
    }

    return 0;
}

static int msh_fs_create(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 3, "fs create NAME") != 0)
    {
        return 1;
    }

    err = runtime_fs_create(args->argv[2], NULL);
    if (fs_failed(err))
    {
        msh_print_error("fs create", err);
        return 1;
    }

    printf("created %s\n", args->argv[2]);
    return 0;
}

static int msh_fs_list(const msh_argv_t *args)
{
    char names[MSH_FS_LIST_MAX][FSC_NAMESPACE_NAME_MAX];
    uint32_t actual;
    fs_error_t err;

    if (msh_require_argc(args, 2, "fs list") != 0)
    {
        return 1;
    }

    err = runtime_fs_list(names, MSH_FS_LIST_MAX, &actual);
    if (fs_failed(err))
    {
        msh_print_error("fs list", err);
        return 1;
    }

    if (actual == 0U)
    {
        printf("none\n");
        return 0;
    }

    for (uint32_t i = 0; i < actual; i++)
    {
        printf("%s\n", names[i]);
    }

    return 0;
}

static int msh_fs_enter(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 3, "fs enter NAME") != 0)
    {
        return 1;
    }

    err = runtime_fs_enter(args->argv[2]);
    if (fs_failed(err))
    {
        msh_print_error("fs enter", err);
        return 1;
    }

    return 0;
}

static int msh_fs_rename(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 4, "fs rename OLD NEW") != 0)
    {
        return 1;
    }

    err = runtime_fs_rename(args->argv[2], args->argv[3]);
    if (fs_failed(err))
    {
        msh_print_error("fs rename", err);
        return 1;
    }

    return 0;
}

static int msh_fs_destroy(const msh_argv_t *args)
{
    char confirm[FSC_NAMESPACE_NAME_MAX + 8U];
    size_t len;
    fs_error_t err;

    if (msh_require_argc(args, 3, "fs destroy NAME") != 0)
    {
        return 1;
    }

    printf("destroy filesystem %s? type %s to confirm: ", args->argv[2],
           args->argv[2]);
    fflush(stdout);
    if (fgets(confirm, sizeof(confirm), stdin) == NULL)
    {
        printf("cancelled\n");
        return 1;
    }

    len = strlen(confirm);
    if ((len > 0U) && (confirm[len - 1U] == '\n'))
    {
        confirm[len - 1U] = 0;
    }

    if (strcmp(confirm, args->argv[2]) != 0)
    {
        printf("cancelled\n");
        return 1;
    }

    err = runtime_fs_destroy_tree(args->argv[2]);
    if (fs_failed(err))
    {
        msh_print_error("fs destroy", err);
        return 1;
    }

    printf("destroyed %s\n", args->argv[2]);
    return 0;
}

static int msh_fs_leave(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 2, "fs leave") != 0)
    {
        return 1;
    }

    err = runtime_fs_leave();
    if (fs_failed(err))
    {
        msh_print_error("fs leave", err);
        return 1;
    }

    return 0;
}

static int msh_fs_current(const msh_argv_t *args)
{
    const char *name;

    if (msh_require_argc(args, 2, "fs current") != 0)
    {
        return 1;
    }

    name = runtime_fs_current();
    if (name == NULL)
    {
        printf("none\n");
    }
    else
    {
        printf("%s\n", name);
    }

    return 0;
}

int msh_cmd_fs(msh_context_t *ctx, const msh_argv_t *args)
{
    const char *sub;

    (void)ctx;

    if ((args == NULL) || (args->argc < 2))
    {
        fprintf(stderr,
                "usage: fs create|list|enter|leave|current|rename|destroy "
                "...\n");
        return 1;
    }

    sub = args->argv[1];
    if (strcmp(sub, "create") == 0)
    {
        return msh_fs_create(args);
    }
    if (strcmp(sub, "list") == 0)
    {
        return msh_fs_list(args);
    }
    if (strcmp(sub, "enter") == 0)
    {
        return msh_fs_enter(args);
    }
    if (strcmp(sub, "rename") == 0)
    {
        return msh_fs_rename(args);
    }
    if (strcmp(sub, "destroy") == 0)
    {
        return msh_fs_destroy(args);
    }
    if (strcmp(sub, "leave") == 0)
    {
        return msh_fs_leave(args);
    }
    if (strcmp(sub, "current") == 0)
    {
        return msh_fs_current(args);
    }

    fprintf(stderr, "msh: unknown fs command: %s\n", sub);
    return 1;
}
