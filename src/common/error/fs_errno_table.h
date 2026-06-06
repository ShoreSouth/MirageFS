#pragma once

/*
 * ============================================================
 * errno table
 *
 * _(errno_name, description)
 *
 * 顺序与 fs_errno.h 保持一致
 * ============================================================
 */

#define FS_ERRNO_TABLE(_)                                                   \
                                                                            \
    /* ======================================================== */          \
    /* 参数错误                                                  */          \
    /* ======================================================== */          \
                                                                            \
    _(EINVAL,       "Invalid argument")                                     \
    _(ERANGE,       "Result too large")                                     \
    _(EFAULT,       "Bad address")                                          \
                                                                            \
    /* ======================================================== */          \
    /* 文件/目录                                                 */          \
    /* ======================================================== */          \
                                                                            \
    _(ENOENT,       "No such file or directory")                            \
    _(EEXIST,       "File exists")                                          \
    _(ENOTDIR,      "Not a directory")                                      \
    _(EISDIR,       "Is a directory")                                       \
    _(ENOTEMPTY,    "Directory not empty")                                  \
    _(ENAMETOOLONG, "File name too long")                                   \
    _(ELOOP,        "Too many symbolic links")                              \
    _(EXDEV,        "Cross-device link")                                    \
                                                                            \
    /* ======================================================== */          \
    /* 权限                                                     */          \
    /* ======================================================== */          \
                                                                            \
    _(EPERM,        "Operation not permitted")                              \
    _(EACCES,       "Permission denied")                                    \
    _(EROFS,        "Read-only file system")                                \
                                                                            \
    /* ======================================================== */          \
    /* 内存                                                     */          \
    /* ======================================================== */          \
                                                                            \
    _(ENOMEM,       "Out of memory")                                        \
                                                                            \
    /* ======================================================== */          \
    /* IO                                                       */          \
    /* ======================================================== */          \
                                                                            \
    _(EIO,          "I/O error")                                            \
    _(ENOSPC,       "No space left on device")                              \
    _(EFBIG,        "File too large")                                       \
    _(EPIPE,        "Broken pipe")                                          \
    _(ESPIPE,       "Illegal seek")                                         \
                                                                            \
    /* ======================================================== */          \
    /* FD / Resource                                            */          \
    /* ======================================================== */          \
                                                                            \
    _(EBADF,        "Bad file descriptor")                                  \
    _(EBUSY,        "Device or resource busy")                              \
    _(EMFILE,       "Too many open files")                                  \
    _(ENFILE,       "File table overflow")                                  \
                                                                            \
    /* ======================================================== */          \
    /* 同步 / 重试                                               */          \
    /* ======================================================== */          \
                                                                            \
    _(EAGAIN,       "Try again")                                            \
    _(EINTR,        "Interrupted system call")                              \
    _(ETIMEDOUT,    "Connection timed out")                                 \
    _(EDEADLK,      "Resource deadlock avoided")