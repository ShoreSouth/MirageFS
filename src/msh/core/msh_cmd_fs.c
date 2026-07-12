#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>

static int msh_require_argc(const msh_argv_t *args,
                            int argc,
                            const char *usage)
{
    if ((args == NULL) || (args->argc != argc)) {
        fprintf(stderr, "usage: %s\n", usage);
        return 1;
    }

    return 0;
}

static int msh_fs_create(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 3, "fs create NAME") != 0) {
        return 1;
    }

    err = runtime_fs_create(args->argv[2], NULL);
    if (fs_failed(err)) {
        msh_print_error("fs create", err);
        return 1;
    }

    printf("created %s\n", args->argv[2]);
    return 0;
}

static int msh_fs_use(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 3, "fs use NAME") != 0) {
        return 1;
    }

    err = runtime_fs_use(args->argv[2]);
    if (fs_failed(err)) {
        msh_print_error("fs use", err);
        return 1;
    }

    return 0;
}

static int msh_fs_leave(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_require_argc(args, 2, "fs leave") != 0) {
        return 1;
    }

    err = runtime_fs_leave();
    if (fs_failed(err)) {
        msh_print_error("fs leave", err);
        return 1;
    }

    return 0;
}

static int msh_fs_current(const msh_argv_t *args)
{
    const char *name;

    if (msh_require_argc(args, 2, "fs current") != 0) {
        return 1;
    }

    name = runtime_fs_current();
    if (name == NULL) {
        printf("none\n");
    } else {
        printf("%s\n", name);
    }

    return 0;
}

int msh_cmd_fs(msh_context_t *ctx, const msh_argv_t *args)
{
    const char *sub;

    (void)ctx;

    if ((args == NULL) || (args->argc < 2)) {
        fprintf(stderr, "usage: fs create|use|leave|current ...\n");
        return 1;
    }

    sub = args->argv[1];
    if (strcmp(sub, "create") == 0) {
        return msh_fs_create(args);
    }
    if (strcmp(sub, "use") == 0) {
        return msh_fs_use(args);
    }
    if (strcmp(sub, "leave") == 0) {
        return msh_fs_leave(args);
    }
    if (strcmp(sub, "current") == 0) {
        return msh_fs_current(args);
    }

    fprintf(stderr, "msh: unknown fs command: %s\n", sub);
    return 1;
}
