#include "msh/internal/msh_internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MSH_IO_CHUNK 4096U
#define MSH_XATTR_BUF 4096U

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

static int msh_parse_u64(const char *text, uint64_t *out)
{
    char *end;
    unsigned long long value;

    if ((text == NULL) || (out == NULL) || (text[0] == 0))
    {
        return 1;
    }

    errno = 0;
    value = strtoull(text, &end, 0);
    if ((errno != 0) || (end == text) || (*end != 0))
    {
        return 1;
    }

    *out = (uint64_t)value;
    return 0;
}

static int msh_parse_mode(const char *text, mode_t *out)
{
    char *end;
    unsigned long value;

    if ((text == NULL) || (out == NULL) || (text[0] == 0))
    {
        return 1;
    }

    errno = 0;
    value = strtoul(text, &end, 8);
    if ((errno != 0) || (end == text) || (*end != 0) ||
        ((value & ~FS_PERM_MASK) != 0UL))
    {
        return 1;
    }

    *out = (mode_t)value;
    return 0;
}

static int msh_parse_id(const char *text, uint32_t *out)
{
    uint64_t value;

    if (msh_parse_u64(text, &value) != 0 || value > UINT32_MAX)
    {
        return 1;
    }

    *out = (uint32_t)value;
    return 0;
}

static int msh_parse_access_mask(const char *text, int *out)
{
    int mask = 0;

    if ((text == NULL) || (out == NULL) || (text[0] == 0))
    {
        return 1;
    }

    if (strcmp(text, "f") == 0)
    {
        *out = F_OK;
        return 0;
    }

    for (const char *p = text; *p != 0; p++)
    {
        if (*p == 'r')
        {
            mask |= R_OK;
        }
        else if (*p == 'w')
        {
            mask |= W_OK;
        }
        else if (*p == 'x')
        {
            mask |= X_OK;
        }
        else
        {
            return 1;
        }
    }

    *out = mask;
    return 0;
}

