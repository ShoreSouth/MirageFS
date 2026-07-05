#include "object/fuid/fuid.h"

#include <stdio.h>
#include <string.h>

/* ============================================================
 * 基础接口
 * ============================================================ */

bool fuid_is_valid(const fuid_t *fuid)
{
    bool valid;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    if (fuid == NULL) {
        valid = false;
        goto out;
    }

    if (fuid->version != FUID_CURRENT_VERSION) {
        valid = false;
        goto out;
    }

    if (fuid->fsid == FUID_INVALID_FSID) {
        valid = false;
        goto out;
    }

    if (fuid->objectid == FUID_INVALID_OBJECTID) {
        valid = false;
        goto out;
    }

    if (!fuid_type_valid((fuid_type_t)fuid->type)) {
        valid = false;
        goto out;
    }

    valid = true;

out:
    FS_LOG_DUMP_INFO("exit: %s",
                     valid ? "true" : "false");
    return valid;
}

void fuid_set_invalid(fuid_t *fuid)
{
    FS_LOG_DUMP_INFO("enter: fuid=%p", (void *)fuid);

    if (fuid == NULL) {
        return;
    }

    memset(fuid, 0, sizeof(fuid_t));

    FS_LOG_DUMP_INFO("exit: done");
}

void fuid_init(fuid_t *fuid)
{
    FS_LOG_DUMP_INFO("enter: fuid=%p", (void *)fuid);

    if (fuid == NULL) {
        return;
    }

    memset(fuid, 0, sizeof(fuid_t));

    fuid->version = FUID_CURRENT_VERSION;

    FS_LOG_DUMP_INFO("exit: done");
}

bool fuid_equal(const fuid_t *a, const fuid_t *b)
{
    bool equal;

    FS_LOG_DUMP_INFO("enter: a=%p, b=%p",
                     (const void *)a, (const void *)b);

    if (a == NULL || b == NULL) {
        equal = false;
        goto out;
    }

    equal = (
        a->fsid     == b->fsid     &&
        a->objectid == b->objectid &&
        a->gen      == b->gen
    );

out:
    FS_LOG_DUMP_INFO("exit: %s",
                     equal ? "true" : "false");
    return equal;
}

uint64_t fuid_hash(const fuid_t *fuid)
{
    uint64_t h;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    if (fuid == NULL) {
        h = 0;
        goto out;
    }

    h = fuid->fsid;

    h ^= (fuid->objectid + 0x9e3779b97f4a7c15ULL +
          (h << 6) + (h >> 2));

    h ^= (fuid->gen + 0x9e3779b97f4a7c15ULL +
          (h << 6) + (h >> 2));

out:
    FS_LOG_DUMP_INFO("exit: hash=0x%lx", (unsigned long)h);
    return h;
}

fuid_t fuid_make(Fsid_t fsid,
                 ObjectId_t objectid,
                 GenId_t gen,
                 fuid_type_t type)
{
    fuid_t fuid;

    FS_LOG_DUMP_INFO("enter: fsid=%lu, objectid=%lu, gen=%u, type=%u",
                     (unsigned long)fsid,
                     (unsigned long)objectid,
                     (unsigned int)gen,
                     (unsigned int)type);

    memset(&fuid, 0, sizeof(fuid_t));

    fuid.fsid     = fsid;
    fuid.objectid = objectid;
    fuid.gen      = gen;

    fuid.type     = (uint8_t)type;
    fuid.version  = FUID_CURRENT_VERSION;

    FS_LOG_DUMP_INFO("exit: ok");
    return fuid;
}

/* ============================================================
 * type
 * ============================================================ */

bool fuid_type_valid(fuid_type_t type)
{
    bool valid;

    FS_LOG_DUMP_INFO("enter: type=%u", (unsigned int)type);

    switch (type) {

    case FUID_TYPE_FILE:
    case FUID_TYPE_DIR:
    case FUID_TYPE_SYMLINK:
    case FUID_TYPE_FIFO:
    case FUID_TYPE_SOCK:
    case FUID_TYPE_BLK:
    case FUID_TYPE_CHR:
        valid = true;
        break;

    default:
        valid = false;
        break;
    }

    FS_LOG_DUMP_INFO("exit: %s",
                     valid ? "true" : "false");
    return valid;
}

fuid_type_t fuid_get_type(const fuid_t *fuid)
{
    fuid_type_t type;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    if (fuid == NULL) {
        type = FUID_TYPE_INVALID;
    } else {
        type = (fuid_type_t)fuid->type;
    }

    FS_LOG_DUMP_INFO("exit: %s", fuid_type_str(type));
    return type;
}

const char *fuid_type_str(fuid_type_t type)
{
    const char *str;

    switch (type) {

    case FUID_TYPE_FILE:
        str = "file";
        break;

    case FUID_TYPE_DIR:
        str = "dir";
        break;

    case FUID_TYPE_SYMLINK:
        str = "symlink";
        break;

    case FUID_TYPE_FIFO:
        str = "fifo";
        break;

    case FUID_TYPE_SOCK:
        str = "sock";
        break;

    case FUID_TYPE_BLK:
        str = "blk";
        break;

    case FUID_TYPE_CHR:
        str = "chr";
        break;

    default:
        str = "invalid";
        break;
    }

    return str;
}

/* ============================================================
 * type helper
 * ============================================================ */

bool fuid_is_file(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_FILE
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_dir(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_DIR
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_symlink(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_SYMLINK
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_fifo(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_FIFO
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_sock(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_SOCK
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_blk(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_BLK
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

bool fuid_is_chr(const fuid_t *fuid)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p", (const void *)fuid);

    result = (
        fuid_get_type(fuid) == FUID_TYPE_CHR
    );

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

/* ============================================================
 * flags
 * ============================================================ */

bool fuid_flag_test(const fuid_t *fuid, uint16_t flag)
{
    bool result;

    FS_LOG_DUMP_INFO("enter: fuid=%p, flag=0x%x",
                     (const void *)fuid, flag);

    if (fuid == NULL) {
        result = false;
    } else {
        result = (
            (fuid->flags & flag) != 0
        );
    }

    FS_LOG_DUMP_INFO("exit: %s",
                     result ? "true" : "false");
    return result;
}

void fuid_flag_set(fuid_t *fuid, uint16_t flag)
{
    FS_LOG_DUMP_INFO("enter: fuid=%p, flag=0x%x",
                     (void *)fuid, flag);

    if (fuid == NULL) {
        return;
    }

    fuid->flags |= flag;

    FS_LOG_DUMP_INFO("exit: done, flags=0x%x", fuid->flags);
}

void fuid_flag_clear(fuid_t *fuid, uint16_t flag)
{
    FS_LOG_DUMP_INFO("enter: fuid=%p, flag=0x%x",
                     (void *)fuid, flag);

    if (fuid == NULL) {
        return;
    }

    fuid->flags &= ~flag;

    FS_LOG_DUMP_INFO("exit: done, flags=0x%x", fuid->flags);
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
