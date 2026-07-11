---
name: miragefs-core
description: MirageFS 核心架构、编码规范、模块边界、评审重点与文档要求。生成、修改、评审或设计 MirageFS 代码时必须使用。
---

# MirageFS 核心规则

## 目标

MirageFS 是一个使用 C17 编写的 Linux 用户态文件系统模拟器。

本 skill 约束以下内容：

* 架构边界
* 模块职责
* 编码规范
* 命名规范
* 错误与日志规范
* 文档更新要求
* 代码评审重点

除非用户明确要求覆盖，本文件中的规则优先适用于所有 MirageFS 代码、文档和设计讨论。

---

## WSL 执行规则

本项目在 WSL2 Ubuntu 环境中开发，仓库路径固定为：

```text
/home/shore/work/github/MirageFS
```

执行命令时应直接在 WSL bash 环境中运行，工作目录必须是上述路径。不要通过 Windows UNC 路径访问仓库，例如 `\\wsl.localhost\...` 或 `\\wsl$\...`。

开始关键任务前可检查：

```sh
pwd
uname -a
echo "$SHELL"
whoami
git rev-parse --show-toplevel
```

文件修改优先使用 WSL 内部的 `git diff` / `git apply`，或在必要时使用小脚本做批量机械替换。不要在 PowerShell 中构造包含中文注释的大型 here-doc 后再转发给 WSL。

补丁匹配应基于函数名、结构体字段、英文符号等稳定内容，不依赖中文注释。

---

## 工作原则

写代码前必须先理解模块职责和现有接口。

基本原则：

1. 先确认职责归属，再写实现。
2. 优先复用已有公共设施。
3. 保持模块边界稳定，不引入向上依赖。
4. 优先做小步、可验证的修改。
5. 不为单个场景过早抽象。
6. 不随意重命名稳定接口。

需求不清时：

* 先探索现有代码和文档。
* 必要时向用户确认设计边界。
* 不凭空发明新模块、新术语或新生命周期。

---

## 当前架构

当前高层结构大致如下：

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
├── object/
│   ├── fuid/
│   ├── objkey/
│   ├── objmeta/
│   ├── objruntime/
│   ├── objtable/
│   ├── objpool/
│   └── objmgr/
├── fsc/
├── fops/
├── namei/
├── lsa/
└── app/
```

依赖方向只能自上而下。上层可以调用下层，下层不能反向依赖上层。

当前主链路：

```text
APP / CLI / SERVER
        ↓
      NAMEI
        ↓
      FOPS
        ↓
  OBJMGR / FSC
        ↓
       LSA
        ↓
 Linux Kernel
```

不得引入反向依赖或跨层捷径。

---

## 模块职责

### common

提供通用基础设施，例如日志、错误、锁、链表、哈希、路径、内存池、trace 等。

业务语义不能放进 `common`。

### object

负责 MirageFS 对象身份、元数据、运行时实例和对象表管理。

关键概念：

* `fuid_t`：文件系统内对象身份。
* `obj_key_t`：对象索引键。
* `obj_meta_t`：对象身份与后端定位信息。
* `obj_runtime_t`：带生命周期和引用计数的运行时对象实例。
* `objmgr`：对象生命周期和查找管理。

`obj_meta_t` 描述对象是什么；`obj_runtime_t` 描述对象实例当前处于什么运行状态。

### fsc

负责文件系统实例、namespace、root、fsid、挂载级上下文等管理。

### fops

负责单步文件操作语义，例如 lookup、create、mkdir、unlink、rename、readlink、readdir 等。

正式上层模块必须通过 `fops_dispatch()` 进入 FOPS。细粒度 FOPS API 可以保留给 FOPS 内部、兼容层和 UT 使用，但 NAMEI、SERVER、CLI 等正式调用方不得直接调用 `fops_create_plus()`、`fops_mkdir_plus()`、`fops_readlink()` 这类接口。

### namei

负责路径解析和 FOPS 参数组织。

NAMEI 可以提供薄封装，让 SERVER / CLI 不必手动拆路径，但 NAMEI 不直接执行底层文件操作，不管理对象生命周期，不管理 namespace 创建/销毁。

### lsa

负责 Linux syscall 和后端文件系统访问封装。

LSA 不能包含 VFS/FOPS/NAMEI 的上层语义。

---

## 语言规则

语言标准：

```text
C17
```

支持编译器：

```text
gcc
clang
```

禁止使用：

```text
C++
Nested Functions
Variable Length Arrays (VLA)
```

除非已有明确理由，不使用编译器私有扩展。

---

## 错误系统

所有模块统一使用 `fs_error_t`。不得把 `0` / `-1` 和 `fs_error_t` 混用。

### 错误布局

```text
 31 30 | 29 -------- 20 | 19 -------- 8 | 7 -------- 0
