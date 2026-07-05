---

name: miragefs-core
description: Core architecture, coding conventions, module boundaries, review standards, and documentation requirements for MirageFS. Use whenever generating, modifying, reviewing, or designing MirageFS code.
---

# MirageFS Core Rules

## Purpose

MirageFS is a Linux userspace filesystem simulator written in C17.

This skill defines:

* Architecture rules
* Module boundaries
* Coding conventions
* Review standards
* Documentation requirements

All generated code and design proposals should follow these rules unless explicitly overridden.

---


## WSL Execution Rules

本项目在 WSL2 Ubuntu 环境中开发，仓库路径固定为：

```text
/home/shore/work/github/MirageFS
```

执行命令时应直接在 WSL bash 环境中运行，工作目录必须是上述路径。
不要通过 Windows UNC 路径访问仓库，例如 `\\wsl.localhost\...` 或
`\\wsl$\...`。

开始关键任务前可检查：

```sh
pwd
uname -a
echo "$SHELL"
whoami
git rev-parse --show-toplevel
```

文件修改优先使用 WSL 内部的 `git diff` / `git apply` 或 `python3` 脚本。
不要在 PowerShell 中构造包含中文注释的大型 here-doc 后再转发给 WSL。
补丁匹配应基于函数名、结构体字段、英文符号等稳定内容，不依赖中文注释。

# Working Principles

Before writing code:

1. Understand the module responsibility.
2. Check existing interfaces first.
3. Reuse existing common facilities whenever possible.
4. Preserve architectural consistency.
5. Prefer incremental modification over large-scale rewrites.

When requirements are unclear:

* Ask questions.
* Explore the existing codebase.
* Avoid inventing new abstractions prematurely.

---

# Project Structure

Current high-level architecture:

```text
src/

├── common/
│   ├── log/
│   ├── error/
│   ├── mempool/
│   ├── lock/
│   ├── list/
│   ├── trace/
│   └── ...
│
├── object/
│   ├── fuid/
│   ├── objkey/
│   ├── objmeta/
│   ├── objruntime/
│   ├── objtable/
│   ├── objpool/
│   └── objmgr/
│
├── cache/
├── vfs/
├── lsa/
└── cli/
```

Layer direction:

```text
CLI
 ↓
VFS
 ↓
CACHE
 ↓
OBJMETA / FUID
 ↓
LSA
 ↓
COMMON
```

Dependencies may only flow downward.

Never introduce upward dependencies.

---

# Module Responsibility

## common

Provides reusable infrastructure.

Examples:

* log
* error
* mempool
* lock
* list
* trace

Business logic must not live here.

---

## fuid

Provides object identity management.

Responsibilities:

* identifier layout
* type extraction
* encoding
* decoding

---

## objmeta

Provides object metadata management.

Responsibilities:

* metadata layout (key + handle)
* metadata validation
* metadata conversion

obj_meta_t describes **what** an object is — identity and backend locator only.
Runtime lifecycle state (refcnt, state) belongs to obj_runtime_t.

---

## objruntime

Provides the runtime object instance that all modules reference.

Responsibilities:

* aggregates obj_meta_t + refcnt + state
* lifecycle state machine (INIT → ACTIVE → DELETING)
* serves as the shared anchor point for Cache, Storage, Journal modules

obj_runtime_t describes an **object instance** — the runtime carrier for meta + lifecycle.
obj_meta_t is a member of obj_runtime_t, not the root object itself.

All modules (ObjMgr, Cache, Storage, VFS) reference `obj_runtime_t *`
as the common handle. Modules that only need metadata access `runtime->meta`.

---

## cache

Provides cache infrastructure.

Responsibilities:

* cache entry management
* cache lifecycle
* replacement policy
* cache lookup

---

## vfs

Provides filesystem semantics.

Responsibilities:

* create
* lookup
* unlink
* rename
* readdir

---

## lsa

Provides low-level system access.

Responsibilities:

* Linux syscall wrappers
* filesystem backend operations

Must not contain VFS semantics.

---

# Language Rules

Language:

```text
C17
```

Supported compilers:

```text
gcc
clang
```

Avoid compiler-specific extensions unless explicitly required.

Forbidden:

```text
C++
Nested Functions
Variable Length Arrays (VLA)
```

---

# Naming Rules

## Error Code System

