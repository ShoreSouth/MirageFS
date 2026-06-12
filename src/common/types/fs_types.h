#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ============================================================
 *  基础语义类型
 * ============================================================ */

typedef uint64_t Fsid_t;
typedef uint64_t ObjectId_t;
typedef uint32_t GenId_t;

typedef uint32_t QtreeId_t;
typedef uint32_t SnapId_t;
typedef uint32_t ShardId_t;

/* ============================================================
 *  文件类型
 * ============================================================ */

typedef enum {
    FS_TYPE_UNKNOWN = 0,

    FS_TYPE_REG,
    FS_TYPE_DIR,
    FS_TYPE_LNK,
    FS_TYPE_FIFO,
    FS_TYPE_SOCK, /* lookup/stat only, not supported by lsa_mknod() */
    FS_TYPE_BLK,
    FS_TYPE_CHR,

} fs_type_t;

/* ============================================================
 *  mode 抽象（兼容 Linux）
 * ============================================================ */

typedef mode_t fs_mode_t;

/* ---------- 文件类型判断（封装 S_ISxxx） ---------- */

#define FS_IS_REG(m)   S_ISREG(m)
#define FS_IS_DIR(m)   S_ISDIR(m)
#define FS_IS_LNK(m)   S_ISLNK(m)
#define FS_IS_FIFO(m)  S_ISFIFO(m)
#define FS_IS_SOCK(m)  S_ISSOCK(m)
#define FS_IS_BLK(m)   S_ISBLK(m)
#define FS_IS_CHR(m)   S_ISCHR(m)

/* ---------- 提取类型位 ---------- */

#define FS_MODE_TYPE(m)   ((m) & S_IFMT)

/* ---------- 权限位 ---------- */

#define FS_PERM_MASK   07777

#define FS_PERM(m)     ((m) & FS_PERM_MASK)

/* 用户权限 */
#define FS_IRUSR S_IRUSR
#define FS_IWUSR S_IWUSR
#define FS_IXUSR S_IXUSR

/* 组权限 */
#define FS_IRGRP S_IRGRP
#define FS_IWGRP S_IWGRP
#define FS_IXGRP S_IXGRP

/* 其他权限 */
#define FS_IROTH S_IROTH
#define FS_IWOTH S_IWOTH
#define FS_IXOTH S_IXOTH

/* 特殊权限 */
#define FS_ISUID S_ISUID
#define FS_ISGID S_ISGID
#define FS_ISVTX S_ISVTX


/* ============================================================
 *  fs_type_t <-> mode 转换
 * ============================================================ */

static inline fs_type_t fs_type_from_mode(fs_mode_t mode)
{
    if (S_ISREG(mode))  return FS_TYPE_REG;
    if (S_ISDIR(mode))  return FS_TYPE_DIR;
    if (S_ISLNK(mode))  return FS_TYPE_LNK;
    if (S_ISFIFO(mode)) return FS_TYPE_FIFO;
    if (S_ISSOCK(mode)) return FS_TYPE_SOCK;
    if (S_ISBLK(mode))  return FS_TYPE_BLK;
    if (S_ISCHR(mode))  return FS_TYPE_CHR;

    return FS_TYPE_UNKNOWN;
}

/* ============================================================
 *  常用组合
 * ============================================================ */

/* 默认文件权限 */
#define FS_MODE_FILE_DEFAULT  (S_IFREG | 0644)

/* 默认目录权限 */
#define FS_MODE_DIR_DEFAULT   (S_IFDIR | 0755)

/* ============================================================
 *  Debug / 打印辅助
 * ============================================================ */

static inline const char* fs_type_to_str(fs_type_t type)
{
    switch (type) {
        case FS_TYPE_REG:  return "REG";
        case FS_TYPE_DIR:  return "DIR";
        case FS_TYPE_LNK:  return "LNK";
        case FS_TYPE_FIFO: return "FIFO";
        case FS_TYPE_SOCK: return "SOCK";
        case FS_TYPE_BLK:  return "BLK";
        case FS_TYPE_CHR:  return "CHR";
        default:           return "UNKNOWN";
    }
}