-------------------------------------------------------
severity|   module id   |   sub error   |    errno
```

字段含义：

* `severity`：`FS_SEV_INFO`、`FS_SEV_WARN`、`FS_SEV_ERROR`、`FS_SEV_FATAL`
* `module`：一级模块，例如 `FS_MODULE_COMMON`、`FS_MODULE_OBJECT`、`FS_MODULE_LSA`、`FS_MODULE_NAMEI`
* `sub`：模块内部组件，不一定是子模块
* `errno`：Linux errno 值

### sub 错误规则

1. `sub` 用来标识模块内部组件，例如 HASH、LOCK、PATH、MEMPOOL、LOOKUP、WALK。
2. 只有在确实无法归因时才使用 `FS_SUB_NONE` 或模块内的 `*_SUB_NONE`。
3. 每个模块初始化时应通过 `fs_sub_register()` 注册 sub 名称回调。
4. 如果被调用函数已经返回 `fs_error_t`，直接向上传播，不要重新包装。

### 错误构造模式

每个模块提供固定 module id 的错误构造函数：

```c
fs_error_t fs_common_error(uint32_t sub, int err);
fs_error_t obj_error(obj_sub_t sub, int err);
fs_error_t lsa_error(fs_op_t sub, int err);
fs_error_t namei_error(namei_sub_t sub, int err);
```

使用示例：

```c
return fs_common_error(FS_COMMON_SUB_HASH, FS_ERRNO_EINVAL);
return obj_error(OBJ_SUB_INSERT, FS_ERRNO_EEXIST);
return lsa_error(FS_OP_LOOKUP, FS_ERRNO_ENOENT);
```

所有可能失败的函数都应直接返回 `fs_error_t`，不得用 `int` 或 `int32_t` 承载错误语义。

检查错误时使用：

```c
if (fs_failed(err)) {
    return err;
}
```

不得使用 `err != 0` 或 `ret == -1` 判断 MirageFS 内部错误。

---

## 资源清理与 goto 约定

当函数分阶段获取多个资源时，优先使用单一出口和 `goto` 清理标签。典型场景包括模块 `init`、`create`、`open`，以及任何拥有回滚责任的函数。

规则：

1. 使用一个 `fs_error_t err`，最终在 `out:` 返回。
2. 失败时跳到释放已获取资源的标签。
3. 清理顺序与获取顺序相反。
4. 标签名表达资源边界，例如 `err_nspool`、`err_fsid`、`err_sysroot`。
5. 没有资源所有权的小型校验函数可以早返回。

推荐模式：

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

## 命名规则

### 公共 API 动词约定

公共函数采用严格的动词命名：

```text
<module>_<verb>[_<noun>]()
```

动词集合应保持收敛。新增动词前必须先更新本文件。

### 值对象动词

用于无内部堆资源的普通结构体：

| 动词 | 语义 |
|------|------|
| `xxx_make` | 构造值对象并按值返回 |
| `xxx_is_valid` | 判断字段是否合法 |
| `xxx_equal` | 判断身份字段是否一致 |
| `xxx_from_yyy` | 从另一个类型转换或投影 |
| `xxx_hash` | 按身份字段计算哈希 |

规则：

* 合法性判断使用 `is_` 前缀，例如 `fuid_is_valid()`。
* 值对象构造统一使用 `make`，不使用 `build`、`new`、`construct`。
* `from_xxx` 使用输出参数形式：`void xxx_from_yyy(T *out, const Y *in)`。
* 值对象上的 `init` 表示重置为零值或无效值，不表示分配资源。

### 运行时对象动词

用于拥有内部资源的结构体：

| 动词 | 语义 |
|------|------|
| `xxx_init` | 初始化已分配对象，可分配内部资源 |
| `xxx_deinit` | 释放内部资源，不释放结构体本身 |
| `xxx_create` | 分配结构体并初始化 |
| `xxx_destroy` | 反初始化并释放结构体 |

规则：

* 嵌入式成员优先使用 `init / deinit`。
* 需要堆分配结构体本身时使用 `create / destroy`。
* 每个 `init` 必须有配对 `deinit`。
* 每个 `create` 必须有配对 `destroy`。

### CRUD / 操作动词

| 动词 | 语义 |
|------|------|
| `xxx_insert` | 插入条目，重复时可失败 |
| `xxx_remove` | 按键删除条目 |
| `xxx_lookup` | 查找并返回指针，不改变引用计数 |
| `xxx_exists` | 判断是否存在 |
| `xxx_acquire` | 查找并增加引用计数 |
| `xxx_release` | 减少引用计数 |
| `xxx_get` | 按键增加引用计数 |
| `xxx_put` | 按键减少引用计数 |
| `xxx_count` | 返回条目数量 |

### 调试与工具动词

| 动词 | 语义 |
|------|------|
| `xxx_dump` | 输出结构体紧凑快照到日志 |
| `xxx_to_str` | 返回静态字符串表示 |
| `xxx_type_str` | 返回枚举值名称 |

### Getter / Setter

* 简单字段访问：`xxx_get_<field>()` / `xxx_set_<field>()`
* 布尔类型判断：`xxx_is_<type>()`
* `is_` 只用于子类型判断或合法性判断，不用于普通 getter。

### flag 操作

```c
bool  xxx_flag_test(const T *v, uint16_t flag);
void  xxx_flag_set(T *v, uint16_t flag);
void  xxx_flag_clear(T *v, uint16_t flag);
```

### 禁止模式

不要暴露无模块前缀的泛名：

```c
create()
destroy()
lookup()
init()
```

不要在不同模块为同一语义混用动词：

```c
objkey_valid()   /* 错，应使用 objkey_is_valid() */
objmeta_reset()  /* 错，应使用 objmeta_deinit() */
```

---

## 内部函数命名

`static` 函数也应保留模块上下文，不能为了短而丢语义。

示例：

```c
static fs_error_t cache_do_insert(void);
static fs_error_t cache_do_remove(void);
static bool       objmeta_is_valid(const obj_meta_t *meta);
```

---

## 局部变量命名

局部变量也必须表达用途，不能只表达“临时”状态。

规则：

1. 禁止使用 `tmp`、`temp`、`foo`、`bar`、`data` 这类语义不明的变量名，除非它们出现在纯示例占位文本中。
2. 临时缓冲区应按内容命名，例如 `path_buf`、`name_buf`、`target_path`、`parent_path`。
3. 中间结果应按业务角色命名，例如 `lookup_result`、`parent_result`、`create_attr`。
4. 循环变量只在极小作用域内可以使用 `i`、`j`；跨越多个分支或参与业务判断时，也应使用语义名。
5. 不要为了缩短代码牺牲可读性，优先选择和模块概念一致的完整名称。

---

## 类型、枚举和宏命名

结构体使用 typedef 风格，避免匿名结构体。

```c
typedef struct fs_mempool {

    void *base;
    uint64_t size;

} fs_mempool_t;
```

枚举使用模块化名称：

```c
typedef enum cache_state {

    CACHE_EMPTY = 0,
    CACHE_VALID,
    CACHE_DIRTY,

} cache_state_t;
```

宏使用全大写：

```c
FS_PAGE_SIZE
FS_CACHE_BUCKETS
OBJMETA_MAGIC
```

---

## include 规则

公共头文件和源文件的 include 顺序：

1. 当前模块头文件优先。
2. 项目头文件在系统头文件之前。
3. 从模块入口路径 include。
4. 避免不必要 include。
5. 能用前置声明时优先使用前置声明。

示例：

```c
#include "cache/cache.h"