All modules (including COMMON) use the unified `fs_error_t` system. Never mix `0`/`-1` with `fs_error_t`.

### Error Layout (32-bit)

```
 31 30 | 29 -------- 20 | 19 -------- 8 | 7 -------- 0
-------------------------------------------------------
severity|   module id   |   sub error   |    errno
```

- **severity** (2-bit): `FS_SEV_INFO`, `FS_SEV_WARN`, `FS_SEV_ERROR`, `FS_SEV_FATAL`
- **module** (10-bit): First-level module — `FS_MODULE_COMMON`, `FS_MODULE_OBJECT`, `FS_MODULE_LSA`, etc.
- **sub** (12-bit): Component within the module. For COMMON: `FS_COMMON_SUB_HASH`, `FS_COMMON_SUB_LOCK`, `FS_COMMON_SUB_PATH`, etc. For OBJECT: `OBJ_SUB_INIT`, `OBJ_SUB_INSERT`, etc.
- **errno** (8-bit): Linux errno value

### Sub-Error Rules

1. **`sub` identifies the internal component**, not necessarily a "sub-module". For example, COMMON's sub values are: HASH, LOCK, PATH, MEMPOOL, ATOMIC, LIST, LOG, OS, TRACE, UTILS, etc.
2. **Only use `FS_SUB_NONE` (or `FS_COMMON_SUB_NONE`) as a fallback** when the error truly cannot be attributed to any specific component. It must not be the default choice.
3. **Each module registers its sub-name callback** via `fs_sub_register()` during module init. For example, `object_init()` registers both `FS_MODULE_COMMON` and `FS_MODULE_OBJECT`.

### Error Constructor Pattern

Each module provides an error constructor that hardcodes its module ID:

```c
// COMMON module
fs_error_t fs_common_error(uint32_t sub, int err);

// Object Layer
fs_error_t obj_error(obj_sub_t sub, int err);

// LSA
fs_error_t lsa_error(fs_op_t sub, int err);
```

Usage:

```c
return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
return obj_error(OBJ_SUB_INSERT, FS_ERRNO_EEXIST);
return lsa_error(FS_OP_LOOKUP, FS_ERRNO_ENOENT);
```

### Error Return Convention

All functions that can fail return `fs_error_t` directly — never `int` or `int32_t`:

```c
fs_error_t fs_hash_init(fs_hash_t *hash, ...);
fs_error_t fs_mutex_init(fs_mutex_t *lock, ...);
fs_error_t objtable_init(obj_table_t *table, uint32_t bucket_nr);
fs_error_t objmgr_init(void);
```

Check results with `fs_failed()` / `fs_succeeded()`:

```c
err = fs_hash_init(&table, bucket_nr, ...);
if (fs_failed(err)) {
    FS_LOG_DUMP_ERROR("fs_hash_init failed, err=%s (0x%x)",
                      fs_error_str(err), err);
    return err;
}
```

Never bridge `0`/`-1` to `fs_error_t` — if a called function already returns `fs_error_t`, propagate it directly. Wrapping with a new error code loses the original component attribution.
### Resource Cleanup and `goto` Convention

For functions that acquire multiple resources in stages, prefer one exit
path and `goto` cleanup labels. Typical examples are module `init`,
`create`, `open`, and any function that owns rollback responsibility.

Rules:

1. Keep one `fs_error_t err` and return it at the final `out:` label.
2. On failure, jump to the label that releases already-acquired resources.
3. Cleanup labels release resources in reverse acquisition order.
4. Label names should describe the resource boundary, for example
   `err_nspool`, `err_fsid`, `err_sysroot`, then `out`.
5. Do not force this style onto tiny validation/query helpers that do not
   acquire resources; early return is acceptable there.

Preferred pattern:

```c
fs_error_t xxx_init(void)
{
    fs_error_t err;

    err = first_init();
    if (fs_failed(err)) {
        goto out;
    }

    err = second_init();
    if (fs_failed(err)) {
        goto err_first;
    }

    goto out;

err_first:
    first_deinit();

out:
    return err;
}
```

---

## Verb Convention (Public API)

All public functions follow a strict verb-based naming convention.

Format:

```text
<module>_<verb>[_<noun>]()
```

The verb set is closed — do not invent new verbs without updating this document.

### Value Object Verbs

For plain-data structs (stack / embedded, no internal heap resources):

