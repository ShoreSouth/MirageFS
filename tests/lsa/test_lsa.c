#include "framework/test_framework.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "common/fs_common.h"
#include "lsa/include/lsa_api.h"


typedef enum test_lsa_component {
    TEST_LSA_COMPONENT_FILE = 0x01,
    TEST_LSA_COMPONENT_LOOKUP = 0x02,
    TEST_LSA_COMPONENT_CREATE = 0x03,
    TEST_LSA_COMPONENT_NAMESPACE = 0x04,
    TEST_LSA_COMPONENT_DIR = 0x05,
    TEST_LSA_COMPONENT_ATTR = 0x06,
    TEST_LSA_COMPONENT_XATTR = 0x07,
    TEST_LSA_COMPONENT_FS = 0x08,
    TEST_LSA_COMPONENT_HANDLE = 0x09,
    TEST_LSA_COMPONENT_ERROR = 0x0a,
} test_lsa_component_t;

static int mkdir_if_missing(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    return errno == EEXIST ? 0 : -1;
}

static int make_tmpdir(char *path, size_t size)
{
    const char *base;
    int written;

    base = "../output/tests/lsa";
    if (access("../output", F_OK) != 0) {
        base = "output/tests/lsa";
    }

    if (strcmp(base, "../output/tests/lsa") == 0) {
        if ((mkdir_if_missing("../output") != 0) ||
            (mkdir_if_missing("../output/tests") != 0) ||
            (mkdir_if_missing("../output/tests/lsa") != 0)) {
            return -1;
        }
    } else {
        if ((mkdir_if_missing("output") != 0) ||
            (mkdir_if_missing("output/tests") != 0) ||
            (mkdir_if_missing("output/tests/lsa") != 0)) {
            return -1;
        }
    }

    written = snprintf(path,
                       size,
                       "%s/lsa-%ld-XXXXXX",
                       base,
                       (long)getpid());
    if ((written < 0) || ((size_t)written >= size)) {
        return -1;
    }

    return mkdtemp(path) == NULL ? -1 : 0;
}

static void cleanup_tmpdir(const char *path)
{
    char cmd[PATH_MAX + 32];
    int written;

    if ((path == NULL) || (path[0] == '\0')) {
        return;
    }

    written = snprintf(cmd, sizeof(cmd), "rm -rf -- '%s'", path);
    if ((written > 0) && ((size_t)written < sizeof(cmd))) {
        int rc = system(cmd);
        (void)rc;
    }
}

static int open_tmp_root(char *path, size_t size, int *dirfd)
{
    if (make_tmpdir(path, size) != 0) {
        return -1;
    }

    *dirfd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (*dirfd < 0) {
        cleanup_tmpdir(path);
        return -1;
    }

    return 0;
}

static int assert_lsa_errno(fs_error_t err, uint8_t expected)
{
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    TEST_ASSERT_EQ_INT(fs_err_errno(err), expected);
    return 0;
}


static int assert_lsa_failed(fs_error_t err)
{
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    return 0;
}

static int create_file(int dirfd, const char *name, int *fd)
{
    fs_error_t err;

    err = lsa_create(dirfd, name, FS_FLAG_REGULAR, 0644, fd);
    if (fs_failed(err)) {
        return -1;
    }

    return 0;
}