#include "common/fs_common.h"
#include "common/fs_log.h"
#include "common/fs_error.h"

#include <stdbool.h>
#include <stdint.h>
```

---

## const 正确性

只读指针参数必须加 `const`。

```c
bool fuid_equal(const fuid_t *a, const fuid_t *b);
void objkey_from_fuid(obj_key_t *key, const fuid_t *fuid);
```

规则：

* 输出参数和输入输出参数不加 `const`。
* 输入指针参数必须加 `const`。
* 让调用方一眼能看出哪些参数可能被修改。

---

## 参数方向标注

公共 API 文档必须为每个参数标注方向。

可用标注：

| 标注 | 含义 |
|------|------|
| `[IN]` | 只读输入，callee 不修改 |
| `[OUT]` | 只写输出 |
| `[IN/OUT]` | 输入输出，callee 会读取并修改 |

示例：

```c
/*
 * 初始化 ObjMeta。
 *
 * 参数：
 *      [OUT] meta      : 目标对象
 *      [IN]  fuid      : MirageFS 对象标识
 *      [IN]  handle    : Linux backend handle
 */
fs_error_t objmeta_init(
                obj_meta_t *meta,
                const fuid_t *fuid,
                const obj_handle_t *handle);
```

规则：

* `const` 指针总是 `[IN]`。
* 非 const 但只写的参数是 `[OUT]`。
* 非 const 且读写的参数是 `[IN/OUT]`。
* 值类型参数天然是 `[IN]`，意图明显时可以省略。

---

## inline 规则

值对象 helper 可以放在头文件中作为 `static inline`。

只有同时满足以下条件才使用 `static inline`：

1. 函数体不超过 10 行。
2. 除构造或检查值对象外没有副作用。
3. 位于热点路径，例如 lookup、比较、hash。

涉及分配、锁、日志或复杂错误路径的函数必须放在 `.c` 文件。

---

## NULL 处理

公共 API 边界必须检查 NULL。

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

    return FS_OK;
}
```

