# Utils Module

## Overview

Utils 模块提供 MirageFS 通用工具函数集合。

主要职责：

* 数值处理
* 对齐计算
* 区间处理
* Trim 裁剪
* 字符串安全操作
* 内存辅助函数
* Debug 工具

该模块不维护状态。

所有接口均采用：

* Header Only
* Static Inline
* Zero Allocation

设计。

---

## Design Goals

### Avoid Repeated Code

统一实现常见工具逻辑。

避免各模块重复编写：

```c
align_up()
align_down()
min()
max()
range_check()
```

等代码。

---

### Reduce Boundary Bugs

MirageFS 中大量操作涉及：

* offset
* length
* block
* page
* buffer
* extent

边界处理极易出现：

* 整数溢出
* 越界访问
* 对齐错误

Utils 模块统一处理这些逻辑。

---

### Improve Readability

推荐：

```c
offset = fs_align_up(offset, 4096);
```

而不是：

```c
offset = (offset + 4095) & ~4095;
```

使代码语义更加明确。

---

## Architecture

```text
mempool
cache
buffer
lsa
inode
vfs
   │
   ▼
 utils
```

Utils 位于 Common 最底层。

不依赖 MirageFS 其他模块。

---

# Numeric Helpers

## Minimum

```c
fs_min_u64()
fs_min_u32()
```

返回较小值。

示例：

```c
uint64_t n = fs_min_u64(a, b);
```

---

## Maximum

```c
fs_max_u64()
fs_max_u32()
```

返回较大值。

示例：

```c
uint64_t n = fs_max_u64(a, b);
```

---

# Alignment Helpers

## Why Alignment

MirageFS 中大量对象存在对齐要求：

* Page
* Block
* IO
* DMA
* SGL
* Buffer

例如：

* 4KB
* 64KB
* 1MB

边界。

---

## Check Alignment

```c
fs_is_aligned(x, align)
```

示例：

```c
if (fs_is_aligned(offset, 4096)) {

}
```

---

## Align Down

向下对齐：

```c
fs_align_down(x, align)
```

示例：

```c
fs_align_down(5000, 4096);
```

结果：

```text
4096
```

---

## Align Up

向上对齐：

```c
fs_align_up(x, align)
```

示例：

```c
fs_align_up(5000, 4096);
```

结果：

```text
8192
```

---

## Alignment Requirement

当前实现要求：

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

```text
1000
3000
```

非法。

---

# Range Helpers

## Motivation

文件系统中最常见的数据模型：

```text
[offset, offset + len)
```

Utils 提供统一区间处理函数。

---

## Range Validation

```c
fs_range_valid2(offset, len, max_size)
```

检查：

### Length Valid

```text
len > 0
```

---

### Overflow Check

检查：

```text
offset + len
```

是否发生整数溢出。

---

### Boundary Check

检查：

```text
offset + len <= max_size
```

---

## Range End

```c
fs_range_end(offset, len)
```

等价：

```c
offset + len
```

用于增强语义表达。

---

## Range Overlap

```c
fs_range_overlap()
```

判断两个区间是否重叠。

例如：

```text
[0,100)

[50,150)
```

返回：

```text
true
```

---

# Trim Helpers

## Motivation

文件系统 IO 经常出现：

```text
非对齐请求
```

例如：

```text
offset = 100
len    = 10000
```

但底层要求：

```text
4KB 对齐
```

需要裁剪出：

```text
完整块区域
```

---

## Trim To Aligned

```c
fs_trim_to_aligned()
```

输入：

```text
offset = 100
len    = 10000
align  = 4096
```

输出：

```text
offset = 4096
len    = 4096
```

保留完整块。

---

## Trim With Statistics

```c
fs_trim_to_aligned_ex()
```

额外返回：

```c
fs_trim_info_t
```

记录裁剪信息。

### Head Trim

```c
info->head_len
```

表示头部裁掉长度。

### Tail Trim

```c
info->tail_len
```

表示尾部裁掉长度。

---

## Typical Usage

适用于：

* Direct IO
* Block IO
* SGL
* DMA
* Extent IO

等场景。

---

# String Helpers

## Safe Copy

```c
fs_strlcpy()
```

行为类似 BSD：

```c
strlcpy()
```

特点：

* 保证 '\0'
* 避免越界
* 返回源字符串长度

推荐替代：

```c
strcpy()
```

---

# Memory Helpers

## Zero Memory

```c
fs_memzero(ptr, size)
```

等价：

```c
memset(ptr, 0, size)
```

用于增强代码可读性。

示例：

```c
fs_memzero(&inode, sizeof(inode));
```

---

# Debug Helpers

## Hex Dump

```c
fs_dump_hex()
```

按十六进制打印 Buffer。

示例：

```c
fs_dump_hex(buf, len);
```

输出：

```text
01 02 03 04
AA BB CC DD
...
```

---

## Typical Usage

用于：

* Metadata Debug
* IO Debug
* Protocol Debug
* Corruption Analysis

---

# Dependency Relationship

Utils 模块仅依赖：

```text
stdint.h
stddef.h
string.h
stdio.h
```

不依赖 MirageFS 其他模块。

---

## Used By

几乎所有核心模块：

* mempool
* cache
* buffer
* inode
* objmeta
* lsa
* vfs

都会直接使用 Utils。

属于 MirageFS Common 基础工具库。

---

# Coding Guidelines

推荐：

```c
fs_align_up()
fs_range_valid2()
fs_strlcpy()
fs_memzero()
```

不推荐：

```c
(offset + 4095) & ~4095

strcpy()

memset(..., 0, ...)
```

直接散落在业务代码中。

统一通过 Utils 模块提供公共实现。

---

# Future Extensions

未来可扩展：

* bitmap helper
* crc helper
* hash helper
* uuid helper
* random helper

保持：

* Header Only
* Static Inline
* No State

设计原则不变。
