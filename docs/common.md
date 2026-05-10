# Common Module Design (MirageFS)

## 1. 模块定位

Common 模块是 MirageFS 的**基础语义与工具层**，用于提供全系统可复用的：

- 基础类型定义（fs_types）
- 错误码体系（fs_error）
- 日志系统（fs_log）
- 断言与调试机制（fs_assert）
- 通用工具函数（fs_utils）
- 编译期辅助宏（fs_macros）

### 核心原则

> Common ≠ 工具箱  
> Common = “系统统一语义 + 最低层基础能力”

---

## 2. 设计目标

### 2.1 统一性
所有模块必须统一使用：
- 类型定义（fsid_t / inodeid_t / fs_type_t）
- 错误码（fs_err_t）
- 日志接口（FS_LOGE / FS_LOGI）

---

### 2.2 无业务依赖
Common 不允许依赖任何上层模块：

❌ 禁止依赖：
- CLI
- VFS
- LSA
- OID

✔ 允许依赖：
- 标准 C library
- POSIX 基础头文件

---

### 2.3 轻量原则
Common 不引入：
- 数据结构库（list / tree / hash）
- IO逻辑
- 文件系统逻辑

---

## 3. 目录结构

```bash
src/common/
├── fs_types.h # 基础类型与文件系统语义
├── fs_error.h # 全局错误码体系
├── fs_log.h # 日志系统接口
├── fs_assert.h # 断言与调试机制
├── fs_utils.h # 通用工具函数（inline）
├── fs_macros.h # 编译期宏
└── internal/
└── fs_log.c # log实现
```

---

## 4. 模块职责说明

### 4.1 fs_types.h

职责：
- 定义文件系统基础语义类型
- 封装 fs_type_t（REG/DIR/LNK等）
- 提供 mode 与 fs_type 转换

关键内容：
- fsid_t
- inodeid_t
- fs_type_t
- mode 封装宏

---

### 4.2 fs_error.h

职责：
- 定义统一错误码体系
- 对齐 Linux errno 风格
- 提供 fs_err_t 类型

原则：
- 所有模块返回值必须使用 fs_err_t 或兼容 errno

---

### 4.3 fs_log.h

职责：
- 提供统一日志接口
- 支持 log level（ERROR/WARN/INFO/DEBUG）
- 提供文件 + 行号信息

日志分级：
- FS_LOG_ERROR
- FS_LOG_WARN
- FS_LOG_INFO
- FS_LOG_DEBUG

---

### 4.4 fs_assert.h

职责：
- 提供断言机制
- 支持 debug / release 行为控制
- 支持返回型断言 / goto断言

设计目标：
- debug：crash + log
- release：return error + log

---

### 4.5 fs_utils.h

职责：
- 提供轻量通用工具函数
- 无状态、无依赖业务逻辑

包含内容：
- align_up / align_down
- range overlap / trim
- safe string helper
- debug dump helper

原则：
> 只放“小工具”，不放“系统逻辑”

---

### 4.6 fs_macros.h

职责：
- 编译期辅助宏
- 性能优化宏
- 类型/位操作宏

包含内容：
- FS_MIN / FS_MAX
- FS_ARRAY_SIZE
- FS_BIT / flag操作
- container_of
- likely / unlikely

---

## 5. 演进策略

Common 模块允许逐步演进，但必须遵循：

✔ 可以增加：
- 新的基础类型
- 新的工具函数
- 新的日志能力

❌ 不可以增加：
- 文件系统业务逻辑
- FUID / inode / handle 管理逻辑

## 6. 设计总结

Common 模块的本质是：

> “为整个 MirageFS 提供统一语言，而不是提供功能实现”