通过公共边界后，内部 `static` helper 可以在调用方已保证参数合法时省略重复 NULL 检查。

返回约定：

* 返回指针：`NULL` 表示未找到或错误。
* 返回 `bool`：NULL 输入时防御性返回 `false`。
* 返回 `fs_error_t`：NULL 输入通常返回 `FS_ERRNO_EINVAL` 对应错误。

---

## 代码布局

左花括号另起一行。

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

结构体字段按逻辑分组，组之间空行分隔。

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

---

## 注释风格

MirageFS 统一使用 C 风格注释。文档和代码注释以中文为主。

字段和短描述使用尾注释：

```c
uint64_t total_size; /* 内存池总大小 */
uint32_t refcnt;     /* 引用计数 */
```

局部逻辑注释使用块注释：

```c
/* 参数检查 */
if (pool == NULL) {
    return fs_common_error(FS_COMMON_SUB_MEMPOOL, FS_ERRNO_EINVAL);
}
```

重要代码分区使用分隔注释：

```c
/* ============================================================
 * public api
 * ============================================================ */
```

推荐分区名：

```text
type definition
private helper
public api
debug helper
compile time check
```

不要写只复述代码的注释。

---

## 返回值约定

全项目保持返回类型一致。

| 返回类型 | 含义 |
|----------|------|
| `fs_error_t` | `FS_OK` 表示成功，正值表示结构化错误 |
| `bool` | 谓词结果 |
| `T` | 值对象构造结果，不失败 |
| `T *` | `NULL` 表示未找到或错误 |
| `uint64_t` / `uint32_t` / `int32_t` | 计数、哈希、引用计数等自然数值 |

