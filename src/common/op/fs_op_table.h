#pragma once

/*
 * ============================================================
 * operation table
 *
 * _(enum_name, string_name)
 *
 * ============================================================
 */

#define FS_OP_TABLE(_)                 \
                                       \
    _(NONE,         "NONE")            \
                                       \
    _(LOOKUP,       "LOOKUP")          \
                                       \
    _(CREATE,       "CREATE")          \
    _(MKDIR,        "MKDIR")           \
    _(MKNOD,        "MKNOD")           \
                                       \
    _(UNLINK,       "UNLINK")          \
    _(RMDIR,        "RMDIR")           \
                                       \
    _(RENAME,       "RENAME")          \
                                       \
    _(LINK,         "LINK")            \
    _(SYMLINK,      "SYMLINK")         \
                                       \
    _(READDIR,      "READDIR")         \
    _(READDIRPLUS,  "READDIRPLUS")     \
                                       \
    _(GETATTR,      "GETATTR")         \
    _(SETATTR,      "SETATTR")         \
                                       \
    _(TRUNCATE,     "TRUNCATE")        \
                                       \
    _(GETXATTR,     "GETXATTR")        \
    _(SETXATTR,     "SETXATTR")        \
    _(LISTXATTR,    "LISTXATTR")       \
    _(REMOVEXATTR,  "REMOVEXATTR")     \
                                       \
    _(ACCESS,       "ACCESS")          \
                                       \
    _(READ,         "READ")            \
    _(WRITE,        "WRITE")
