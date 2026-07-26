# 模块地图

本文给出 MirageFS 的模块地图，帮助快速理解各目录职责和依赖方向。更详细的 API、结构体和错误码说明见 `docs/modules/`。

## 总体调用方向

MirageFS 当前主要调用链：

```text
APP / CLI / SERVER
        ↓
      RUNTIME
        ↓
      NAMEI
        ↓
      FOPS
        ↓
  OBJMGR / FSC
        ↓
       LSA
        ↓
 Linux Kernel / Backend FS
```

依赖方向只能自上而下。下层模块不能反向依赖上层模块，也不要跨层偷调上层语义。

## 目录概览

```text
src/
  app/       主程序入口
  config/    配置加载和默认配置
  common/    通用基础设施
  lsa/       Linux syscall/backend fs 访问封装
  object/    对象身份、元数据、运行时实例和对象管理
  fsc/       文件系统实例、namespace、fsid、sysroot
  fops/      单步文件操作语义
  namei/     路径解析和 FOPS 参数组织
  runtime/   运行时会话、初始化和对上入口
  msh/       MirageFS shell/parser/repl
```

## common

`common` 提供通用基础设施：

- 错误码：`fs_error_t`、errno、module/sub 注册
- 日志：`FS_LOG_DUMP_*`
- 路径：join、normalize 等 helper
- 容器：hash、list
- 并发：lock、atomic
- 内存：mempool
- 性能：metrics counter、latency histogram、snapshot
- 调试：trace、assert

边界约束：

- 不能放业务语义。
- 不能依赖 object/fsc/fops/namei 等上层模块。
- 错误码和日志设施被其他模块复用，因此变更要谨慎。

## config

`config` 负责项目配置加载和默认值管理。

当前适合关注：

- 默认配置是否稳定。
- 重复初始化是否幂等。
- 配置缺失时是否有合理 fallback。

## lsa

`lsa` 是 Linux Storage Adapter，负责封装 Linux syscall 和后端文件系统访问。

典型职责：

- 文件和目录操作
- stat/attr/xattr
- handle 转换
- namespace/backend path 访问

边界约束：

- 不包含 VFS、FOPS、NAMEI 的上层语义。
- 错误应映射为 `fs_error_t`，不要让 Linux `errno` 随意穿透到上层。

## object

`object` 负责 MirageFS 对象模型：

- `fuid_t`：文件系统内对象身份
- `obj_key_t`：对象索引键
- `obj_meta_t`：对象元数据和后端定位信息
- `obj_runtime_t`：运行时对象实例、状态和引用
- `obj_table_t`：对象表
- `obj_pool_t`：对象池
- `objmgr`：对象生命周期管理

理解 object 时要区分：

- `obj_meta_t` 描述对象是什么。
- `obj_runtime_t` 描述对象实例当前处于什么运行状态。

## fsc

`fsc` 负责文件系统级上下文：

- `fsid`
- `namespace`
- `nspool`
- `fstable`
- `fsmgr`
- `sysroot`

它回答的问题是：当前有哪些文件系统实例、根在哪里、namespace 如何组织。

边界约束：

- 不直接表达单步文件操作语义。
- 不替代 object 管理对象生命周期。

## fops

`fops` 负责单步文件操作语义，例如：

- lookup
- create
- mkdir
- mknod
- unlink
- rmdir
- rename
- readdir
- read/write
- attr/xattr
- link/readlink

正式上层调用应通过：

```c
fops_dispatch()
```

细粒度 FOPS API 可以保留给 FOPS 内部、兼容层和 UT 使用。NAMEI、SERVER、CLI 等正式调用方不应绕过统一 dispatch 入口直接调用细粒度函数。

## namei

`namei` 负责路径解析和 FOPS 参数组织。

它回答的问题是：

- 路径从哪个 root/cwd 开始解析。
- 父目录是谁。
- basename 是什么。
- 应该构造什么 FOPS 请求。

边界约束：

- 不直接执行底层文件操作。
- 不管理对象生命周期。
- 不创建或销毁 namespace。

## runtime

`runtime` 是当前对上层使用者更友好的运行时入口。

典型职责：

- 初始化和反初始化项目运行环境
- 管理 session
- 保存 root/cwd 等运行状态
- 对外提供操作封装
- 管理 Run/filesystem session 性能聚合、内存历史和退出报告

它适合成为 app、shell 或未来服务入口调用的第一层。

## msh

`msh` 是 MirageFS shell 相关模块：

- 命令解析
- REPL
- 文件/目录/元数据命令
- 参数读取和默认值处理

学习时可以把 `msh` 看作上层使用者，它不应该承载底层文件系统语义。

## app

`app` 是主程序入口，目前主要负责启动 MirageFS。

如果需要理解完整路径，可以从 `src/app/main.c` 开始，但如果目标是理解核心文件系统语义，更推荐从 `runtime`、`namei`、`fops` 往下读。

## 测试对应关系

模块和 UT 文件对应：

```text
config   -> tests/config/test_config.c
common   -> tests/common/test_common.c
lsa      -> tests/lsa/test_lsa.c
object   -> tests/object/test_object.c
fsc      -> tests/fsc/test_fsc.c
fops     -> tests/fops/test_fops.c
namei    -> tests/namei/test_namei.c
runtime  -> tests/runtime/test_runtime.c
msh      -> tests/msh/test_msh.c
```

新增模块时，应同步补模块文档、UT 文件和本模块地图。