规则：

* 可能失败的函数必须返回 `fs_error_t`。
* 不使用裸 `0` / `-1` 表达错误。
* 用 `fs_failed(err)` / `fs_succeeded(err)` 检查错误。
* 不把已有 `fs_error_t` 重新包装成新错误。
* 值构造函数不失败，只打包字段。

---

## 日志规则

只使用 MirageFS 日志设施，不在正式代码中引入 `printf()`、`fprintf()`、`puts()`。

日志宏：

```c
FS_LOG_DUMP_DEBUG(fmt, ...)   /* 详细调试信息 */
FS_LOG_DUMP_INFO(fmt, ...)    /* 关键路径进出 / 状态变化 */
FS_LOG_DUMP_WARN(fmt, ...)    /* 可恢复异常 */
FS_LOG_DUMP_ERROR(fmt, ...)   /* 不可恢复错误 */
```

日志宏会通过 `fs_log_write()` 自动带上 `__FILE__`、`__LINE__`、`__func__`。

### 日志级别

| 级别 | 使用场景 |
|------|----------|
| `DEBUG` | 详细内部状态，例如 hash bucket walk、mempool expansion |
| `INFO` | 函数进入/退出、状态变更、关键决策 |
| `WARN` | 可恢复异常，例如 retry、fallback、降级 |
| `ERROR` | 不可恢复失败，必须配合 `fs_error_str(err)` |

### 错误日志统一模式

每条错误路径必须遵循：

```c
fs_error_t err;

err = <module>_error(<SUB>, <ERRNO>);
FS_LOG_DUMP_ERROR("<what> failed: <why>, err=%s (0x%x)",
                  fs_error_str(err), err);
return err;
```

规则：

* `<what>` 描述失败操作，例如 `fs_hash_init`、`param check`。
* `<why>` 描述业务原因，例如 `table is NULL`。
* 必须使用 `fs_error_str(err)`。
* 必须同时打印原始十六进制错误值 `(0x%x)`。
* 错误对象先创建，日志只打一遍，然后返回。
* 日志正文保持英文，方便 grep；中文用于文档和注释。

禁止模式：

```c
FS_LOG_DUMP_ERROR("table is NULL"); /* 缺少结构化错误 */
FS_LOG_DUMP_ERROR(err);             /* err 不是 format string */
FS_LOG_DUMP_ERROR("ret=%d", err);   /* 缺少 fs_error_str(err) */
printf("debug\n");                 /* 绕过日志系统 */
```

### 入口与退出日志

公共函数必须在 `INFO` 级别记录进入和退出。

```c
FS_LOG_DUMP_INFO("enter: table=%p, key=%p", (void *)table, (void *)key);
FS_LOG_DUMP_INFO("exit: ok");
```

查询函数也需要记录进入和退出。如果日志量过大，后续可按模块降低等级，但默认先保持一致。

内部 `static` helper 如果只是很薄的包装或纯字段访问，可以省略进入/退出日志；如果包含分支或错误路径，应记录日志。

---

## 内存管理

优先使用项目内存池设施：

```c
fs_mp_create()
fs_mp_destroy()
fs_mp_alloc()
fs_mp_free()
```

避免直接使用：

```c
malloc()
calloc()
realloc()
free()
```

实现内存基础设施本身时可以例外。

所有权必须清晰，每次分配都要有明确释放路径。

---

## 并发规则

使用 common 提供的同步抽象，不在模块边界暴露 pthread 类型。

推荐：

```c
fs_mutex_t
fs_spinlock_t
fs_rwlock_t
```

禁止在模块公共接口中暴露：

