# Macros Module

## Overview

Macros 模块提供 MirageFS 编译期基础设施。

包含两个头文件：

```text
common/macros/
├── fs_defs.h
└── fs_macros.h
```

其中：

| 文件          | 职责             |
| ----------- | -------------- |
| fs_defs.h   | 系统常量、架构约束、默认配置 |
| fs_macros.h | 通用宏、语法增强、编译辅助  |

Macros 模块位于 MirageFS 最底层。

不依赖其他 MirageFS 模块。

---

## Design Goals

### Centralized Definitions

统一管理：

* 容量单位
* Page 参数
* Buddy 参数
* 系统限制
* 默认配置

避免魔法数字散落在代码中。

---

### Compile-Time Infrastructure

提供：

* container_of
* align
* min/max
* bit operation
* static assert

等基础能力。

供所有模块使用。

---

### Improve Readability

推荐：

```c
size = FS_ALIGN_UP(size, MP_PAGE_SIZE);
```

而不是：

```c
size = (size + 4095) & ~4095;
```

增强代码可读性。

---

## Architecture

```text
common
│
├── types
├── utils
├── os
├── path
├── trace
├── log
│
└── macros
     ├── fs_defs.h
     └── fs_macros.h
```

Macros 位于 MirageFS 基础层。

几乎所有模块都会直接依赖。

---

# fs_defs.h

## Overview

fs_defs.h 用于定义 MirageFS 全局常量。

这些定义代表：

* 系统约束
* 架构参数
* 默认配置

属于工程级配置。

---

## Capacity Units

提供统一容量单位：

```c
FS_KB
FS_MB
FS_GB
FS_TB
```

示例：

```c
uint64_t size = 128 * FS_MB;
```

避免：

```c
134217728
```

这类不可读数字。

---

## Page Definitions

当前 MirageFS 固定采用：

```text
4KB Page
```

定义：

```c
MP_PAGE_SHIFT
MP_PAGE_SIZE
MP_PAGE_MASK
```

对应：

```text
PAGE_SHIFT = 12
PAGE_SIZE  = 4096
```

---

### Typical Usage

页对齐：

```c
size = FS_ALIGN_UP(size, MP_PAGE_SIZE);
```

页号计算：

```c
page_index = offset >> MP_PAGE_SHIFT;
```

---

## Buddy Allocator Definitions

Buddy 分配器参数：

```c
MP_MAX_ORDER
MP_MAX_BLOCK_SIZE
```

当前：

```text
order 0  -> 4KB
order 1  -> 8KB
...
order 10 -> 4MB
```

最大块：

```text
4MB
```

---

## Filesystem Limits

### File Name Length

```c
FS_MAX_NAME_LEN
```

当前：

```text
255
```

与 Linux 保持一致。

---

### Path Length

```c
FS_MAX_PATH_LEN
```

当前：

```text
4096
```

用于路径缓存与路径处理模块。

---

## Default Configuration

### Default Memory Pool

```c
FS_DEFAULT_MEMPOOL_SIZE
```

当前：

```text
128MB
```

用于默认内存池初始化。

---

### Default Worker Count

```c
FS_DEFAULT_WORKER_NR
```

当前：

```text
1
```

后续可扩展为多线程工作模型。

---

# fs_macros.h

## Overview

fs_macros.h 提供 MirageFS 通用开发宏。

主要用于：

* 代码简化
* 类型安全
* 编译器优化
* 调试辅助

---

## Array Helpers

### FS_ARRAY_SIZE

计算数组元素个数：

```c
FS_ARRAY_SIZE(arr)
```

示例：

```c
int a[32];

size_t nr =
    FS_ARRAY_SIZE(a);
```

结果：

```text
32
```

---

## Min / Max

### FS_MIN

返回较小值：

```c
FS_MIN(a, b)
```

---

### FS_MAX

返回较大值：

```c
FS_MAX(a, b)
```

---

### Notes

内部使用：

```c
typeof()
```

避免参数重复求值问题。

---

## Alignment Helpers

### FS_ALIGN_UP

向上对齐：

```c
FS_ALIGN_UP(x, align)
```