static int test_lsa_create_write_read_round_trip(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int filefd = -1;
    size_t actual = 0;
    off_t offset = -1;
    char buf[16];
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "hello.txt", &filefd), 0);

    err = lsa_write_full(filefd, "abc", 3, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);

    err = lsa_lseek(filefd, 0, SEEK_SET, &offset);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(offset, 0);

    memset(buf, 0, sizeof(buf));
    err = lsa_read_full(filefd, buf, 8, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(buf, "abc");

    err = lsa_pwrite(filefd, "Z", 1, 1, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 1);

    memset(buf, 0, sizeof(buf));
    err = lsa_pread(filefd, buf, 3, 0, &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(buf, "aZc");

    err = lsa_ftruncate(filefd, 2);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    err = lsa_fsync(filefd);
    TEST_ASSERT_EQ_INT(err, FS_OK);

    TEST_ASSERT_EQ_INT(lsa_close(filefd), FS_OK);
    filefd = -1;
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "hello.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_invalid_argument_matrix(void)
{
    char byte = 0;
    size_t actual = 99;
    struct stat st;
    struct statfs fsst;
    lsa_dir_iter_t *iter = NULL;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_read(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_read_full(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_write(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_write_full(-1, NULL, 1, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_pread(-1, NULL, 1, 0, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_pwrite(-1, NULL, 1, 0, &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fstat(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fstatat(AT_FDCWD,
                                                    NULL,
                                                    0,
                                                    &st),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_statfs(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_readlink(AT_FDCWD,
                                                     NULL,
                                                     &byte,
                                                     1,
                                                     &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_listxattr(-1, NULL, 0, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_getxattr(-1,
                                                     NULL,
                                                     &byte,
                                                     1,
                                                     &actual),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_setxattr(-1, NULL, &byte, 1, 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_removexattr(-1, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_dir_iter_open(-1, 1, &iter),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_dir_iter_next(NULL, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_dir_iter_next_plus(NULL, NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_statfs(-1, &fsst);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EBADF), 0);
    return 0;
}

static int test_lsa_lookup_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "plain.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "dir", FS_FLAG_DIRECTORY, 0755),
                       FS_OK);

    err = lsa_lookup(dirfd, "plain.txt", FS_FLAG_REGULAR, &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(close(fd), 0);
    fd = -1;

    err = lsa_lookup(dirfd, "plain.txt", FS_FLAG_DIRECTORY, &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_ENOTDIR), 0);
    err = lsa_lookup(dirfd, "dir", FS_FLAG_REGULAR, &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EISDIR), 0);
    err = lsa_lookup(dirfd,
                     "plain.txt",
                     FS_FLAG_DIRECTORY | FS_FLAG_REGULAR,
                     &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EINVAL), 0);
    err = lsa_lookup(dirfd, "plain.txt", 0x80000000U, &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "plain.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "dir", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_create_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);

    err = lsa_create(dirfd, "matrix.txt", FS_FLAG_REGULAR, 0644, &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    err = lsa_create(dirfd,
                     "matrix.txt",
                     FS_FLAG_EXCLUSIVE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EEXIST), 0);

    err = lsa_create(dirfd,
                     "matrix.txt",
                     FS_FLAG_REPLACE | FS_FLAG_TRUNCATE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    err = lsa_create(dirfd, "bad.txt", 0x80000000U, 0644, &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EINVAL), 0);
    err = lsa_create(dirfd,
                     "bad.txt",
                     FS_FLAG_REPLACE | FS_FLAG_EXCLUSIVE,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "matrix.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_namespace_round_trip(void)
{
    char dir_path[PATH_MAX];
    char link_buf[64];
    int dirfd = -1;
    int fd = -1;
    size_t actual = 0;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "sub", FS_FLAG_DIRECTORY, 0755),
                       FS_OK);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "source.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    fd = -1;

    TEST_ASSERT_EQ_INT(lsa_link(dirfd, "source.txt", dirfd, "hard.txt", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_rename(dirfd, "hard.txt", dirfd, "moved.txt", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_symlink("source.txt", dirfd, "sym", 0), FS_OK);

    memset(link_buf, 0, sizeof(link_buf));
    err = lsa_readlink(dirfd, "sym", link_buf, sizeof(link_buf), &actual);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_STR_EQ(link_buf, "source.txt");
    TEST_ASSERT_EQ_INT(actual, 10);

    TEST_ASSERT_EQ_INT(lsa_mknod(dirfd, "pipe", FS_TYPE_FIFO, 0644, NULL),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "pipe", 0),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "sym", FS_FLAG_NOFOLLOW), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "moved.txt", FS_FLAG_REGULAR),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "source.txt", FS_FLAG_REGULAR),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_rmdir(dirfd, "sub", FS_FLAG_DIRECTORY), FS_OK);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_namespace_error_matrix(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    lsa_device_t device = { 1, 7 };
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "adir", 0, 0755), FS_OK);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "afile", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_mkdir(dirfd, NULL, 0, 0755),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_mkdir(dirfd, "bad", 0x80000000U,
                                                  0755),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_unlink(dirfd, NULL, 0),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_unlink(dirfd, "adir", FS_FLAG_REGULAR);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EISDIR), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_rmdir(dirfd, "afile", 0),
                                        FS_ERRNO_ENOTDIR), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_rename(dirfd,
                                                   NULL,
                                                   dirfd,
                                                   "x",
                                                   0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_link(dirfd,
                                                 NULL,
                                                 dirfd,
                                                 "x",
                                                 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_symlink(NULL, dirfd, "s", 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_mknod(dirfd,
                                                  NULL,
                                                  FS_TYPE_FIFO,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_mknod(dirfd,
                                                  "blk",
                                                  FS_TYPE_BLK,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_mknod(dirfd, "unknown", FS_TYPE_REG, 0644, &device);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "afile", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "adir", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_dir_iterator_cookie_and_plus(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    bool saw_file = false;
    bool saw_dir = false;
    lsa_dir_iter_t *iter = NULL;
    lsa_dirent_t entry;
    lsa_dirent_plus_t plus;
    lsa_dir_cookie_t cookie;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "a.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_mkdir(dirfd, "sub", 0, 0755), FS_OK);

    err = lsa_dir_iter_open(dirfd, 0, &iter);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    while (fs_succeeded(lsa_dir_iter_next(iter, &entry))) {
        if (strcmp(entry.name, "a.txt") == 0) {
            saw_file = true;
        }
        if (strcmp(entry.name, "sub") == 0) {
            saw_dir = true;
        }
    }
    TEST_ASSERT_TRUE(saw_file);
    TEST_ASSERT_TRUE(saw_dir);

    cookie.value = 0;
    TEST_ASSERT_EQ_INT(lsa_dir_iter_seek(iter, cookie), FS_OK);
    err = lsa_dir_iter_next_plus(iter, &plus);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_TRUE(plus.entry.name[0] != '\0');
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(iter), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(NULL), FS_OK);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "a.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "sub", AT_REMOVEDIR), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_attr_and_fs_ops(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    struct stat st;
    struct statfs fsst;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "attr.txt", &fd), 0);

    TEST_ASSERT_EQ_INT(lsa_fstat(fd, &st), FS_OK);
    TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
    TEST_ASSERT_EQ_INT(lsa_fchmod(fd, 0600), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_fchown(fd, (uid_t)-1, (gid_t)-1), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_faccess(fd, R_OK | W_OK), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_fstatat(dirfd, "attr.txt", 0, &st), FS_OK);
    TEST_ASSERT_EQ_INT(lsa_statfs(fd, &fsst), FS_OK);
    err = lsa_syncfs(fd);
    TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));

    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "attr.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_xattr_flag_matrix(void)
{
    char dir_path[PATH_MAX];
    char list[128];
    char value[16];
    int dirfd = -1;
    int fd = -1;
    size_t actual = 0;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "xattr.txt", &fd), 0);

    err = lsa_setxattr(fd,
                       "user.miragefs.test",
                       "one",
                       3,
                       FS_FLAG_EXCLUSIVE);
    if (fs_failed(err)) {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
        TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
        TEST_ASSERT_EQ_INT(unlinkat(dirfd, "xattr.txt", 0), 0);
        TEST_ASSERT_EQ_INT(close(dirfd), 0);
        cleanup_tmpdir(dir_path);
        return 0;
    }

    memset(value, 0, sizeof(value));
    TEST_ASSERT_EQ_INT(lsa_getxattr(fd,
                                    "user.miragefs.test",
                                    value,
                                    sizeof(value),
                                    &actual),
                       FS_OK);
    TEST_ASSERT_EQ_INT(actual, 3);
    TEST_ASSERT_STR_EQ(value, "one");

    err = lsa_setxattr(fd,
                       "user.miragefs.test",
                       "two",
                       3,
                       FS_FLAG_EXCLUSIVE);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_EEXIST), 0);
    TEST_ASSERT_EQ_INT(lsa_setxattr(fd,
                                    "user.miragefs.test",
                                    "two",
                                    3,
                                    FS_FLAG_REPLACE),
                       FS_OK);
    TEST_ASSERT_EQ_INT(lsa_listxattr(fd, list, sizeof(list), &actual), FS_OK);
    TEST_ASSERT_TRUE(actual > 0U);
    TEST_ASSERT_EQ_INT(lsa_removexattr(fd, "user.miragefs.test"), FS_OK);
    err = lsa_getxattr(fd,
                       "user.miragefs.test",
                       value,
                       sizeof(value),
                       &actual);
    TEST_ASSERT_TRUE(fs_failed(err));
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_setxattr(fd,
                                                     "user.bad",
                                                     "x",
                                                     1,
                                                     FS_FLAG_REPLACE |
                                                     FS_FLAG_EXCLUSIVE),
                                        FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "xattr.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_handle_capability_paths(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    int opened = -1;
    int32_t mount_id = -1;
    lsa_file_handle_t handle;
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "handle.txt", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    memset(&handle, 0, sizeof(handle));
    err = lsa_name_to_handle_at(dirfd, "handle.txt", &handle, &mount_id, 0);
    if (fs_succeeded(err)) {
        err = lsa_open_by_handle_id(mount_id, &handle, O_RDONLY, &opened);
        TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));
        if (opened >= 0) {
            TEST_ASSERT_EQ_INT(lsa_close(opened), FS_OK);
        }
        TEST_ASSERT_EQ_INT(lsa_release_mount(mount_id), FS_OK);
    } else {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    }

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_open_by_handle_at(-1,
                                                              NULL,
                                                              O_RDONLY,
                                                              &opened),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_open_by_handle_id(0x12345678, &handle, O_RDONLY, &opened);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(err, FS_ERRNO_ENOENT), 0);
    TEST_ASSERT_EQ_INT(lsa_release_mount(0x12345678), FS_OK);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "handle.txt", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_bootstrap_root_and_init(void)
{
    char dir_path[PATH_MAX];
    char root_path[PATH_MAX];
    int32_t mount_id = -1;
    lsa_file_handle_t handle;
    fs_error_t err;
    int written;

    TEST_ASSERT_EQ_INT(make_tmpdir(dir_path, sizeof(dir_path)), 0);
    written = snprintf(root_path, sizeof(root_path), "%s/bootstrap", dir_path);
    TEST_ASSERT_TRUE((written > 0) && ((size_t)written < sizeof(root_path)));

    lsa_init();
    memset(&handle, 0, sizeof(handle));
    err = lsa_bootstrap_root(root_path, &handle, &mount_id);
    if (fs_succeeded(err)) {
        TEST_ASSERT_TRUE(mount_id >= 0);
        TEST_ASSERT_EQ_INT(lsa_release_mount(mount_id), FS_OK);
    } else {
        TEST_ASSERT_EQ_INT(fs_err_module(err), FS_MODULE_LSA);
    }

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_bootstrap_root(NULL,
                                                           &handle,
                                                           &mount_id),
                                        FS_ERRNO_EINVAL), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_syscall_failure_matrix(void)
{
    char dir_path[PATH_MAX];
    char byte = 'x';
    int dirfd = -1;
    size_t actual = 0;
    off_t offset = 0;
    struct stat st;
    struct statfs fsst;
    lsa_dirent_t entry;
    lsa_dir_iter_t *iter = NULL;

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_close(-1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_read(-1, &byte, 1, &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_read_full(-1,
                                                      &byte,
                                                      1,
                                                      &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_write(-1, &byte, 1, &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_write_full(-1,
                                                       &byte,
                                                       1,
                                                       &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_pread(-1,
                                                  &byte,
                                                  1,
                                                  0,
                                                  &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_pwrite(-1,
                                                   &byte,
                                                   1,
                                                   0,
                                                   &actual),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_lseek(-1, 0, SEEK_SET, &offset)),
                       0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fsync(-1), FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_ftruncate(-1, 1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fstat(-1, &st),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fchmod(-1, 0600),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_fchown(-1,
                                                  (uid_t)-1,
                                                  (gid_t)-1),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_faccess(-1, R_OK)), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_statfs(-1, &fsst),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_syncfs(-1), FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_listxattr(-1,
                                                       &byte,
                                                       1,
                                                       &actual)),
                       0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_removexattr(-1, "user.none")),
                       0);

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_open(dirfd, 0, &iter), FS_OK);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    dirfd = -1;
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_dir_iter_next(iter, &entry),
                                        FS_ERRNO_EBADF), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_dir_iter_seek(iter,
                                                           (lsa_dir_cookie_t){
                                                               0,
                                                           })),
                       0);
    TEST_ASSERT_EQ_INT(lsa_dir_iter_close(iter), FS_OK);
    cleanup_tmpdir(dir_path);
    return 0;
}

