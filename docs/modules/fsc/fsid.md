# fsid.md

# FSID 模块设计

## 1. 模块定位

`fsid/` 是 FSC 中最底层、最独立的身份分配组件。
它负责为 filesystem instance 分配唯一身份：

```text
fsc_fsid_t
```

FSC 使用 `fsc_fsid_t` 而不是 `fsid_t`，原因是 Linux 系统头文件中已经存在 `fsid_t` 类型。
为了避免命名冲突，FSC 内部统一使用 `fsc_fsid_t`。

---

# 2. 模块职责

FSID 模块负责：

- 初始化 FSID 分配器
- 销毁 FSID 分配器
- 分配新的 FSID
- 释放 FSID
- 复用已释放 slot
- 通过 generation 防止 stale FSID 被误用
- 判断 FSID 编码是否有效
- 为 hash index 提供 hash 值

FSID 模块不负责：

- Namespace 内存管理
- Namespace 生命周期编排
- Mount 关系
- Policy 检查
- Object 创建

---

# 3. FSID 编码

`fsc_fsid_t` 是 64-bit 值类型，内部编码为：

```text
63                         32 31                         0
+----------------------------+----------------------------+
|        generation          |            slot            |
+----------------------------+----------------------------+
```

其中：

- `slot`：分配器内部槽位，从 `1` 开始，`0` 保留为 invalid。
- `generation`：slot 的版本号，从 `1` 开始，每次释放 slot 后递增。

无效值：

```text
FSID_INVALID == 0
```

---

# 4. 分配器结构

当前实现使用固定容量静态分配器：

```text
FSID_MAX_SLOTS = 4096
```

内部状态：

```text
allocated[]   : slot bitmap，记录 slot 是否正在使用
generation[]  : slot 当前 generation
free_stack[]  : 可复用 slot 栈
free_count    : 当前空闲 slot 数量
lock          : 保护上述状态
```

分配流程：

```text
fsid_alloc()
  ├── lock
  ├── 从 free_stack 弹出 slot
  ├── 读取 generation[slot]
  ├── 标记 allocated[slot] = true
  ├── 组合 FSID = generation:slot
  └── unlock
```

释放流程：

```text
fsid_free(fsid)
  ├── 拆出 slot / generation
  ├── 检查 slot 合法
  ├── 检查 slot 已分配
  ├── 检查 generation 匹配
  ├── 标记 allocated[slot] = false
  ├── generation[slot]++
  └── slot 压回 free_stack
```

---

# 5. stale 防护

旧 FSID 的风险来自 slot 复用：

```text
old = gen 1, slot 8
free(old)
new = gen 2, slot 8
```

虽然 `old` 和 `new` 使用同一个 slot，但完整 FSID 不同。
因此：

- `fstable` 以完整 `fsc_fsid_t` 建索引，旧 FSID 查不到新 namespace。
- `fsid_free(old)` 会因为 generation 不匹配而失败。
- 双重释放会因为 slot 未分配而失败。

`fsid_is_valid()` 只检查 FSID 的结构编码是否合法：

```text
fsid != 0
slot in range
generation != 0
```

它不检查该 FSID 当前是否 live。live / stale 判断只在 `fsid_free()` 等需要分配器状态的路径中完成。

---

# 6. API

## fsid_init()

初始化 FSID 分配器。
所有 slot 的 generation 初始化为 `1`，所有 slot 进入 free-list。

## fsid_deinit()

销毁 FSID 分配器。
释放锁资源并清空内部状态。

## fsid_alloc()

分配一个新的 `fsc_fsid_t`。

参数：

```text
[OUT] fsid
```

返回：

```text
FS_OK
fs_error_t
```

当 free-list 为空时返回 `ENOSPC`。

## fsid_free()

释放一个 `fsc_fsid_t`。

成功释放后，slot 会被复用，generation 会递增。
如果传入 stale FSID、重复释放 FSID 或非法 FSID，则返回错误。

## fsid_is_valid()

判断 FSID 编码是否有效。

## fsid_hash()

为 `fstable` 的 FSID 索引提供 hash 值。

---

# 7. 与其它模块关系

```text
FSMgr
  ├── fsid_alloc()
  └── fsid_free()

FSTable
  └── fsid_hash()

Namespace
  └── 保存 fsc_fsid_t
```

---

# 8. 设计总结

FSID 模块是 FSC 的身份基础。
当前实现已经从单调递增分配器升级为可复用、带 generation、防 stale 的分配器。
外部 API 保持不变，FSMgr / FSTable / Namespace 不需要理解 slot、free-list 或 generation 的内部细节。