| Verb | Signature | Semantics |
|------|-----------|-----------|
| `xxx_make` | `T xxx_make(A a, B b)` | Construct a value object, return by value |
| `xxx_is_valid` | `bool xxx_is_valid(const T *v)` | Return true if all fields are legal |
| `xxx_equal` | `bool xxx_equal(const T *a, const T *b)` | Return true if identity fields match |
| `xxx_from_yyy` | `void xxx_from_yyy(T *out, const Y *in)` | Convert / project from another type |
| `xxx_hash` | `uint64_t xxx_hash(const T *v)` | Hash on identity fields |

Examples:

```c
/* fuid — file unique identity (value object) */
fuid_t  fuid_make(Fsid_t fsid, ObjectId_t oid, GenId_t gen, fuid_type_t type);
bool    fuid_is_valid(const fuid_t *fuid);
bool    fuid_equal(const fuid_t *a, const fuid_t *b);
void    fuid_init(fuid_t *fuid);   /* reset to zero / invalid */

/* objkey — object index key (value object) */
obj_key_t  objkey_make(ObjectId_t oid, GenId_t gen);
bool       objkey_is_valid(const obj_key_t *key);
bool       objkey_equal(const obj_key_t *lhs, const obj_key_t *rhs);
void       objkey_from_fuid(obj_key_t *key, const fuid_t *fuid);

/* objhandle — backend handle (value object) */
obj_handle_t  objhandle_make(int32_t mount_id, uint16_t type,
                             uint16_t len, const uint8_t *data);
bool          objhandle_valid(const obj_handle_t *h);
bool          objhandle_equal(const obj_handle_t *a, const obj_handle_t *b);
void          objhandle_from_fuid(obj_handle_t *h, const fuid_t *fuid);
```

Rules:
- `valid` uses an `is_` prefix — `fuid_is_valid()`, not `fuid_valid()`.
- `make` is the single verb for value construction — never `build`, `create`, `new`, `construct`.
- `from_xxx` is output-parameter style: `void xxx_from_yyy(T *out, const Y *in)`.
- `init` on a value object means "reset to zero / invalid state" (no allocation).

### Runtime Object Verbs

For structs that own internal heap resources (hash tables, pools, caches):

| Verb | Semantics |
|------|-----------|
| `xxx_init` | Initialize an already-allocated object, may allocate internal resources. Caller owns the memory. |
| `xxx_deinit` | Release internal resources, leave the struct zeroed. Does NOT free the struct itself. |
| `xxx_create` | Allocate + init. Returns pointer. |
| `xxx_destroy` | Deinit + free. Nulls the pointer. |

Guidance:
- Prefer `init / deinit` when the struct is embedded inside another struct.
- Use `create / destroy` only when heap allocation of the struct itself is the common case.
- Every `init` must have a matching `deinit`; every `create` must have a matching `destroy`.

Examples:

```c
/* objtable — embedded hash table (init / deinit) */
fs_error_t  objtable_init(obj_table_t *table, uint32_t bucket_nr);
void        objtable_deinit(obj_table_t *table);

/* mempool — heap-allocated pool (create / destroy) */
fs_mempool_t *fs_mp_create(const fs_mp_config_t *cfg);
void          fs_mp_destroy(fs_mempool_t *mp);

/* objmgr — global singleton (module-level init / deinit) */
fs_error_t objmgr_init(void);
void       objmgr_deinit(void);
```

### CRUD / Operation Verbs

For higher-level services (objmgr, vfs, cache):

| Verb | Semantics |
|------|-----------|
| `xxx_insert` | Add an entry (may fail if duplicate) |
| `xxx_remove` | Delete an entry by key |
| `xxx_lookup` | Find and return pointer (no refcount change) |
| `xxx_exists` | Return bool — cheaper than lookup |
| `xxx_acquire` | Lookup + increment refcount |
| `xxx_release` | Decrement refcount (pair with acquire) |
| `xxx_get` | Increment refcount by key |
| `xxx_put` | Decrement refcount by key |
| `xxx_count` | Return current entry count |

Examples:

```c
fs_error_t   objtable_insert(obj_table_t *t, const obj_meta_t *meta);
fs_error_t   objtable_remove(obj_table_t *t, const obj_key_t *key);
obj_meta_t  *objtable_lookup(obj_table_t *t, const obj_key_t *key);
bool         objtable_exists(obj_table_t *t, const obj_key_t *key);

obj_meta_t  *objmgr_acquire(fuid_t fuid);
void         objmgr_release(obj_meta_t *meta);
```

