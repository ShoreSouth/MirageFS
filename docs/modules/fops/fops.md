# FOPS 模块总纲

FOPS（File Operations）是 MirageFS 的单步文件操作层。它向上提供统一的文件操作入口，向下组合 Object、FSC 和 LSA 能力，负责把上层请求转换为清晰、可校验、可追踪的文件系统语义。

本文只保留总纲和导航；每类 OP 的参数、flag、返回语义和实现约束放在同目录专题文档中。

## 模块定位

FOPS 位于 NAMEI 和底层对象/文件系统上下文之间：

```text
NAMEI / RUNTIME / MSH
        ↓
      FOPS
        ↓
  Object / FSC / LSA
```

职责边界：

- FOPS 负责单步文件操作语义，例如 lookup、create、mkdir、unlink、rename、readdir、read/write、attr、xattr 等。
- FOPS 不负责路径解析；路径拆分、父目录定位和 basename 组织属于 NAMEI。
- FOPS 不负责文件系统实例生命周期；namespace、fsid、fstable、sysroot 属于 FSC。
- FOPS 不直接表达 Linux syscall 细节；后端访问应通过 LSA 封装。
- FOPS 可以使用 Object 模块表达对象身份、元数据、运行时对象和对象表。

## 统一入口

正式上层模块应通过：

```c
fops_dispatch(fops_args_t *args)
```

进入 FOPS。

`fops_dispatch()` 负责：

1. 校验 `args` 和 `op`。
2. 根据 `op` 查询操作规格。
3. 校验参数、name、flag、类型和属性约束。
4. 分发到具体操作实现。
5. 保持错误码、日志和返回语义一致。

细粒度 API 可以保留给 FOPS 内部、兼容层和 UT 使用，但 NAMEI、SERVER、CLI、MSH 等正式调用方不应绕过 `fops_dispatch()` 直接调用 `fops_create_plus()`、`fops_mkdir_plus()`、`fops_readlink()` 这类接口。

## 目录结构

```text
src/fops/
  include/
    fops.h
    fops_types.h
  internal/
    fops_error.h
    fops_internal.h
  core/
    fops_dispatch.c
    fops_error.c
    fops_helper.c
    fops_init.c
    fops_spec.c
  ops/
    fops_attr.c
    fops_create.c
    fops_fs.c
    fops_handle.c
    fops_link.c
    fops_lookup.c
    fops_mkdir.c
    fops_mknod.c
    fops_readdir.c
    fops_rename.c
    fops_rmdir.c
    fops_rw.c
    fops_unlink.c
    fops_xattr.c
```

## 文档导航

- [dispatch](dispatch.md)：统一入口、参数校验和分发规则。
- [flags](flags.md)：FOPS flag 语义和合法组合。
- [errors](errors.md)：FOPS 错误码和 sub 错误约定。
- [identity](identity.md)：FUID、对象身份和子对象身份派生。
- [handle](handle.md)：Object handle 与 LSA handle 转换边界。
- [lookup](lookup.md)：lookup 类操作。
- [create](create.md)：create 类操作。
- [mkdir](mkdir.md)：mkdir 类操作。
- [mknod](mknod.md)：mknod 类操作。
- [unlink](unlink.md)：unlink 类操作。
- [rmdir](rmdir.md)：rmdir 类操作。
- [rename](rename.md)：rename 类操作。
- [readdir](readdir.md)：readdir/readdirplus 操作。
- [rw](rw.md)：read/write 操作。
- [attr](attr.md)：getattr/setattr 和属性转换。
- [xattr](xattr.md)：扩展属性操作。
- [link](link.md)：link/symlink/readlink 操作。
- [fs](fs.md)：文件系统级操作。

## 设计约束

- 所有可能失败的 FOPS public/internal 操作应返回 `fs_error_t`。
- 不把 Linux `errno`、`0`、`-1` 与 `fs_error_t` 混用。
- 参数边界必须在模块入口或公共 helper 中显式校验。
- 下层已经返回 `fs_error_t` 时直接向上传播，不重新包装。
- 错误日志应包含 `fs_error_str(err)` 和原始十六进制错误码。
- 新增 OP 时必须同步更新规格表、UT、模块文档和必要的指导手册。

## UT 要求

FOPS 相关 UT 位于：

```text
tests/fops/test_fops.c
```

新增 FOPS case 时必须分配稳定编号，编号规则见：

```text
docs/testing/ut-framework.md
```

常用检查命令：

```sh
tools/test/list-ut.py --module fops
tools/test/list-ut.py --check
tools/test/run-ut.sh fops
```