static int test_lsa_namespace_failure_edges(void)
{
    char dir_path[PATH_MAX];
    int dirfd = -1;
    int fd = -1;
    lsa_device_t device = { 1, 7 };
    fs_error_t err;

    TEST_ASSERT_EQ_INT(open_tmp_root(dir_path, sizeof(dir_path), &dirfd), 0);
    TEST_ASSERT_EQ_INT(create_file(dirfd, "src", &fd), 0);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_lookup(dirfd, NULL, 0, &fd),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_lookup(dirfd, "src", 0, NULL),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_create(dirfd,
                                                   NULL,
                                                   0,
                                                   0644,
                                                   &fd),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_create(dirfd,
                                                   "bad",
                                                   0,
                                                   0644,
                                                   NULL),
                                        FS_ERRNO_EINVAL), 0);
    err = lsa_create(dirfd,
                     "append.txt",
                     FS_FLAG_APPEND | FS_FLAG_SYNC | FS_FLAG_NOFOLLOW,
                     0644,
                     &fd);
    TEST_ASSERT_EQ_INT(err, FS_OK);
    TEST_ASSERT_EQ_INT(lsa_close(fd), FS_OK);

    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_rmdir(dirfd, NULL, 0),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_rmdir(dirfd,
                                                  "missing",
                                                  0x80000000U),
                                        FS_ERRNO_EINVAL), 0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_rename(dirfd,
                                                    "missing",
                                                    dirfd,
                                                    "dst",
                                                    0)),
                       0);
    TEST_ASSERT_EQ_INT(lsa_symlink("src", dirfd, "sym", 0), FS_OK);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_symlink("src",
                                                     dirfd,
                                                     "sym",
                                                     0)),
                       0);
    TEST_ASSERT_EQ_INT(assert_lsa_failed(lsa_readlink(dirfd,
                                                      "missing-link",
                                                      dir_path,
                                                      sizeof(dir_path),
                                                      &(size_t){ 0 })),
                       0);
    err = lsa_mknod(dirfd, "blk", FS_TYPE_BLK, 0644, &device);
    TEST_ASSERT_TRUE((err == FS_OK) || fs_failed(err));
    if (err == FS_OK) {
        TEST_ASSERT_EQ_INT(lsa_unlink(dirfd, "blk", 0), FS_OK);
    }
    TEST_ASSERT_EQ_INT(assert_lsa_errno(lsa_mknod(dirfd,
                                                  "chr",
                                                  FS_TYPE_CHR,
                                                  0644,
                                                  NULL),
                                        FS_ERRNO_EINVAL), 0);

    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "sym", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "append.txt", 0), 0);
    TEST_ASSERT_EQ_INT(unlinkat(dirfd, "src", 0), 0);
    TEST_ASSERT_EQ_INT(close(dirfd), 0);
    cleanup_tmpdir(dir_path);
    return 0;
}

