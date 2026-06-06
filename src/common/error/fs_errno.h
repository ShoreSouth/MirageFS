#pragma once

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================
 * POSIX/Linux errno wrapper
 *
 * 说明：
 *
 * 1. 数值与 Linux errno 完全一致
 * 2. fs_error_t 的 errno 字段直接存储该值
 * 3. 项目代码统一使用 fs_errno_t
 * 4. 按常用程度分组排序，非按数值排序
 *
 * ============================================================
 */

typedef enum fs_errno {

    FS_ERRNO_OK = 0,

    /* ========================================================
     * 参数错误
     * ======================================================== */

    FS_ERRNO_EINVAL       = EINVAL,          /* Invalid argument */
    FS_ERRNO_ERANGE       = ERANGE,          /* Result too large */
    FS_ERRNO_EFAULT       = EFAULT,          /* Bad address */

    /* ========================================================
     * 文件/目录
     * ======================================================== */

    FS_ERRNO_ENOENT       = ENOENT,          /* No such file */
    FS_ERRNO_EEXIST       = EEXIST,          /* File exists */
    FS_ERRNO_ENOTDIR      = ENOTDIR,         /* Not a directory */
    FS_ERRNO_EISDIR       = EISDIR,          /* Is a directory */
    FS_ERRNO_ENOTEMPTY    = ENOTEMPTY,       /* Directory not empty */
    FS_ERRNO_ENAMETOOLONG = ENAMETOOLONG,    /* Name too long */
    FS_ERRNO_ELOOP        = ELOOP,           /* Too many symlinks */
    FS_ERRNO_EXDEV        = EXDEV,           /* Cross-device link */

    /* ========================================================
     * 权限
     * ======================================================== */

    FS_ERRNO_EPERM        = EPERM,           /* Operation not permitted */
    FS_ERRNO_EACCES       = EACCES,          /* Permission denied */
    FS_ERRNO_EROFS        = EROFS,           /* Read-only filesystem */

    /* ========================================================
     * 内存
     * ======================================================== */

    FS_ERRNO_ENOMEM       = ENOMEM,          /* Out of memory */

    /* ========================================================
     * IO
     * ======================================================== */

    FS_ERRNO_EIO          = EIO,             /* I/O error */
    FS_ERRNO_ENOSPC       = ENOSPC,          /* No space left */
    FS_ERRNO_EFBIG        = EFBIG,           /* File too large */
    FS_ERRNO_EPIPE        = EPIPE,           /* Broken pipe */
    FS_ERRNO_ESPIPE       = ESPIPE,          /* Illegal seek */

    /* ========================================================
     * FD / Resource
     * ======================================================== */

    FS_ERRNO_EBADF        = EBADF,           /* Bad fd */
    FS_ERRNO_EBUSY        = EBUSY,           /* Resource busy */
    FS_ERRNO_EMFILE       = EMFILE,          /* Too many open files */
    FS_ERRNO_ENFILE       = ENFILE,          /* File table overflow */

    /* ========================================================
     * 同步 / 重试
     * ======================================================== */

    FS_ERRNO_EAGAIN       = EAGAIN,          /* Try again */
    FS_ERRNO_EINTR        = EINTR,           /* Interrupted syscall */
    FS_ERRNO_ETIMEDOUT    = ETIMEDOUT,       /* Timeout */
    FS_ERRNO_EDEADLK      = EDEADLK,         /* Deadlock */

    /* ========================================================
     * Linux errno全集
     * ======================================================== */

    FS_ERRNO_ESRCH            = ESRCH,
    FS_ERRNO_ENXIO            = ENXIO,
    FS_ERRNO_E2BIG            = E2BIG,
    FS_ERRNO_ENOEXEC          = ENOEXEC,
    FS_ERRNO_ECHILD           = ECHILD,
    FS_ERRNO_ENOTBLK          = ENOTBLK,
    FS_ERRNO_ENODEV           = ENODEV,
    FS_ERRNO_ENOTTY           = ENOTTY,
    FS_ERRNO_ETXTBSY          = ETXTBSY,
    FS_ERRNO_EDOM             = EDOM,
    FS_ERRNO_ENOMSG           = ENOMSG,
    FS_ERRNO_EIDRM            = EIDRM,
    FS_ERRNO_ECHRNG           = ECHRNG,
    FS_ERRNO_EL2NSYNC         = EL2NSYNC,
    FS_ERRNO_EL3HLT           = EL3HLT,
    FS_ERRNO_EL3RST           = EL3RST,
    FS_ERRNO_ELNRNG           = ELNRNG,
    FS_ERRNO_EUNATCH          = EUNATCH,
    FS_ERRNO_ENOCSI           = ENOCSI,
    FS_ERRNO_EL2HLT           = EL2HLT,
    FS_ERRNO_EBADE            = EBADE,
    FS_ERRNO_EBADR            = EBADR,
    FS_ERRNO_EXFULL           = EXFULL,
    FS_ERRNO_ENOANO           = ENOANO,
    FS_ERRNO_EBADRQC          = EBADRQC,
    FS_ERRNO_EBADSLT          = EBADSLT,
#ifdef EBFONT
    FS_ERRNO_EBFONT           = EBFONT,
#endif
    FS_ERRNO_ENOSTR           = ENOSTR,
    FS_ERRNO_ENODATA          = ENODATA,
    FS_ERRNO_ETIME            = ETIME,
    FS_ERRNO_ENOSR            = ENOSR,
    FS_ERRNO_ENONET           = ENONET,
    FS_ERRNO_ENOPKG           = ENOPKG,
    FS_ERRNO_EREMOTE          = EREMOTE,
    FS_ERRNO_ENOLINK          = ENOLINK,
    FS_ERRNO_EADV             = EADV,
    FS_ERRNO_ESRMNT           = ESRMNT,
    FS_ERRNO_ECOMM            = ECOMM,
    FS_ERRNO_EPROTO           = EPROTO,
    FS_ERRNO_EMULTIHOP        = EMULTIHOP,
    FS_ERRNO_EDOTDOT          = EDOTDOT,
    FS_ERRNO_EBADMSG          = EBADMSG,
    FS_ERRNO_EOVERFLOW        = EOVERFLOW,
    FS_ERRNO_ENOTUNIQ         = ENOTUNIQ,
    FS_ERRNO_EBADFD           = EBADFD,
    FS_ERRNO_EREMCHG          = EREMCHG,
    FS_ERRNO_ELIBACC          = ELIBACC,
    FS_ERRNO_ELIBBAD          = ELIBBAD,
    FS_ERRNO_ELIBSCN          = ELIBSCN,
    FS_ERRNO_ELIBMAX          = ELIBMAX,
    FS_ERRNO_ELIBEXEC         = ELIBEXEC,
    FS_ERRNO_EILSEQ           = EILSEQ,
    FS_ERRNO_ERESTART         = ERESTART,
    FS_ERRNO_ESTRPIPE         = ESTRPIPE,
    FS_ERRNO_EUSERS           = EUSERS,
    FS_ERRNO_ENOTSOCK         = ENOTSOCK,
    FS_ERRNO_EDESTADDRREQ     = EDESTADDRREQ,
    FS_ERRNO_EMSGSIZE         = EMSGSIZE,
    FS_ERRNO_EPROTOTYPE       = EPROTOTYPE,
    FS_ERRNO_ENOPROTOOPT      = ENOPROTOOPT,
    FS_ERRNO_EPROTONOSUPPORT  = EPROTONOSUPPORT,
    FS_ERRNO_ESOCKTNOSUPPORT  = ESOCKTNOSUPPORT,
    FS_ERRNO_EOPNOTSUPP       = EOPNOTSUPP,
    FS_ERRNO_EPFNOSUPPORT     = EPFNOSUPPORT,
    FS_ERRNO_EAFNOSUPPORT     = EAFNOSUPPORT,
    FS_ERRNO_EADDRINUSE       = EADDRINUSE,
    FS_ERRNO_EADDRNOTAVAIL    = EADDRNOTAVAIL,
    FS_ERRNO_ENETDOWN         = ENETDOWN,
    FS_ERRNO_ENETUNREACH      = ENETUNREACH,
    FS_ERRNO_ENETRESET        = ENETRESET,
    FS_ERRNO_ECONNABORTED     = ECONNABORTED,
    FS_ERRNO_ECONNRESET       = ECONNRESET,
    FS_ERRNO_ENOBUFS          = ENOBUFS,
    FS_ERRNO_EISCONN          = EISCONN,
    FS_ERRNO_ENOTCONN         = ENOTCONN,
    FS_ERRNO_ESHUTDOWN        = ESHUTDOWN,
    FS_ERRNO_ETOOMANYREFS     = ETOOMANYREFS,
    FS_ERRNO_ECONNREFUSED     = ECONNREFUSED,
    FS_ERRNO_EHOSTDOWN        = EHOSTDOWN,
    FS_ERRNO_EHOSTUNREACH     = EHOSTUNREACH,
    FS_ERRNO_EALREADY         = EALREADY,
    FS_ERRNO_EINPROGRESS      = EINPROGRESS,
    FS_ERRNO_ESTALE           = ESTALE,
    FS_ERRNO_EUCLEAN          = EUCLEAN,
    FS_ERRNO_ENOTNAM          = ENOTNAM,
    FS_ERRNO_ENAVAIL          = ENAVAIL,
    FS_ERRNO_EISNAM           = EISNAM,
    FS_ERRNO_EREMOTEIO        = EREMOTEIO,
    FS_ERRNO_EDQUOT           = EDQUOT,
    FS_ERRNO_ENOMEDIUM        = ENOMEDIUM,
    FS_ERRNO_EMEDIUMTYPE      = EMEDIUMTYPE,
    FS_ERRNO_ECANCELED        = ECANCELED,
    FS_ERRNO_ENOKEY           = ENOKEY,
    FS_ERRNO_EKEYEXPIRED      = EKEYEXPIRED,
    FS_ERRNO_EKEYREVOKED      = EKEYREVOKED,
    FS_ERRNO_EKEYREJECTED     = EKEYREJECTED,
    FS_ERRNO_EOWNERDEAD       = EOWNERDEAD,
    FS_ERRNO_ENOTRECOVERABLE  = ENOTRECOVERABLE,
    FS_ERRNO_ERFKILL          = ERFKILL,
    FS_ERRNO_EHWPOISON        = EHWPOISON,

    /*
    * ============================================================
    * Linux alias errno
    *
    * 不单独定义，避免 switch duplicate case
    *
    * EWOULDBLOCK -> EAGAIN
    * EDEADLOCK   -> EDEADLK
    *
    * ============================================================
    */
} fs_errno_t;

/*
 * ============================================================
 * 编译期检查
 *
 * fs_error_t:
 * | level | module | sub | errno |
 * |  2bit | 10bit  |12bit| 8bit  |
 *
 * errno字段仅允许存储 0~255
 * ============================================================
 */

_Static_assert(ENOTRECOVERABLE < 256,
               "errno value exceeds fs_error_t errno field");

_Static_assert(EHWPOISON < 256,
               "errno value exceeds fs_error_t errno field");

/* ============================================================
 * helper
 * ============================================================ */

const char *fs_errno_name(fs_errno_t err);

const char *fs_errno_desc(fs_errno_t err);

bool fs_errno_valid(int err);