### Debug / Utility Verbs

| Verb | Semantics |
|------|-----------|
| `xxx_dump` | Print a compact one-line struct snapshot to log (important fields, no entry/exit banners) |
| `xxx_to_str` | Return a static string representation (short, human-readable, no logging side effects) |
| `xxx_type_str` | Return string name for an enum value |

Examples:

```c
void         objmeta_dump(const obj_meta_t *meta);
void         objruntime_dump(const obj_runtime_t *rt);
const char  *fuid_to_str(const fuid_t *fuid);
const char  *fuid_type_str(fuid_type_t type);
```

### Getter / Setter Convention

- Simple field access: `xxx_get_<field>()` / `xxx_set_<field>()`
- Boolean type checks: `xxx_is_<type>()` — the `is_` prefix is ONLY for subtype checks

Examples:

```c
fuid_type_t  fuid_get_type(const fuid_t *fuid);    /* getter */
bool         fuid_is_file(const fuid_t *fuid);     /* subtype check — is_ prefix ok here */
bool         fuid_is_dir(const fuid_t *fuid);
```

### Flag Operations

```c
bool  xxx_flag_test(const T *v, uint16_t flag);
void  xxx_flag_set(T *v, uint16_t flag);
void  xxx_flag_clear(T *v, uint16_t flag);
```

### Anti-Patterns

Never expose generic names:

```c
create()    /* use <module>_create */
destroy()   /* use <module>_destroy */
lookup()    /* use <module>_lookup */
init()      /* use <module>_init */
```

Never mix verbs for the same operation across modules:

```c
objkey_valid()   /* wrong — use objkey_is_valid() */
objmeta_reset()  /* wrong — use objmeta_deinit() */
```

---

## Internal Functions

Static functions should also preserve module context.

Examples:

```c
static fs_error_t  cache_do_insert(void);
static fs_error_t  cache_do_remove(void);
static bool        objmeta_valid(const obj_meta_t *meta);
```

Naming consistency is preferred over shortening.

---

## Type Naming

All structures use typedef style.

Preferred:

```c
typedef struct fs_mempool {

    void *base;
    uint64_t size;

} fs_mempool_t;
```

Avoid anonymous structures.

---

## Enum Naming

Preferred:

```c
typedef enum cache_state {

    CACHE_EMPTY = 0,
    CACHE_VALID,
    CACHE_DIRTY,

} cache_state_t;
```

---

## Macro Naming

Use uppercase.

Examples:

```c
FS_PAGE_SIZE

FS_CACHE_BUCKETS

OBJMETA_MAGIC
```

---

# Include Rules

Current module header first.

Example:

```c
#include "cache/cache.h"

#include "common/fs_common.h"
#include "common/fs_log.h"
#include "common/fs_error.h"

#include <stdbool.h>
#include <stdint.h>
```

Rules:

1. Current module header first.
2. Project headers before system headers.
3. Include from module entry paths.
4. Avoid unnecessary includes.
5. Prefer forward declarations where appropriate.

---

# Const Correctness

All pointer parameters that are read but not written must be `const`.

```c
/* good — input pointers are const */
bool fuid_equal(const fuid_t *a, const fuid_t *b);
void objkey_from_fuid(obj_key_t *key, const fuid_t *fuid);

/* bad — missing const on read-only pointer */
bool fuid_equal(fuid_t *a, fuid_t *b);
```

Rules:
- Output / in-out parameters: no const.
- Input parameters: always const.
- This lets the caller immediately see which parameters may be mutated.

---

# Parameter Direction Annotations

All public API function documentation MUST annotate each parameter with a direction tag.

Use one of three tags:

| Tag | Meaning |
|-----|---------|
| `[IN]` | Read-only input. Callee reads but does not modify. `const` pointers are always `[IN]`. |
| `[OUT]` | Output only. Callee writes to this parameter. Non-const pointers are usually `[OUT]`. |
| `[IN/OUT]` | Input and output. Callee reads and may modify. Non-const pointers to mutable state. |

Examples:

```c
/*
 * 初始化 ObjMeta。
 *
 * 参数：
 *      [OUT] meta      : 目标对象（由 objpool_alloc 分配）
 *      [IN]  fuid      : MirageFS 对象标识
 *      [IN]  handle    : Linux backend handle
 */
fs_error_t objmeta_init(
                obj_meta_t *meta,
                const fuid_t *fuid,
                const obj_handle_t *handle);

/*
 * 插入对象。
 *
 * 参数：
 *      [IN/OUT] table  : 对象表
 *      [IN]     meta   : 待插入的元数据
 */
fs_error_t objtable_insert(
            obj_table_t *table,
            const obj_meta_t *meta);
```

Rules:
- Every parameter in a `/* 参数：... */` block must have a direction tag.
- Tags appear left-aligned in a column of their own (`[IN]`, `[OUT]`, `[IN/OUT]` are 7 chars wide).
- `const` pointers are always `[IN]` — the compiler enforces this.
- Non-const pointers that are only written-to (not read) are `[OUT]`.
- Non-const pointers that are both read and written are `[IN/OUT]`.
- Value-type parameters (non-pointer) are always `[IN]` and may omit the tag when the intent is obvious.

---

# Inline Functions

Value-object helpers belong in the header as `static inline`.

Only use `static inline` when ALL of:
- The function body is ≤ 10 lines.
- It has no side effects besides constructing / inspecting a value.
- It is called on hot paths (lookup, comparison, hash).

Examples:

```c
/* objkey.h — good fit for static inline */
static inline obj_key_t objkey_make(ObjectId_t oid, GenId_t gen)
{
    obj_key_t key;
    key.objectid = oid;
    key.gen      = gen;
    return key;
}

static inline bool objkey_is_valid(const obj_key_t *key)
{
    if (key == NULL) { return false; }
    return (key->objectid != 0) && (key->gen != 0);
}
```

Anything that allocates, locks, logs, or has complex error paths belongs in the `.c` file.

---

# NULL Handling

NULL checks on public API boundaries are required.

```c
fs_error_t objtable_insert(obj_table_t *table, const obj_meta_t *meta)
{
    fs_error_t err;

    if ((table == NULL) || (meta == NULL)) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table or meta is NULL, "
                          "err=%s (0x%x)", fs_error_str(err), err);
        return err;
    }
    /* ... */
}
```

Once past the public boundary, internal static helpers may skip redundant NULL checks when the caller has already validated.

Return conventions for NULL-able returns:
- Functions returning pointers: `NULL` means "not found" or "error".
- Functions returning `bool`: `false` on NULL input (defensive).
- Functions returning `fs_error_t`: positive `fs_error_t` on NULL input (typically `FS_ERRNO_EINVAL`).

---

# Code Layout

Opening brace on next line.

Example:

```c
fs_error_t cache_lookup(cache_t *cache,
                 uint64_t key)
{
    if (cache == NULL) {
        return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
    }

    return FS_OK;
}
```

---

# Structure Layout

Group related fields together.

Example:

```c
typedef struct cache_entry {

    uint64_t hash;
    uint32_t refcnt;

    void *key;
    void *value;

    struct cache_entry *prev;
    struct cache_entry *next;

} cache_entry_t;
```

Separate logical groups using blank lines.

---

# Comment Style

MirageFS uses C-style comments consistently.

## Field Comments

For structure fields and short descriptions:

```c
uint64_t total_size; /* 内存池总大小 */
uint32_t refcnt;     /* 引用计数 */
```

Preferred for:

- structure fields
- enum items
- macro descriptions

## Local Comments

For short logical explanations:

```c
/* 参数检查 */
if (pool == NULL) {
    return fs_common_error(FS_COMMON_SUB_MEMPOOL, FS_ERRNO_EINVAL);
}
```

```c
/* 从空闲链表摘除 */
cache_remove(entry);
```

## Section comments

Use section separators for major code blocks:

```c
/* ============================================================
 * public api
 * ============================================================ */
```

Recommended sections:

```text
type definition
private helper
public api
debug helper
compile time check
```

Avoid obvious comments that merely restate code.

---

# Return Value Conventions

Use consistent return types across the codebase.

