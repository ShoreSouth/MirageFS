#include "object/fuid/fuid.h"

#include <stdio.h>
#include <string.h>

/* ============================================================
 * 基础接口
 * ============================================================ */

bool fuid_is_valid(const fuid_t *fuid)
{
    if (fuid == NULL) {
        return false;
    }

    if (fuid->version != FUID_CURRENT_VERSION) {
        return false;
    }

    if (fuid->fsid == FUID_INVALID_FSID) {
        return false;
    }

    if (fuid->objectid == FUID_INVALID_OBJECTID) {
        return false;
    }

    return true;
}

void fuid_set_invalid(fuid_t *fuid)
{
    if (fuid == NULL) {
        return;
    }

    memset(fuid, 0, sizeof(fuid_t));
}

void fuid_init(fuid_t *fuid)
{
    if (fuid == NULL) {
        return;
    }

    memset(fuid, 0, sizeof(fuid_t));

    fuid->version = FUID_CURRENT_VERSION;
}

bool fuid_equal(const fuid_t *a, const fuid_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }

    return (
        a->fsid     == b->fsid     &&
        a->objectid == b->objectid &&
        a->gen      == b->gen
    );
}

uint64_t fuid_hash(const fuid_t *fuid)
{
    uint64_t h;

    if (fuid == NULL) {
        return 0;
    }

    h = fuid->fsid;

    h ^= (fuid->objectid + 0x9e3779b97f4a7c15ULL +
          (h << 6) + (h >> 2));

    h ^= (fuid->gen + 0x9e3779b97f4a7c15ULL +
          (h << 6) + (h >> 2));

    return h;
}

fuid_t fuid_build(Fsid_t fsid,
                ObjectId_t objectid,
                GenId_t gen,
                fuid_type_t type)
{
    fuid_t fuid;

    memset(&fuid, 0, sizeof(fuid_t));

    fuid.fsid     = fsid;
    fuid.objectid = objectid;
    fuid.gen      = gen;

    fuid.type     = (uint8_t)type;
    fuid.version  = FUID_CURRENT_VERSION;

    return fuid;
}

/* ============================================================
 * type
 * ============================================================ */

bool fuid_type_valid(fuid_type_t type)
{
    switch (type) {

    case FUID_TYPE_FILE:
    case FUID_TYPE_DIR:
    case FUID_TYPE_SYMLINK:
    case FUID_TYPE_FIFO:
    case FUID_TYPE_SOCK:
    case FUID_TYPE_BLK:
    case FUID_TYPE_CHR:
        return true;

    default:
        return false;
    }
}

fuid_type_t fuid_get_type(const fuid_t *fuid)
{
    if (fuid == NULL) {
        return FUID_TYPE_INVALID;
    }

    return (fuid_type_t)fuid->type;
}

const char *fuid_type_str(fuid_type_t type)
{
    switch (type) {

    case FUID_TYPE_FILE:
        return "file";

    case FUID_TYPE_DIR:
        return "dir";

    case FUID_TYPE_SYMLINK:
        return "symlink";

    case FUID_TYPE_FIFO:
        return "fifo";

    case FUID_TYPE_SOCK:
        return "sock";

    case FUID_TYPE_BLK:
        return "blk";

    case FUID_TYPE_CHR:
        return "chr";

    default:
        return "invalid";
    }
}

/* ============================================================
 * type helper
 * ============================================================ */

bool fuid_is_file(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_FILE
    );
}

bool fuid_is_dir(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_DIR
    );
}

bool fuid_is_symlink(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_SYMLINK
    );
}

bool fuid_is_fifo(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_FIFO
    );
}

bool fuid_is_sock(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_SOCK
    );
}

bool fuid_is_blk(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_BLK
    );
}

bool fuid_is_chr(const fuid_t *fuid)
{
    return (
        fuid_get_type(fuid) == FUID_TYPE_CHR
    );
}

/* ============================================================
 * flags
 * ============================================================ */

bool fuid_flag_test(const fuid_t *fuid, uint16_t flag)
{
    if (fuid == NULL) {
        return false;
    }

    return (
        (fuid->flags & flag) != 0
    );
}

void fuid_flag_set(fuid_t *fuid, uint16_t flag)
{
    if (fuid == NULL) {
        return;
    }

    fuid->flags |= flag;
}

void fuid_flag_clear(fuid_t *fuid, uint16_t flag)
{
    if (fuid == NULL) {
        return;
    }

    fuid->flags &= ~flag;
}

/* ============================================================
 * debug
 * ============================================================ */

const char *fuid_to_str(const fuid_t *fuid)
{
    static __thread char buf[256];

    if (fuid == NULL) {
        return "null";
    }

    snprintf(buf,
        sizeof(buf),
        "fs=%lu,obj=%lu,gen=%u,type=%s",
        (unsigned long)fuid->fsid,
        (unsigned long)fuid->objectid,
        (unsigned int)fuid->gen,
        fuid_type_str((fuid_type_t)fuid->type));

    return buf;
}
