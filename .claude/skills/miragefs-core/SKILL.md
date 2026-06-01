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
├── fuid/
├── objmeta/
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

* metadata layout
* metadata validation
* metadata conversion

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

## Public API

Use module prefixes.

Format:

```text
<module>_<action>()
```

Examples:

```c
fs_mp_create()
fs_mp_alloc()

fuid_get_type()

objmeta_validate()

cache_lookup()

vfs_create()

lsa_open()
```

Never expose generic names such as:

```c
create()
destroy()
lookup()
```

---

## Internal Functions

Static functions should also preserve module context.

Examples:

```c
static int cache_do_insert();
static int cache_do_remove();

static bool objmeta_is_valid();

static int fs_mp_expand();
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

# Code Layout

Opening brace on next line.

Example:

```c
int cache_lookup(cache_t *cache,
                 uint64_t key)
{
    if (cache == NULL) {
        return FS_EINVAL;
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
    return FS_EINVAL;
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

# Error Handling

Use project error types.

Example:

```c
fs_error_t rc;

rc = cache_insert(cache, entry);
if (rc != FS_OK) {
    return rc;
}
```

Avoid:

```c
return -1;
return NULL;
```

unless explicitly documented.

---

# Logging

Use MirageFS logging facilities only.

Current logging macros:

```c
FS_LOG_DUMP_DEBUG()

FS_LOG_DUMP_INFO()

FS_LOG_DUMP_WARN()

FS_LOG_DUMP_ERROR()
```

Do not introduce:

```c
printf()
fprintf()
puts()
```

outside debugging experiments.

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
    sizeof(objmeta_t) == FS_OBJMETA_SIZE,
    "objmeta_t size invalid");
```

---

# API Design

Object lifetime must be explicit.

Typical lifecycle:

```text
create
destroy

init
fini

get
put
```

Avoid hidden initialization.

Avoid hidden ownership transfer.

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