static int msh_join_args(const msh_argv_t *args, int start, char *buf,
                         size_t size)
{
    size_t used = 0U;

    if ((args == NULL) || (buf == NULL) || (size == 0U) ||
        (start >= args->argc))
    {
        return 1;
    }

    buf[0] = 0;
    for (int i = start; i < args->argc; i++)
    {
        size_t arg_len = strlen(args->argv[i]);
        size_t sep = (i == start) ? 0U : 1U;

        if ((used + sep + arg_len + 1U) > size)
        {
            return 1;
        }

        if (sep != 0U)
        {
            buf[used++] = ' ';
        }
        memcpy(&buf[used], args->argv[i], arg_len);
        used += arg_len;
        buf[used] = 0;
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

    err = runtime_unlink(args->argv[1], FS_FLAG_NOFOLLOW);
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

static int msh_lookup(const msh_argv_t *args)
{
    fops_object_result_t result;
    fs_error_t err;

    if (msh_need_argc(args, 2, "lookup PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_lookup_plus(args->argv[1], FS_FLAG_NOFOLLOW, &result);
    if (fs_failed(err))
    {
        msh_print_error("lookup", err);
        return 1;
    }

    printf("fuid: %s\n", fuid_to_str(&result.fuid));
    msh_print_attr(&result.attr);
    return 0;
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

static int msh_mkfifo(const msh_argv_t *args)
{
    fops_object_result_t result;
    fs_error_t err;

    if (msh_need_argc(args, 2, "mkfifo PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_mknod(args->argv[1], FS_TYPE_FIFO, NULL, NULL,
                        FS_FLAG_EXCLUSIVE, &result);
    if (fs_failed(err))
    {
        msh_print_error("mkfifo", err);
        return 1;
    }

    return 0;
}

static int msh_ln(const msh_argv_t *args)
{
    fops_object_result_t result;
    fs_error_t err;

    if (args == NULL)
    {
        return 1;
    }

    if ((args->argc == 4) && (strcmp(args->argv[1], "-s") == 0))
    {
        if (msh_need_session() != 0)
        {
            return 1;
        }
        err = runtime_symlink(args->argv[2], args->argv[3], FS_FLAG_EXCLUSIVE,
                              NULL);
        if (fs_failed(err))
        {
            msh_print_error("ln -s", err);
            return 1;
        }
        return 0;
    }

    if (msh_need_argc(args, 3, "ln [-s] OLD NEW") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_link(args->argv[1], args->argv[2], FS_FLAG_EXCLUSIVE,
                       &result);
    if (fs_failed(err))
    {
        msh_print_error("ln", err);
        return 1;
    }

    return 0;
}

static int msh_symlink(const msh_argv_t *args)
{
    fs_error_t err;

    if (msh_need_argc(args, 3, "symlink TARGET LINKPATH") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_symlink(args->argv[1], args->argv[2], FS_FLAG_EXCLUSIVE,
                          NULL);
    if (fs_failed(err))
    {
        msh_print_error("symlink", err);
        return 1;
    }

    return 0;
}

static int msh_readlink(const msh_argv_t *args)
{
    char buf[FS_MAX_PATH_LEN + 1U];
    size_t actual;
    fs_error_t err;

    if (msh_need_argc(args, 2, "readlink PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    err = runtime_readlink(args->argv[1], FS_FLAG_NONE, buf, sizeof(buf) - 1U,
                           &actual);
    if (fs_failed(err))
    {
        msh_print_error("readlink", err);
        return 1;
    }

    if (actual < sizeof(buf))
    {
        buf[actual] = 0;
    }
    printf("%s\n", buf);
    return 0;
}

static int msh_close_file(const char *op, fops_file_t *file, int rc)
{
    fs_error_t close_err;

    if (file == NULL)
    {
        return rc;
    }

    close_err = runtime_close(file);
    if (fs_failed(close_err) && rc == 0)
    {
        msh_print_error(op, close_err);
        return 1;
    }

    return rc;
}

static int msh_cat(const msh_argv_t *args)
{
    char buf[MSH_IO_CHUNK];
    fops_file_t *file = NULL;
    size_t actual;
    fs_error_t err;

    if (msh_need_argc(args, 2, "cat PATH") != 0 || msh_need_session() != 0)
    {
        return 1;
    }

    err = runtime_open(args->argv[1], FS_FLAG_READ | FS_FLAG_REGULAR, &file);
    if (fs_failed(err))
    {
        msh_print_error("cat", err);
        return 1;
    }

    do
    {
        err = runtime_read(file, buf, sizeof(buf), &actual);
        if (fs_failed(err))
        {
            msh_print_error("cat", err);
            return msh_close_file("cat", file, 1);
        }
        if (actual > 0U && fwrite(buf, 1U, actual, stdout) != actual)
        {
            return msh_close_file("cat", file, 1);
        }
    } while (actual > 0U);

    return msh_close_file("cat", file, 0);
}

static int msh_write_common(const msh_argv_t *args, bool append)
{
    char text[MSH_LINE_MAX];
    fops_create_attr_t attr;
    fops_file_t *file = NULL;
    fs_flags_t open_flags;
    size_t actual;
    fs_error_t err;

    if ((args == NULL) || (args->argc < 3))
    {
        fprintf(stderr, "usage: %s PATH TEXT\n", append ? "append" : "write");
        return 1;
    }
    if ((msh_need_session() != 0) ||
        (msh_join_args(args, 2, text, sizeof(text)) != 0))
    {
        return 1;
    }

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_CREATE_ATTR_MODE;
    attr.mode = FS_MODE_FILE_DEFAULT;
    err = runtime_create(args->argv[1], &attr, FS_FLAG_REPLACE, NULL);
    if (fs_failed(err))
    {
        msh_print_error(append ? "append" : "write", err);
        return 1;
    }

    open_flags = FS_FLAG_WRITE | FS_FLAG_REGULAR;
    open_flags |= append ? FS_FLAG_APPEND : FS_FLAG_TRUNCATE;
    err = runtime_open(args->argv[1], open_flags, &file);
    if (fs_failed(err))
    {
        msh_print_error(append ? "append" : "write", err);
        return 1;
    }

    err = runtime_write(file, text, strlen(text), &actual);
    if (fs_failed(err) || actual != strlen(text))
    {
        if (fs_failed(err))
        {
            msh_print_error(append ? "append" : "write", err);
        }
        return msh_close_file(append ? "append" : "write", file, 1);
    }

    return msh_close_file(append ? "append" : "write", file, 0);
}

static int msh_chmod(const msh_argv_t *args)
{
    fops_setattr_t attr;
    mode_t mode;
    fs_error_t err;

    if (msh_need_argc(args, 3, "chmod MODE PATH") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    if (msh_parse_mode(args->argv[1], &mode) != 0)
    {
        fprintf(stderr, "usage: chmod MODE PATH\n");
        return 1;
    }

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_SETATTR_MODE;
    attr.mode = mode;
    err = runtime_setattr(args->argv[2], &attr, FS_FLAG_NONE);
    if (fs_failed(err))
    {
        msh_print_error("chmod", err);
        return 1;
    }

    return 0;
}

static int msh_chown(const msh_argv_t *args)
{
    fops_setattr_t attr;
    uint32_t uid;
    uint32_t gid;
    fs_error_t err;

    if (msh_need_argc(args, 4, "chown UID GID PATH") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    if ((msh_parse_id(args->argv[1], &uid) != 0) ||
        (msh_parse_id(args->argv[2], &gid) != 0))
    {
        fprintf(stderr, "usage: chown UID GID PATH\n");
        return 1;
    }

    memset(&attr, 0, sizeof(attr));
    attr.valid_mask = FOPS_SETATTR_UID | FOPS_SETATTR_GID;
    attr.uid = (uid_t)uid;
    attr.gid = (gid_t)gid;
    err = runtime_setattr(args->argv[3], &attr, FS_FLAG_NONE);
    if (fs_failed(err))
    {
        msh_print_error("chown", err);
        return 1;
    }

    return 0;
}

static int msh_truncate(const msh_argv_t *args)
{
    uint64_t size;
    fs_error_t err;

    if (msh_need_argc(args, 3, "truncate PATH SIZE") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    if (msh_parse_u64(args->argv[2], &size) != 0)
    {
        fprintf(stderr, "usage: truncate PATH SIZE\n");
        return 1;
    }

    err = runtime_truncate(args->argv[1], size, FS_FLAG_REGULAR);
    if (fs_failed(err))
    {
        msh_print_error("truncate", err);
        return 1;
    }

    return 0;
}

static int msh_access(const msh_argv_t *args)
{
    int mask;
    fs_error_t err;

    if (msh_need_argc(args, 3, "access PATH f|r|w|x|rw|rx|wx|rwx") != 0 ||
        msh_need_session() != 0)
    {
        return 1;
    }

    if (msh_parse_access_mask(args->argv[2], &mask) != 0)
    {
        fprintf(stderr, "usage: access PATH f|r|w|x|rw|rx|wx|rwx\n");
        return 1;
    }

    err = runtime_access(args->argv[1], mask, FS_FLAG_NONE);
    if (fs_failed(err))
    {
        msh_print_error("access", err);
        return 1;
    }

    return 0;
}

static int msh_xattr(const msh_argv_t *args)
{
    char buf[MSH_XATTR_BUF];
    size_t actual;
    fs_error_t err;

    if ((args == NULL) || (args->argc < 3))
    {
        fprintf(stderr,
                "usage: xattr list|get|set|remove PATH [NAME] [VALUE]\n");
        return 1;
    }
    if (msh_need_session() != 0)
    {
        return 1;
    }

    if (strcmp(args->argv[1], "list") == 0)
    {
        if (msh_need_argc(args, 3, "xattr list PATH") != 0)
        {
            return 1;
        }
        memset(buf, 0, sizeof(buf));
        err = runtime_listxattr(args->argv[2], buf, sizeof(buf), &actual);
        if (fs_failed(err))
        {
            msh_print_error("xattr list", err);
            return 1;
        }
        for (size_t off = 0U; off < actual;)
        {
            size_t len = strlen(&buf[off]);
            printf("%s\n", &buf[off]);
            off += len + 1U;
        }
        return 0;
    }

    if (strcmp(args->argv[1], "get") == 0)
    {
        if (msh_need_argc(args, 4, "xattr get PATH NAME") != 0)
        {
            return 1;
        }
        memset(buf, 0, sizeof(buf));
        err = runtime_getxattr(args->argv[2], args->argv[3], buf,
                               sizeof(buf) - 1U, &actual);
        if (fs_failed(err))
        {
            msh_print_error("xattr get", err);
            return 1;
        }
        if (actual < sizeof(buf))
        {
            buf[actual] = 0;
        }
        printf("%s\n", buf);
        return 0;
    }

    if (strcmp(args->argv[1], "set") == 0)
    {
        if (msh_need_argc(args, 5, "xattr set PATH NAME VALUE") != 0)
        {
            return 1;
        }
        err = runtime_setxattr(args->argv[2], args->argv[3], args->argv[4],
                               strlen(args->argv[4]), FS_FLAG_NONE);
        if (fs_failed(err))
        {
            msh_print_error("xattr set", err);
            return 1;
        }
        return 0;
    }

    if (strcmp(args->argv[1], "remove") == 0)
    {
        if (msh_need_argc(args, 4, "xattr remove PATH NAME") != 0)
        {
            return 1;
        }
        err = runtime_removexattr(args->argv[2], args->argv[3]);
        if (fs_failed(err))
        {
            msh_print_error("xattr remove", err);
            return 1;
        }
        return 0;
    }

    fprintf(stderr, "msh: unknown xattr command: %s\n", args->argv[1]);
    return 1;
}

static int msh_statfs(const msh_argv_t *args)
{
    const char *path;
    fops_statfs_t st;
    fs_error_t err;

    if ((args == NULL) || args->argc > 2)
    {
        fprintf(stderr, "usage: statfs [PATH]\n");
        return 1;
    }
    if (msh_need_session() != 0)
    {
        return 1;
    }

    path = msh_arg_or_default(args, 1, ".");
    err = runtime_statfs(path, &st);
    if (fs_failed(err))
    {
        msh_print_error("statfs", err);
        return 1;
    }

    printf("type: 0x%lx\n", (unsigned long)st.f_type);
    printf("block_size: %lu\n", (unsigned long)st.f_bsize);
    printf("blocks: %llu\n", (unsigned long long)st.f_blocks);
    printf("free_blocks: %llu\n", (unsigned long long)st.f_bfree);
    printf("files: %llu\n", (unsigned long long)st.f_files);
    printf("free_files: %llu\n", (unsigned long long)st.f_ffree);
    return 0;
}

static int msh_syncfs(const msh_argv_t *args)
{
    const char *path;
    fs_error_t err;

    if ((args == NULL) || args->argc > 2)
    {
        fprintf(stderr, "usage: syncfs [PATH]\n");
        return 1;
    }
    if (msh_need_session() != 0)
    {
        return 1;
    }

    path = msh_arg_or_default(args, 1, ".");
    err = runtime_syncfs(path);
    if (fs_failed(err))
    {
        msh_print_error("syncfs", err);
        return 1;
    }

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
    if (strcmp(cmd, "lookup") == 0)
        return msh_lookup(args);
    if (strcmp(cmd, "stat") == 0)
        return msh_stat(args);
    if (strcmp(cmd, "mkfifo") == 0)
        return msh_mkfifo(args);
    if (strcmp(cmd, "ln") == 0)
        return msh_ln(args);
    if (strcmp(cmd, "symlink") == 0)
        return msh_symlink(args);
    if (strcmp(cmd, "readlink") == 0)
        return msh_readlink(args);
    if (strcmp(cmd, "cat") == 0)
        return msh_cat(args);
    if (strcmp(cmd, "write") == 0)
        return msh_write_common(args, false);
    if (strcmp(cmd, "append") == 0)
        return msh_write_common(args, true);
    if (strcmp(cmd, "chmod") == 0)
        return msh_chmod(args);
    if (strcmp(cmd, "chown") == 0)
        return msh_chown(args);
    if (strcmp(cmd, "truncate") == 0)
        return msh_truncate(args);
    if (strcmp(cmd, "access") == 0)
        return msh_access(args);
    if (strcmp(cmd, "xattr") == 0)
        return msh_xattr(args);
    if (strcmp(cmd, "statfs") == 0)
        return msh_statfs(args);
    if (strcmp(cmd, "syncfs") == 0)
        return msh_syncfs(args);
    if (strcmp(cmd, "ls") == 0)
        return msh_ls(args, false);
    if (strcmp(cmd, "ll") == 0)
        return msh_ls(args, true);

    fprintf(stderr, "msh: unknown command: %s\n", cmd);
    return 1;
}