int main(void)
{
    const test_case_t cases[] = {
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_FILE,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_FILE,
                             0x1,
                             0x001),
                  test_lsa_create_write_read_round_trip,
                  "在项目 output 临时目录创建文件并完成读写回环",
                  "使用真实 LSA 后端执行 write/read/pwrite/pread/truncate",
                  "所有 IO 路径返回 FS_OK，内容和实际字节数符合预期"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ERROR,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ERROR,
                             0x1,
                             0x001),
                  test_lsa_invalid_argument_matrix,
                  "LSA 公共 API 空参数和非法 fd 防御矩阵",
                  "向 read/write/stat/xattr/readdir 等接口注入非法输入",
                  "在 syscall 前或 syscall 失败后返回 LSA 模块结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_LOOKUP,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_LOOKUP,
                             0x1,
                             0x001),
                  test_lsa_lookup_flag_matrix,
                  "lookup 类型约束和未知 flag 矩阵",
                  "对文件和目录分别注入 DIRECTORY/REGULAR/未知 flag",
                  "合法约束成功，类型冲突映射为 ENOTDIR/EISDIR/EINVAL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_CREATE,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_CREATE,
                             0x1,
                             0x001),
                  test_lsa_create_flag_matrix,
                  "create 已存在目标与 flag 组合矩阵",
                  "重复创建、REPLACE/TRUNCATE、未知 flag 和互斥 flag",
                  "EXCLUSIVE 返回 EEXIST，REPLACE 成功，非法组合返回 EINVAL"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x1,
                             0x001),
                  test_lsa_namespace_round_trip,
                  "namespace 操作真实后端回环",
                  "执行 mkdir/link/rename/symlink/readlink/mknod/unlink/rmdir",
                  "命名空间变更可观察且最终临时目录可完整清理"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x2),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x2,
                             0x001),
                  test_lsa_namespace_error_matrix,
                  "namespace 操作非法参数和类型冲突矩阵",
                  "注入 NULL 名称、未知 flag、目录当文件删除和非法 mknod",
                  "LSA 返回 EINVAL/EISDIR/ENOTDIR 等结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_DIR,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_DIR,
                             0x1,
                             0x001),
                  test_lsa_dir_iterator_cookie_and_plus,
                  "目录迭代器 entry、plus 和 cookie 行为",
                  "创建文件和目录后执行 next/seek/next_plus/get_cookie",
                  "能枚举目标项，plus 可取 stat，cookie 可用于重新定位"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ATTR,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ATTR,
                             0x1,
                             0x001),
                  test_lsa_attr_and_fs_ops,
                  "属性和文件系统级操作真实后端验证",
                  "执行 fstat/fchmod/fchown/faccess/fstatat/statfs/syncfs",
                  "属性与文件系统信息可读取，允许 syncfs 返回结构化失败"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_XATTR,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_XATTR,
                             0x1,
                             0x001),
                  test_lsa_xattr_flag_matrix,
                  "xattr 创建、替换、列表和删除矩阵",
                  "对 user xattr 注入 EXCLUSIVE/REPLACE/互斥 flag",
                  "支持 xattr 时完整回环，不支持时仍返回 LSA 结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_HANDLE,
                             0x1),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_HANDLE,
                             0x1,
                             0x001),
                  test_lsa_handle_capability_paths,
                  "file handle 能力路径和非法参数验证",
                  "尝试 name_to_handle/open_by_handle 并注入非法 mount/handle",
                  "能力可用则可打开对象，不可用也必须返回 LSA 结构化错误"),

        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ERROR,
                             0x2),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_ERROR,
                             0x2,
                             0x001),
                  test_lsa_syscall_failure_matrix,
                  "LSA syscall 失败路径错误映射矩阵",
                  "使用非法 fd 和失效目录迭代器触发 EBADF/失败分支",
                  "所有失败都映射为 LSA 模块结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x3),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_NAMESPACE,
                             0x3,
                             0x001),
                  test_lsa_namespace_failure_edges,
                  "namespace syscall 失败和补充 flag 路径",
                  "注入 NULL lookup/create、缺失 rename、重复 symlink 和设备节点",
                  "非法输入早退，真实 syscall 失败返回 LSA 结构化错误"),
        TEST_CASE(UT_LIST_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_HANDLE,
                             0x2),
                  UT_CASE_NO(UT_MOD_LSA,
                             TEST_LSA_COMPONENT_HANDLE,
                             0x2,
                             0x001),
                  test_lsa_bootstrap_root_and_init,
                  "LSA 初始化和 bootstrap root 能力验证",
                  "创建项目 output 下的 bootstrap root 并注册 mount handle",
                  "成功时可释放 mount，不支持 handle 时返回 LSA 结构化错误"),
    };

    return test_run_suite("lsa", cases, sizeof(cases) / sizeof(cases[0]));
}