| Return type | Meaning |
|-------------|---------|
| `fs_error_t` | `FS_OK` (0) = success; positive value = structured error (severity\|module\|sub\|errno) |
| `bool` | Predicate result (valid, equal, exists, etc.) |
| `T` (value type) | Constructed value object (never fails) |
| `T *` (pointer) | `NULL` = not found / error; non-NULL = valid pointer |
| `uint64_t` / `uint32_t` / `int32_t` | Count, hash, or refcount (unsigned or signed, never fails) |

Examples:

```c
/* fs_error_t: FS_OK = ok, >0 = structured error */
fs_error_t  objtable_init(obj_table_t *table, uint32_t bucket_nr);
fs_error_t  objtable_insert(obj_table_t *table, const obj_meta_t *meta);
fs_error_t  fs_hash_init(fs_hash_t *hash, uint32_t bucket_nr, ...);
fs_error_t  fs_mutex_init(fs_mutex_t *lock, const char *name, uint32_t flags);

/* bool: predicate */
bool fuid_is_valid(const fuid_t *fuid);
bool objkey_equal(const obj_key_t *a, const obj_key_t *b);
bool fs_path_is_absolute(const char *path);

/* value type: constructor */
fuid_t      fuid_make(Fsid_t fsid, ObjectId_t oid, GenId_t gen, fuid_type_t type);
obj_key_t   objkey_make(ObjectId_t oid, GenId_t gen);

/* pointer: NULL = not found */
obj_meta_t *objtable_lookup(obj_table_t *table, const obj_key_t *key);
obj_meta_t *objmgr_acquire(fuid_t fuid);

/* count / refcnt: never fails */
uint64_t objtable_count(const obj_table_t *table);
uint64_t fuid_hash(const fuid_t *fuid);
int32_t  objmgr_refcnt(const fuid_t *fuid);
```

Rules:
- Every function that can fail must return `fs_error_t`. Never use `int` or `int32_t` for error returns.
- Never use bare `0`/`-1` for error returns. Use `FS_OK` and structured `fs_error_t` values.
- Check errors with `fs_failed(err)` or `fs_succeeded(err)`, never with `!= 0` or `== -1`.
- Do not bridge `0`/`-1` into `fs_error_t` by wrapping — propagate the original `fs_error_t` directly.
- Value constructors (`xxx_make`) never fail — they simply pack fields.
- Count and refcount functions (`xxx_count`, `xxx_refcnt`) return their natural integer type.

---

# Logging

Use MirageFS logging facilities only.

## Macros

```c
FS_LOG_DUMP_DEBUG(fmt, ...)   /* verbose —— 仅开发期开启 */
FS_LOG_DUMP_INFO(fmt, ...)    /* 关键路径进出 / 状态变化 */
FS_LOG_DUMP_WARN(fmt, ...)    /* 可恢复异常 */
FS_LOG_DUMP_ERROR(fmt, ...)   /* 不可恢复错误 */
```

Each macro automatically injects `__FILE__`, `__LINE__`, `__func__` via `fs_log_write()`.

Do NOT introduce `printf()`, `fprintf()`, `puts()` outside debugging experiments.

## Log Levels — When To Use

| Level | Use Case |
|-------|----------|
| `DEBUG` | Detailed internal state (hash bucket walk, mempool expansion, etc.) |
| `INFO` | Function entry/exit, state transitions, key decisions |
| `WARN` | Recoverable anomalies (retry, fallback, degraded mode) |
| `ERROR` | Unrecoverable failure — always paired with `fs_error_str(err)` |

## Error Log — Unified Pattern

Every error path MUST follow this pattern:

```c
fs_error_t err;

err = <module>_error(<SUB>, <ERRNO>);
FS_LOG_DUMP_ERROR("<what> failed: <why>, err=%s (0x%x)",
                  fs_error_str(err), err);
return err;
```

Rules:
- `<what>` describes the operation that failed (e.g., `fs_hash_init`, `param check`, `calloc`).
- `<why>` gives the business reason (e.g., `table is NULL`, `object already exists`).
- `err=%s` always uses `fs_error_str(err)` to emit the full decoded error.
- `(0x%x)` always follows —— the raw hex value is grep-friendly and unambiguous.
- The error object is created first, logged once, then returned.
- `fs_error_str(err)` already emits severity + module + sub + errno + description —— do NOT duplicate that information in the format string.

Correct:

```c
err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EEXIST);
FS_LOG_DUMP_ERROR("insert failed: object already exists, "
                  "key=(%lu,%u), err=%s (0x%x)",
                  (unsigned long)key.objectid,
                  (unsigned int)key.gen,
                  fs_error_str(err), err);
return err;
```