示例：

```c
FS_ALIGN_UP(5000, 4096)
```

结果：

```text
8192
```

---

### FS_ALIGN_DOWN

向下对齐：

```c
FS_ALIGN_DOWN(x, align)
```

结果：

```text
4096
```

---

### FS_IS_ALIGNED

检查是否对齐：

```c
FS_IS_ALIGNED(x, align)
```

---

### Requirements

要求：

```text
align 必须是 2 的幂
```

例如：

```text
512
4096
65536
```

合法。

---

## Bit Operations

### Single Bit

```c
FS_BIT(n)

FS_BIT_ULL(n)
```

示例：

```c
FS_BIT(3)
```

结果：

```text
0x8
```

---

### Flag Operations

设置：

```c
FS_SET_FLAG(v, flag)
```

清除：

```c
FS_CLR_FLAG(v, flag)
```

判断：

```c
FS_HAS_FLAG(v, flag)
```

---

### Typical Usage

状态位：

```c
FS_SET_FLAG(flags, OBJ_DIRTY);

if (FS_HAS_FLAG(flags, OBJ_DIRTY)) {

}
```

---

## Container Of

### FS_CONTAINER_OF

根据成员地址获取宿主结构体。

定义：

```c
FS_CONTAINER_OF(ptr, type, member)
```

---

### Example

```c
typedef struct inode {
    fs_list_head_t list;
} inode_t;
```

获取：

```c
inode_t *inode =
    FS_CONTAINER_OF(
        node,
        inode_t,
        list);
```

---

### Typical Usage

广泛用于：

* list
* cache
* queue
* object manager

等模块。

---

## Branch Prediction

### FS_LIKELY

表示高概率路径：

```c
FS_LIKELY(x)
```

---

### FS_UNLIKELY

表示低概率路径：

```c
FS_UNLIKELY(x)
```

---

### Example

```c
if (FS_UNLIKELY(ptr == NULL)) {
    return -EINVAL;
}
```

帮助编译器优化分支布局。

---

## Static Assert

### FS_STATIC_ASSERT

编译期检查：

```c
FS_STATIC_ASSERT(cond, msg)
```

---

### Example

```c
FS_STATIC_ASSERT(
    sizeof(obj_meta_t) == 64,
    "obj_meta size invalid");
```

编译失败时立即发现问题。

---

## Warning Control

### FS_UNUSED

消除未使用变量告警：

```c
FS_UNUSED(x);
```

---

### FS_FALLTHROUGH

显式标记 switch 穿透：

```c
switch (type) {

case A:
    prepare();
    FS_FALLTHROUGH;

case B:
    run();
}
```

增强代码可读性。

---

## Type Check

### FS_TYPE_CHECK

编译期类型检查：

```c
FS_TYPE_CHECK(a, b)
```

用于调试阶段验证类型兼容性。

---

# Dependency Relationship

fs_defs.h：

```text
No Dependency
```

---

fs_macros.h：

```text
stddef.h
```

仅依赖标准库。

---

# Coding Guidelines

推荐：

```c
FS_ARRAY_SIZE()
FS_ALIGN_UP()
FS_ALIGN_DOWN()
FS_CONTAINER_OF()
FS_STATIC_ASSERT()
```

统一使用公共宏。

不推荐：

```c
sizeof(arr)/sizeof(arr[0])

(ptr - offset)

硬编码常量
```

散落在业务代码中。

---

# Important Notes

## fs_defs.h

用于：

```text
系统定义
架构参数
默认配置
```

不应存放：

```text
业务逻辑
运行时变量
```

---

## fs_macros.h

用于：

```text
工具宏
编译辅助
语法增强
```

不应存放：

```text
业务功能
复杂逻辑
```

保持轻量。

---

# Future Extensions

未来可扩展：

```text
BUILD_BUG_ON

READ_ONCE

WRITE_ONCE

ROUND_UP

ROUND_DOWN

CACHELINE_SIZE

CACHELINE_ALIGN
```

继续保持：

```text
Header Only
Compile-Time Friendly
Zero Runtime Cost
```

设计原则不变。
