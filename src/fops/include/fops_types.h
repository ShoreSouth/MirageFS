#pragma once

#include <stdint.h>
#include <sys/statfs.h>
#include <sys/types.h>

#include "common/fs_common.h"
#include "object/fuid/fuid.h"
#include "object/objmeta/objmeta.h"

/* FOPS 打开的文件句柄。对上层不暴露 Linux fd。 */
typedef struct fops_file fops_file_t;

#define FOPS_READDIR_MAX_BATCH 1024U

#define FOPS_CREATE_ATTR_MODE  (1U << 0)
#define FOPS_CREATE_ATTR_UID   (1U << 1)
#define FOPS_CREATE_ATTR_GID   (1U << 2)
#define FOPS_CREATE_ATTR_SIZE  (1U << 3)

/*
 * fops_attr_t
 *
 * FOPS 返回的稳定属性快照。该结构只作为输出值使用，不作为 create/setattr
 * 请求。nlink、ctime 等字段由后端文件系统维护，调用方不应主动填写。
 */
typedef struct fops_attr {

    fs_type_t type;      /* 对象类型 */
    mode_t mode;         /* 完整 Linux mode 位 */
    uid_t uid;           /* owner uid */
    gid_t gid;           /* owner gid */

    uint64_t size;       /* 字节大小 */
    uint64_t nlink;      /* 硬链接计数 */

    uint64_t atime_sec;  /* 访问时间，秒 */
    uint64_t mtime_sec;  /* 修改时间，秒 */
    uint64_t ctime_sec;  /* 状态变更时间，秒 */

} fops_attr_t;

/*
 * fops_create_attr_t
 *
 * 创建类操作的属性请求。valid_mask 决定哪些字段有效；目标 OP 不支持的
 * mask 位会被拒绝。
 */
typedef struct fops_create_attr {

    uint32_t valid_mask; /* FOPS_CREATE_ATTR_* */

    mode_t mode;         /* FOPS_CREATE_ATTR_MODE 有效时使用 */
    uid_t uid;           /* FOPS_CREATE_ATTR_UID 有效时使用 */
    gid_t gid;           /* FOPS_CREATE_ATTR_GID 有效时使用 */
    uint64_t size;       /* 仅 regular file create 支持 */

} fops_create_attr_t;

#define FOPS_SETATTR_MODE  FOPS_CREATE_ATTR_MODE
#define FOPS_SETATTR_UID   FOPS_CREATE_ATTR_UID
#define FOPS_SETATTR_GID   FOPS_CREATE_ATTR_GID
#define FOPS_SETATTR_SIZE  FOPS_CREATE_ATTR_SIZE

/* 属性修改请求。valid_mask 决定哪些字段有效。 */
typedef struct fops_setattr {

    uint32_t valid_mask; /* FOPS_SETATTR_* */

    mode_t mode;         /* FOPS_SETATTR_MODE 有效时使用 */
    uid_t uid;           /* FOPS_SETATTR_UID 有效时使用 */
    gid_t gid;           /* FOPS_SETATTR_GID 有效时使用 */
    uint64_t size;       /* FOPS_SETATTR_SIZE 有效时使用 */

} fops_setattr_t;

/* mknod 使用的设备号。仅 block/character device 有效。 */
typedef struct fops_device {

    uint32_t major_id;
    uint32_t minor_id;

} fops_device_t;

/*
 * mknod 创建请求。
 *
 * 参数：
 *      [IN] parent_fuid : 父目录 FUID
 *      [IN] name        : 待创建的单级名称
 *      [IN] type        : 待创建对象类型
 *      [IN] attr        : 创建属性，可为 NULL
 *      [IN] device      : 设备号，仅 block/character device 使用
 *      [IN] flags       : 本次创建标志
 */
typedef struct fops_mknod_req {

    const fuid_t *parent_fuid;
    const char *name;
    fs_type_t type;
    const fops_create_attr_t *attr;
    const fops_device_t *device;
    fs_flags_t flags;

} fops_mknod_req_t;

typedef struct statfs fops_statfs_t;

/* 不带属性的目录项。 */
typedef struct fops_dirent {

    char name[FS_MAX_NAME_LEN + 1U];
    fuid_t fuid;

} fops_dirent_t;

/* 带属性的目录项。 */
typedef struct fops_dirent_plus {

    fops_dirent_t entry;
    fops_attr_t attr;

} fops_dirent_plus_t;

/* lookup/create/link 等 plus 接口的通用返回值。 */
typedef struct fops_object_result {

    fuid_t fuid;
    fops_attr_t attr;

} fops_object_result_t;

/*
 * fops_args_t
 *
 * FOPS 统一调度入口的参数对象。公共区只放高频且语义稳定的字段；
 * 每个 OP 的特有参数放入 union，避免函数参数持续膨胀。
 */
typedef struct fops_args {

    fs_op_t op;                 /* [IN] 目标操作字 */
    fs_flags_t flags;           /* [IN] 本次操作的 FS_FLAG_* */

    const fuid_t *fuid;         /* [IN] 对象级 OP 的目标 FUID */
    const fuid_t *parent_fuid;  /* [IN] 命名类 OP 的父目录 FUID */
    const char *name;           /* [IN] 单级路径分量或 xattr 名称 */

    union {
        struct {
            fops_object_result_t *out;
        } lookup;

        struct {
            const fops_create_attr_t *attr;
            fops_object_result_t *out;
        } create;

        struct {
            const fops_create_attr_t *attr;
            fops_object_result_t *out;
        } mkdir;

        struct {
            fs_type_t type;
            const fops_create_attr_t *attr;
            const fops_device_t *device;
            fops_object_result_t *out;
        } mknod;

        struct {
            const fuid_t *new_parent_fuid;
            const char *new_name;
        } rename;

        struct {
            const fuid_t *new_parent_fuid;
            const char *new_name;
            fops_object_result_t *out;
        } link;

        struct {
            const char *target;
            fops_object_result_t *out;
        } symlink;

        struct {
            fops_attr_t *out_attr;
        } getattr;

        struct {
            const fops_setattr_t *attr;
        } setattr;

        struct {
            fops_dirent_t *entries;
            uint32_t entry_cap;
            uint32_t *out_entry_nr;
            bool *out_eof;
        } readdir;

        struct {
            fops_dirent_plus_t *entries;
            uint32_t entry_cap;
            uint32_t *out_entry_nr;
            bool *out_eof;
        } readdirplus;

        struct {
            int mask;
        } access;

        struct {
            obj_handle_t *out_handle;
        } gethandle;

        struct {
            fops_file_t **out_file;
        } open;

        struct {
            const obj_handle_t *handle;
            fops_file_t **out_file;
        } openhandle;

        struct {
            fops_file_t *file;
        } close;

        struct {
            fops_file_t *file;
            void *buf;
            size_t size;
            size_t *actual;
        } read;

        struct {
            fops_file_t *file;
            const void *buf;
            size_t size;
            size_t *actual;
        } write;

        struct {
            fops_file_t *file;
            void *buf;
            size_t size;
            off_t offset;
            size_t *actual;
        } pread;

        struct {
            fops_file_t *file;
            const void *buf;
            size_t size;
            off_t offset;
            size_t *actual;
        } pwrite;

        struct {
            uint64_t size;
        } truncate;

        struct {
            void *value;
            size_t size;
            size_t *actual;
        } getxattr;

        struct {
            const void *value;
            size_t size;
        } setxattr;

        struct {
            char *list;
            size_t size;
            size_t *actual;
        } listxattr;

        struct {
            fops_statfs_t *out_statfs;
        } statfs;
    } u;

} fops_args_t;