Wrong:

```c
/* BAD: bare string, no error code */
FS_LOG_DUMP_ERROR("table is NULL");
return obj_error(OBJ_SUB_INIT, FS_ERRNO_EINVAL);

/* BAD: passes fs_error_t as format string (CRASH) */
FS_LOG_DUMP_ERROR(err);

/* BAD: missing fs_error_str, error code not decoded */
FS_LOG_DUMP_ERROR("init failed, ret=%d", ret);
```

## Entry / Exit — INFO Pattern

Every public function MUST log entry and exit at `INFO` level.

**Entry** —— log key input parameters:

```c
FS_LOG_DUMP_INFO("enter: table=%p, key=%p", (void *)table, (void *)key);
```

**Exit (success)** —— log key result:

```c
FS_LOG_DUMP_INFO("exit: ok, count=%lu", (unsigned long)count);
```

**Exit (not found / predicate false)** —— log the outcome:

```c
FS_LOG_DUMP_INFO("exit: not found");
FS_LOG_DUMP_INFO("exit: false");
```

**Exit (error)** —— the error path already logs via `FS_LOG_DUMP_ERROR`; add a one-line exit:

```c
FS_LOG_DUMP_INFO("exit: failed, ret=%d", (int)ret);
return ret;
```

**Void functions** —— log `done`:

```c
FS_LOG_DUMP_INFO("exit: done");
```

## Query Functions

Query functions (`lookup`, `exists`, `count`, `is_valid`, `equal`, etc.) also get entry/exit INFO logs. If the volume becomes excessive, lower the global log level or selectively downgrade specific functions to `DEBUG` later. A consistent baseline is more important than premature optimization.

## Internal Helpers

`static` helper functions may omit entry/exit if they are thin wrappers (≤ 3 lines) or pure field-access predicates. Use judgment: if a helper contains branching or error paths, log it.

## Complete Example

```c
fs_error_t objtable_insert(obj_table_t *table, const obj_meta_t *meta)
{
    fs_error_t err;
    objtable_entry_t *entry;
    obj_key_t key;

    FS_LOG_DUMP_INFO("enter: table=%p, meta=%p",
                     (void *)table, (void *)meta);

    if (table == NULL) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EINVAL);
        FS_LOG_DUMP_ERROR("param check failed: table is NULL, err=%s (0x%x)",
                          fs_error_str(err), err);
        return err;
    }

    /* ... validation, business logic ... */

    if (objtable_exists(table, &key)) {
        err = obj_error(OBJ_SUB_INSERT, FS_ERRNO_EEXIST);
        FS_LOG_DUMP_ERROR("insert failed: object already exists, "
                          "key=(%lu,%u), err=%s (0x%x)",
                          (unsigned long)key.objectid,
                          (unsigned int)key.gen,
                          fs_error_str(err), err);
        return err;
    }

    FS_LOG_DUMP_INFO("exit: ok");
    return FS_OK;
}
```

## Anti-Patterns Summary

| Anti-Pattern | Why Wrong |
|--------------|-----------|
| Bare string, no error code | Can't grep for the error; can't correlate across modules |
| `FS_LOG_DUMP_ERROR(err)` | `fs_error_t` (uint32_t) is not a format string —— potential crash |
| `FS_LOG_DUMP_ERROR("... %d", err)` | Raw integer is opaque; use `fs_error_str(err) + (0x%x)` |
| Missing entry/exit logs | Silent functions make production debugging impossible |
| English+Chinese mixed in log messages | Logs must be grep-friendly; Chinese is for comments only |
| `printf()` in production code | Bypasses the log infrastructure (file, rotation, level filter) |

---

# Memory Management

Primary allocator:

```c
fs_mempool
```

Use:

```c
fs_mp_create()

fs_mp_destroy()

fs_mp_alloc()

fs_mp_free()
```

Avoid direct use of:

```c
malloc()
calloc()
realloc()
free()
```

except when implementing memory infrastructure itself.

Ownership must always be clear.

Every allocation must have a defined release path.

---

# Concurrency

Use common synchronization abstractions.

Examples:

```c
fs_mutex_t

fs_spinlock_t

fs_rwlock_t
```

Do not expose pthread types outside common.

