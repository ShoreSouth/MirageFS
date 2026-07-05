#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "common/fs_common.h"

/* ============================================================
 * 基础类型
 * ============================================================ */

typedef int fuid_ret_t;

typedef uint64_t Fsid_t;
typedef uint64_t ObjectId_t;
typedef uint32_t GenId_t;

typedef uint32_t QtreeId_t;
typedef uint32_t SnapId_t;
typedef uint32_t ShardId_t;

/* ============================================================
 * 文件类型
 * ============================================================ */

typedef enum {

    FUID_TYPE_INVALID = 0,

    FUID_TYPE_FILE,
    FUID_TYPE_DIR,
    FUID_TYPE_SYMLINK,

    FUID_TYPE_FIFO,
    FUID_TYPE_SOCK,

    FUID_TYPE_BLK,
    FUID_TYPE_CHR,

} fuid_type_t;

/* ============================================================
 * FUID flags
 * ============================================================ */

#define FUID_FLAG_NONE        ((uint16_t)0x0000)

#define FUID_FLAG_COMPRESSED  ((uint16_t)0x0001)
#define FUID_FLAG_ENCRYPTED   ((uint16_t)0x0002)
#define FUID_FLAG_CLONED      ((uint16_t)0x0004)

/* ============================================================
 * 常量定义
 * ============================================================ */

#define FUID_CURRENT_VERSION  1

#define FUID_INVALID_FSID      ((Fsid_t)0)
#define FUID_INVALID_OBJECTID  ((ObjectId_t)0)

/* ============================================================
 * File Unique Identity
 *
 * MirageFS 核心对象标识:
 *
 * Identity:
 *     (fsid, objectid, gen)
 *
 * View:
 *     (qtreeid, snapid, shardid)
 *
 * 特点:
 *     - 文件系统范围内唯一
 *     - 固定长度
 *     - 可序列化
 *     - 支持 stale 检测
 * ============================================================ */

typedef struct fuid {

    /* ---------- identity ---------- */

    Fsid_t         fsid;      /* 文件系统ID */

    ObjectId_t objectid;      /* 对象唯一ID */
    GenId_t         gen;      /* generation */

    /* ---------- view ---------- */

    QtreeId_t   qtreeid;      /* qtree / tenant */
    SnapId_t     snapid;      /* snapshot */
    ShardId_t   shardid;      /* shard */

    /* ---------- attributes ---------- */

    uint8_t         type;     /* fuid_type_t */
    uint8_t      version;     /* 结构版本 */

    uint16_t       flags;     /* FUID_FLAG_* */

    /* ---------- reserved ---------- */

    uint8_t  reserved0[12];
    uint64_t reserved1[2];

} fuid_t;

/* ============================================================
 * 编译期检查
 * ============================================================ */

#define FUID_SIZE 64

_Static_assert(sizeof(fuid_t) == FUID_SIZE,
    "fuid_t size invalid");

/* ============================================================
 * 基础接口
 * ============================================================ */

/* 是否有效 */
bool fuid_is_valid(const fuid_t *fuid);

/* 设置为 invalid */
void fuid_set_invalid(fuid_t *fuid);

/* 初始化 */
void fuid_init(fuid_t *fuid);

/* identity 比较 */
bool fuid_equal(const fuid_t *a, const fuid_t *b);

/* hash */
uint64_t fuid_hash(const fuid_t *fuid);

/* 构造 */
fuid_t fuid_make(Fsid_t fsid,
    ObjectId_t objectid,
    GenId_t gen,
    fuid_type_t type);

/* 类型字符串 */
const char *fuid_type_str(fuid_type_t type);

/* ============================================================
 * type
 * ============================================================ */

bool fuid_type_valid(fuid_type_t type);

fuid_type_t fuid_get_type(const fuid_t *fuid);

/* ============================================================
 * type helper
 * ============================================================ */

bool fuid_is_file(const fuid_t *fuid);

bool fuid_is_dir(const fuid_t *fuid);

bool fuid_is_symlink(const fuid_t *fuid);

bool fuid_is_fifo(const fuid_t *fuid);

bool fuid_is_sock(const fuid_t *fuid);

bool fuid_is_blk(const fuid_t *fuid);

bool fuid_is_chr(const fuid_t *fuid);

/* ============================================================
 * flags helper
 * ============================================================ */

bool fuid_flag_test(const fuid_t *fuid, uint16_t flag);

void fuid_flag_set(fuid_t *fuid, uint16_t flag);

void fuid_flag_clear(fuid_t *fuid, uint16_t flag);

/* ============================================================
 * debug
 * ============================================================ */

/*
 * 输出格式:
 *
 * fs=1,obj=100,gen=1,type=file
 */
const char *fuid_to_str(const fuid_t *fuid);
