# FUID 设计文档（File Unique ID）

## 1. 背景与目标

在 MirageFS 中，需要一种**稳定、统一、可扩展的对象标识符**，用于：

* 唯一标识文件系统中的任意对象（文件/目录/链接等）
* 支持跨模块（FSA / FSIO / Cache / Audit）传递
* 支持 handle 打开（open_by_handle_at）等能力
* 支持 snapshot、qtree、多租户等扩展能力

因此设计 **FUID（File Unique ID）**。

---

## 2. 设计原则

FUID 设计遵循以下原则：

### 2.1 唯一性

在文件系统范围内，必须唯一标识一个对象：

```text
唯一标识 = (fsid, objectid, gen)
```

---

### 2.2 稳定性

* FUID 一旦生成，不应被修改
* 对象删除后，旧 FUID 可能变为 **stale（失效）**

---

### 2.3 分层设计

FUID 包含两类信息：

| 类型           | 说明       |
| ------------ | -------- |
| Identity（身份） | 标识“它是谁”  |
| View（视图）     | 标识“怎么看它” |

---

### 2.4 轻量化

* 固定长度结构体
* 可直接序列化（网络传输 / RPC）

---

### 2.5 可扩展性

通过 flags / version 支持未来能力扩展

---

## 3. 结构定义

```c
typedef struct {
    /* ---------- Identity（核心唯一标识） ---------- */
    uint32_t fsid;        // 文件系统ID
    uint64_t objectid;    // 对象唯一标识（类似 inode）
    uint32_t gen;         // generation（防复用）

    /* ---------- 基本属性（轻量 hint） ---------- */
    uint8_t  type;        // 文件类型（REG/DIR/...）

    /* ---------- View（访问上下文） ---------- */
    uint32_t qtreeid;     // 租户 / 子卷
    uint32_t snapid;      // 快照ID（0表示当前）
    uint16_t shardid;     // 分片ID

    /* ---------- 扩展 ---------- */
    uint16_t flags;       // 标志位
    uint32_t version;     // 结构版本

} Fuid_t;
```

---

## 4. 字段说明

### 4.1 Identity（核心字段）

#### fsid

* 文件系统唯一标识
* 由系统内部维护（不可依赖 mount_id）

---

#### objectid

* 对象唯一 ID（类似 inode number）
* 在 fs 内唯一

---

#### gen（generation）

* 用于防止 objectid 复用问题
* 当对象删除并复用 objectid 时，gen 必须递增

```text
(obj_id, gen) 唯一标识一个历史对象
```

---

### 4.2 type（类型）

* 表示对象类型（REG/DIR/SYMLINK等）
* **属于 hint 信息，不是权威来源**

用途：

* 避免频繁 stat
* 优化 readdir / lookup / cache

注意：

* 可能过期
* 不可用于强校验

---

### 4.3 View（视图字段）

这些字段不参与对象唯一性，只影响访问方式

---

#### snapid

* 表示快照版本
* 同一个 objectid 在不同 snapid 下数据可能不同

```text
objectid 相同 + snapid 不同 = 不同视图
```

---

#### qtreeid

* 表示租户 / 子卷 / 配额管理单元
* 用于逻辑隔离

---

#### shardid

* 用于大目录 / 大文件分片
* 表示访问的局部视图

---

### 4.4 扩展字段

#### flags

用于扩展功能，例如：

```c
#define FUID_FLAG_CLONE     (1 << 0)
#define FUID_FLAG_ENCRYPT   (1 << 1)
#define FUID_FLAG_COMPRESS  (1 << 2)
```

---

#### version

* 用于结构演进
* 保证向后兼容

---

## 5. 核心语义

### 5.1 Identity vs View

```text
Identity:
    (fsid, objectid, gen)
    → 决定“它是谁”

View:
    (snapid, qtreeid, shardid)
    → 决定“怎么看它”
```

---

### 5.2 FUID 生命周期

FUID 是“引用”，而不是“对象本身”：

```text
对象删除后：
    FUID 仍可能存在（成为 stale）

新对象：
    必须分配新的 objectid 或 gen
```

---

### 5.3 stale FUID

当发生以下情况：

* 对象被删除
* inode 被复用
* gen 不匹配

该 FUID 应被视为：

```text
ESTALE（失效引用）
```

---

## 6. 与 Stat 的关系

| 属性           | FUID.type | stat.mode |
| ------------ | --------- | --------- |
| 性质           | hint      | 权威        |
| 是否可能过期       | 是         | 否         |
| 是否需要 syscall | 否         | 是         |

---

设计原则：

```text
FUID.type → 用于优化
stat.mode → 用于校验
```

---

## 7. 与 file_handle 的关系

FUID ≠ file_handle

### 转换链路：

```text
FUID
 → ObjMeta（path / parent+name）
 → name_to_handle_at
 → file_handle
 → open_by_handle_at
 → fd
```

---

### 设计原则：

* file_handle 不作为唯一标识
* file_handle 可缓存，但必须可失效
* 必须通过 gen 校验正确性

---

## 8. 使用建议

### 8.1 open

```text
FUID → handle → open_by_handle_at → fd
→ fstat → 校验 gen
```

---

### 8.2 readdir

返回：

```text
name + FUID + type
```

避免额外 stat

---

### 8.3 cache

建议实现：

* HandleCache（file_handle 缓存）
* StatCache（属性缓存）

---

## 9. 设计总结

FUID 是 MirageFS 中的核心抽象：

```text
它不是“文件本身”，而是“文件引用”
```

具备以下能力：

* 唯一标识对象
* 支持跨模块传递
* 支持 snapshot / qtree / shard
* 支持 handle 打开链路
* 支持 stale 检测

---

## 10. 后续扩展方向

* FUID → inode 索引映射
* 分布式场景下全局唯一（GFID）
* snapshot 语义增强
* cache 一致性策略
* ESTALE 处理机制完善

---
