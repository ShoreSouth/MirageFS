#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int msh_need_session(void)
{
    if (!runtime_fs_is_active())
    {
        fprintf(stderr, "msh: no filesystem selected\n");
        return 1;
    }

    return 0;
}

static int msh_need_argc(const msh_argv_t *args, int argc, const char *usage)
{
    if ((args == NULL) || (args->argc != argc))
    {
        fprintf(stderr, "usage: %s\n", usage);
        return 1;
    }

    return 0;
}

static char msh_type_char(fs_type_t type)
{
    switch (type)
    {
    case FS_TYPE_DIR:
        return 'd';
    case FS_TYPE_LNK:
        return 'l';
    case FS_TYPE_FIFO:
        return 'p';
    case FS_TYPE_SOCK:
        return 's';
    case FS_TYPE_BLK:
        return 'b';
    case FS_TYPE_CHR:
        return 'c';
    case FS_TYPE_REG:
        return '-';
    default:
        return '?';
    }
}

static void msh_mode_string(mode_t mode, char out[11])
{
    out[0] = msh_type_char(fs_type_from_mode(mode));
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = 0;
}

static int msh_pwd(const msh_argv_t *args)
{
    char cwd[FS_MAX_PATH_LEN + 1U];
    fs_error_t err;

    if (msh_need_argc(args, 1, "pwd") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_getcwd(cwd, sizeof(cwd));
    if (fs_failed(err))
    {
        msh_print_error("pwd", err);
        return 1;
    }

    printf("%s\n", cwd);
    return 0;
}

static int msh_cd(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_need_argc(args, 2, "cd PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_chdir(args->argv[1]);
    if (fs_failed(err))
    {
        msh_print_error("cd", err);
        return 1;
    }

    return 0;
}

static int msh_mkdir(const msh_argv_t *args)
{
    fops_create_attr_t attr;
    fs_error_t err;

    if (msh_need_argc(args, 2, "mkdir PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = FS_MODE_DIR_DEFAULT;

    err = runtime_mkdir(args->argv[1], &attr, FS_FLAG_NONE, NULL);
    if (fs_failed(err))
    {
        msh_print_error("mkdir", err);
        return 1;
    }

    return 0;
}

static int msh_touch(const msh_argv_t *args)
{
    fops_create_attr_t attr;
    fs_error_t err;

    if (msh_need_argc(args, 2, "touch PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = FS_MODE_FILE_DEFAULT;

    err = runtime_create(args->argv[1], &attr, FS_FLAG_NONE, NULL);
    if (fs_failed(err))
    {
        msh_print_error("touch", err);
        return 1;
    }

    return 0;
}

static int msh_rm(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_need_argc(args, 2, "rm PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_unlink(args->argv[1], FS_FLAG_NONE);
    if (fs_failed(err))
    {
        msh_print_error("rm", err);
        return 1;
    }

    return 0;
}

static int msh_rmdir(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_need_argc(args, 2, "rmdir PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_rmdir(args->argv[1], FS_FLAG_NONE);
    if (fs_failed(err))
    {
        msh_print_error("rmdir", err);
        return 1;
    }

    return 0;
}

static int msh_mv(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_need_argc(args, 3, "mv OLD_PATH NEW_PATH") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_rename(args->argv[1], args->argv[2], FS_FLAG_REPLACE);
    if (fs_failed(err))
    {
        msh_print_error("mv", err);
        return 1;
    }

    return 0;
}

static void msh_print_attr(const fops_attr_t *attr)
{
    char mode[11];

    if (attr == NULL)
    {
        return;
    }

    msh_mode_string(attr->mode, mode);
    printf("type: %s\n", fs_type_to_str(attr->type));
    printf("mode: %s (%04o)\n", mode, (unsigned)(attr->mode & FS_PERM_MASK));
    printf("uid: %u\n", (unsigned)attr->uid);
    printf("gid: %u\n", (unsigned)attr->gid);
    printf("size: %llu\n", (unsigned long long)attr->size);
    printf("nlink: %llu\n", (unsigned long long)attr->nlink);
}

static int msh_stat(const msh_argv_t *args)
{
    fops_attr_t attr;
    fs_error_t err;

    if (msh_need_argc(args, 2, "stat PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_getattr(args->argv[1], FS_FLAG_NONE, &attr);
    if (fs_failed(err))
    {
        msh_print_error("stat", err);
        return 1;
    }

    msh_print_attr(&attr);
    return 0;
}

static int msh_ls_plain(const char *path)
{
    fops_dirent_t entries[MSH_DIR_BATCH];
    uint32_t nr;
    bool eof;
    fs_error_t err;

    err = runtime_readdir(path, FS_FLAG_DIRECTORY, entries, MSH_DIR_BATCH, &nr,
                          &eof);
    if (fs_failed(err))
    {
        msh_print_error("ls", err);
        return 1;
    }

    for (uint32_t i = 0; i < nr; i++)
    {
        printf("%s\n", entries[i].name);
    }

    (void)eof;
    return 0;
}

static int msh_ls_long(const char *path)
{
    fops_dirent_plus_t entries[MSH_DIR_BATCH];
    uint32_t nr;
    bool eof;
    fs_error_t err;
    char mode[11];

    err = runtime_readdirplus(path, FS_FLAG_DIRECTORY, entries, MSH_DIR_BATCH,
                              &nr, &eof);
    if (fs_failed(err))
    {
        msh_print_error("ll", err);
        return 1;
    }

    for (uint32_t i = 0; i < nr; i++)
    {
        msh_mode_string(entries[i].attr.mode, mode);
        printf("%s %8llu %s\n", mode, (unsigned long long)entries[i].attr.size,
               entries[i].entry.name);
    }

    (void)eof;
    return 0;
}

static int msh_ls(const msh_argv_t *args, bool long_format)
{
    const char *path;

    if ((args == NULL) || args->argc > 2)
    {
        fprintf(stderr, "usage: %s [PATH]\n", long_format ? "ll" : "ls");
        return 1;
    }
    if (msh_need_session() != 0)
    {
        return 1;
    }

    path = msh_arg_or_default(args, 1, ".");
    return long_format ? msh_ls_long(path) : msh_ls_plain(path);
}

int msh_cmd_file(msh_context_t *ctx, const msh_argv_t *args)
{
    const char *cmd;

    (void)ctx;

    if ((args == NULL) || (args->argc == 0))
    {
        return 0;
    }

    cmd = args->argv[0];
    if (strcmp(cmd, "pwd") == 0)
        return msh_pwd(args);
    if (strcmp(cmd, "cd") == 0)
        return msh_cd(args);
    if (strcmp(cmd, "mkdir") == 0)
        return msh_mkdir(args);
    if (strcmp(cmd, "touch") == 0)
        return msh_touch(args);
    if (strcmp(cmd, "rm") == 0)
        return msh_rm(args);
    if (strcmp(cmd, "rmdir") == 0)
        return msh_rmdir(args);
    if (strcmp(cmd, "mv") == 0)
        return msh_mv(args);
    if (strcmp(cmd, "stat") == 0)
        return msh_stat(args);
    if (strcmp(cmd, "ls") == 0)
        return msh_ls(args, false);
    if (strcmp(cmd, "ll") == 0)
        return msh_ls(args, true);

    fprintf(stderr, "msh: unknown command: %s\n", cmd);
    return 1;
}