Bad:

```c
pthread_mutex_t
```

Good:

```c
fs_mutex_t
```

---

# Alignment

Core metadata structures should consider cacheline alignment.

Example:

```c
64B cacheline alignment
```

Critical metadata structures should define size expectations explicitly.

---

# Compile-Time Validation

Important structures should provide compile-time size validation.

Example:

```c
_Static_assert(
    sizeof(obj_meta_t) == OBJMETA_SIZE,
    "obj_meta_t size invalid");
```

---

# API Design

Object lifetime must be explicit.

Typical lifecycle:

```text
create  →  destroy     (heap alloc + dealloc)
init    →  deinit      (init resources in-place)
acquire →  release     (refcount +1 / -1)
```

Avoid hidden initialization.

Avoid hidden ownership transfer.

Every `init` must be paired with a matching `deinit` in the same module.
Every `create` must be paired with a matching `destroy` in the same module.

---

# Opaque Type Pattern

When a struct's internals should not be visible to callers, use an opaque typedef in the header and define the struct only in the `.c` file.

Header:

```c
/* fs_mempool.h */
typedef struct fs_mempool fs_mempool_t;

fs_mempool_t *fs_mp_create(const fs_mp_config_t *cfg);
void          fs_mp_destroy(fs_mempool_t *mp);
void         *fs_mp_alloc(fs_mempool_t *mp, size_t size);
```

Source:

```c
/* fs_mempool.c */
struct fs_mempool {
    void    *base;
    uint64_t size;
    /* ... */
};
```

Use opaque types when:
- The struct layout is an implementation detail that may change.
- Direct field access would break invariants.
- The module is in a lower layer and callers only use it through its API.

Do NOT use opaque types when:
- The struct is a value object passed by value (fuid_t, obj_key_t).
- The struct is embedded inside another struct (obj_meta_t inside obj_runtime_t).
- sizeof() or inline access is needed for performance on hot paths.

---

# Header File Layout

Every public header follows this order:

```c
#pragma once

/* 1. system headers */
#include <stdbool.h>
#include <stdint.h>

/* 2. project headers */
#include "common/fs_common.h"

/* 3. type definitions (enums first, then structs) */
typedef enum { ... } foo_type_t;
typedef struct foo { ... } foo_t;

/* 4. compile-time checks */
_Static_assert(sizeof(foo_t) == FOO_SIZE, "foo_t size invalid");

/* 5. public API declarations, grouped by category */
/* ---- lifecycle ---- */
int  foo_init(foo_t *f, ...);
void foo_deinit(foo_t *f);

/* ---- value ops ---- */
foo_t foo_make(...);
bool  foo_valid(const foo_t *f);
bool  foo_equal(const foo_t *a, const foo_t *b);

/* ---- debug ---- */
void foo_dump(const foo_t *f);
```

Rules:
- No function bodies in `.h` except `static inline` helpers (see Inline Functions).
- Section separators (`/* ==== ... ==== */`) are mandatory between logical groups.
- Forward-declare opaque types at the top of the type-definitions section.

---

# Documentation Rules

Documentation is part of the codebase.

Whenever a change affects:

* public API
* structure layout
* module responsibility
* object lifecycle
* architecture decisions

update the corresponding documentation.

Examples:

```text
docs/FUID.md

docs/OBJMETA.md

docs/CACHE.md

docs/VFS.md
```

Code and documentation must remain consistent.

---

# Design Discussion Rules

When designing new functionality:

1. Verify whether an existing module already owns the responsibility.
2. Challenge unnecessary abstractions.
3. Prefer simple solutions first.
4. Resolve lifecycle ownership before implementation.
5. Resolve concurrency strategy before implementation.

For significant architectural changes:

* discuss design first
* implement later

---

# Review Focus

When reviewing code, pay special attention to:

1. Memory leak
2. Double free
3. Ownership confusion
4. Lock leak
5. Deadlock risk
6. Layer violation
7. Incomplete error path
8. Missing logging
9. Missing cleanup path
10. Missing compile-time validation
11. Cacheline layout issues
12. API consistency

---

# Output Expectations

When generating code:

* preserve existing style
* avoid unnecessary refactoring
* avoid renaming stable interfaces
* prefer patch-style changes
* explain architectural impact
* mention documentation updates if required

Consistency is more important than cleverness.
