# FOPS 模块总纲

FOPS（File Operations）是 MirageFS 的单步文件操作层。它向上提供统一的文件操作入口，向下组合 ObjMgr、FSC 与 LSA 能力，负责把上层 OP 请求转换为清晰、可校验、可追踪的文件系统语义。

本文档只保留总纲与导航；每类 OP 的参数、flag、返回语义和实现约束放在同目录的专题文档中。

## 模块定位

FOPS 的核心职责：

- 提供统一入口 `fops_dispatch(args)`，也保留细粒度 C API 便于内部复用和单测覆盖。
- 使用 `fops_args_t` 承载所有 OP 参数，公共字段放在外层，差异字段放在 union 中。
- 使用 `fops_op_spec_t` 描述每个 OP 的参数规则、flag 白名单、flag 冲突和基础校验规则。
- 以 `obj_fuid_t` 作为对象身份，objectid/gen 由 MirageFS 的对象体系分配和管理。
- 对目录项、属性、句柄、读写、xattr、fs 级操作提供单步语义封装。

## 分层关系

```text
上层调用者
  ↓
FOPS dispatch / spec / validate
  ↓
FOPS 细粒度 OP
  ↓
FSC / ObjMgr / LSA
  ↓
Linux / 后端文件系统
```

FOPS 不直接承担全局文件系统注册职责；文件系统实例和命名空间仍由 FSC 管理。FOPS 只在执行 OP 时消费 FSC 提供的上下文。

## 文档目录

- [dispatch.md](dispatch.md)：统一入口、参数结构、OP 表和校验流程。
- [identity.md](identity.md)：FUID、objectid、gen 与对象生命周期。
- [lookup.md](lookup.md)：lookup / lookup_plus。
- [create.md](create.md)：create / create_plus。
- [mkdir.md](mkdir.md)：mkdir / mkdir_plus。
- [mknod.md](mknod.md)：mknod / mknod_plus。
- [unlink.md](unlink.md)：unlink。
- [rmdir.md](rmdir.md)：rmdir。
- [rename.md](rename.md)：rename。
- [link.md](link.md)：link / link_plus / symlink / symlink_plus。
- [attr.md](attr.md)：getattr / setattr / access / truncate。
- [readdir.md](readdir.md)：readdir / readdirplus。
- [handle.md](handle.md)：open / openhandle / close / gethandle。
- [rw.md](rw.md)：read / write / pread / pwrite。
- [xattr.md](xattr.md)：扩展属性操作。
- [fs.md](fs.md)：statfs / syncfs。
- [flags.md](flags.md)：flag 总表和各 OP 支持矩阵。
- [errors.md](errors.md)：错误码映射与返回约定。

## 源码对应关系

```text
src/fops/
  core/      统一入口、OP spec、参数校验、错误映射、辅助逻辑
  include/   对外头文件 fops.h / fops_types.h
  internal/  FOPS 内部接口
  ops/       各类 OP 的具体实现
```

专题文档按 `src/fops/ops/` 的文件划分为主，只有 dispatch、identity、flags、errors 属于横切说明。

## 当前约定

- 文档、注释和设计说明以中文为主，必要英文术语保留英文原词。
- 对外统一入口优先走 `fops_dispatch()`；细粒度接口仍作为内部实现单元和轻量调用入口。
- 创建类 OP 同时提供轻量接口和 `*_plus` 接口；`*_plus` 在创建成功后返回属性，减少上层额外 getattr。
- `lookup_plus` 返回目录项解析结果和属性；轻量 `lookup` 只返回对象身份。
- `readdir` 只返回目录项基础信息；`readdirplus` 返回目录项和属性。
- flag 语义由 `common/flag` 定义，FOPS 通过 spec 表限制每个 OP 可接受的 flag 集合。
