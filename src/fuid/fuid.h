#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * 基础类型定义
 * ============================================================ */

typedef int fuid_ret_t;

typedef uint64_t Fsid_t;
typedef uint64_t ObjectId_t;
typedef uint32_t GenId_t;

typedef uint32_t QtreeId_t;
typedef uint32_t SnapId_t;
typedef uint32_t ShardId_t;

/* ============================================================
 * 文件类型（统一抽象）
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
 * File Unique Identity（MirageFS核心结构）
 * ============================================================ */

typedef struct Fuid_t {
    Fsid_t           fsid; /* 文件系统ID */

    ObjectId_t   objectid; /* MirageFS 内部对象唯一标识 */
    GenId_t           gen; /* generation, 用于检测 stale handle */

    QtreeId_t     qtreeid; /* 用于目录树级别的配额/隔离管理, 当前阶段固定为0 */
    SnapId_t       snapid; /* 快照ID, 当前阶段固定为0 */
    ShardId_t     shardid; /* 分片ID, 用于未来大目录/大文件分片, 当前阶段固定为0 */

    uint16_t         type; /* 对象类型, fuid_type_t */
    uint16_t        flags; /* 标志位, compressed/encrypted/clone... */

    uint8_t       version; /* FUID 结构版本, 用于未来结构扩展兼容 */
    uint8_t reserved0[11]; /* 保留字段 */
    uint64_t reserved1[2]; /* 保留字段 */
} Fuid_t;

/* version */
#define FUID_CURRENT_VERSION 1

/* invalid */
#define FUID_INVALID_OBJECTID ((ObjectId)0)

/* flags */
#define FUID_FLAG_NONE        0x00000000
#define FUID_FLAG_COMPRESSED  0x00000001
#define FUID_FLAG_ENCRYPTED   0x00000002
#define FUID_FLAG_CLONED      0x00000004

bool fuid_is_valid(const Fuid_t *fuid);

void fuid_set_invalid(Fuid_t *fuid);

bool fuid_equal(const Fuid_t *a, const Fuid_t *b);

uint64_t fuid_hash(const Fuid_t *fuid);

void fuid_init(Fuid_t *fuid);

Fuid_t fuid_build(Fsid_t fsid, ObjectId_t objectid,
    GenId_t gen, fuid_type_t type);

const char * fuid_type_str(fuid_type_t type);