```c
pthread_mutex_t
```

---

## 对齐与编译期校验

核心元数据结构应考虑 cacheline 对齐。

重要结构应提供编译期尺寸校验。

```c
_Static_assert(
    sizeof(obj_meta_t) == OBJMETA_SIZE,
    "obj_meta_t size invalid");
```

---

## API 设计

对象生命周期必须显式。

典型生命周期：

```text
create  -> destroy     /* 分配结构体并释放 */
init    -> deinit      /* 原地初始化并反初始化 */
acquire -> release     /* 引用计数 +1 / -1 */
```

规则：

* 不隐藏初始化。
* 不隐藏所有权转移。
* `init` 和 `deinit` 必须在同一模块内成对出现。
* `create` 和 `destroy` 必须在同一模块内成对出现。

---

## Opaque Type 模式

当结构体内部布局不应暴露给调用者时，在头文件中使用 opaque typedef，在 `.c` 文件中定义结构体。

头文件：

```c
typedef struct fs_mempool fs_mempool_t;

fs_mempool_t *fs_mp_create(const fs_mp_config_t *cfg);
void          fs_mp_destroy(fs_mempool_t *mp);
void         *fs_mp_alloc(fs_mempool_t *mp, size_t size);
```

源文件：

```c
struct fs_mempool {
    void    *base;
    uint64_t size;
};
```

适合 opaque type 的场景：

* 结构体布局是实现细节。
* 直接字段访问会破坏不变量。
* 模块位于下层，调用方只应通过 API 使用。

不适合 opaque type 的场景：

* 值对象按值传递，例如 `fuid_t`、`obj_key_t`。
* 结构体需要嵌入其他结构体。
* 热点路径需要 `sizeof()` 或 inline 字段访问。

---

## 公共头文件布局

公共头文件采用固定顺序：

```c
#pragma once

/* 1. system headers */
#include <stdbool.h>
#include <stdint.h>

/* 2. project headers */
#include "common/fs_common.h"

/* 3. type definitions */
typedef enum foo_type { ... } foo_type_t;
typedef struct foo { ... } foo_t;

/* 4. compile-time checks */
_Static_assert(sizeof(foo_t) == FOO_SIZE, "foo_t size invalid");

/* 5. public API declarations */
```

规则：

* `.h` 中除 `static inline` helper 外不写函数体。
* 逻辑分组之间必须有分隔注释。
* opaque 类型的前置声明放在类型定义区顶部。

---

## 文档规则

文档是代码库的一部分。

当改动影响以下内容时，必须同步更新文档：

* public API
* 结构体布局
* 模块职责
* 对象生命周期
* 架构决策
* 跨模块调用约束

文档和代码必须保持一致。

文档以中文为主；代码标识符、命令、日志格式、API 名称保持原文。

---

## 设计讨论规则

设计新功能时：

1. 先确认现有模块是否已经拥有该职责。
2. 主动质疑不必要的抽象。
3. 优先选择简单方案。
4. 先解决生命周期所有权，再实现。
5. 先解决并发策略，再实现共享状态。

重大架构改动应先讨论设计，再写正式代码。

---

## 评审重点

评审代码时优先关注：

1. 内存泄漏
2. double free
3. 所有权不清
4. 锁泄漏
5. 死锁风险
6. 层级依赖违规
7. 错误路径不完整
8. 缺少日志
9. 缺少清理路径
10. 缺少编译期校验
11. cacheline 布局风险
12. API 命名不一致
13. 文档与代码不一致
14. 上层绕过统一 dispatch 入口

---

## 输出要求

生成代码或文档时：

* 保持现有风格。
* 避免无关重构。
* 避免重命名稳定接口。
* 优先小补丁修改。
* 说明架构影响。
* 涉及公共接口、职责边界或调用链时同步更新文档。
* 文档和注释以中文为主。
* 局部变量名必须表达业务含义，不使用 `tmp` 等弱语义名称。

一致性比炫技更重要。