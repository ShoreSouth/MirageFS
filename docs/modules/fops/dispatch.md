# FOPS Dispatch 统一入口

`fops_dispatch(args)` 是 FOPS 面向上层的统一入口。上层只需要构造 `fops_args_t`，填写 `op`、公共字段和对应 union 分支，再交给 dispatch 执行。

## 调用流程

```text
fops_dispatch(args)
  ↓
fops_validate_args(args)
  ↓
查 fops_op_spec_t
  ↓
校验 op / flag / 基础指针 / name 规则
  ↓
g_fops_ops[op](args)
  ↓
调用现有 fops_lookup/create/... 细粒度函数
```

该流程把“入口统一”和“实现可维护”分开：外部看到统一函数指针模型，内部仍保留清晰的单 OP 实现。

## fops_args_t 设计原则

`fops_args_t` 的设计目标是避免外部接口随着 OP 参数膨胀而失控。

公共字段适合放在外层：

- `op`：操作字。
- `flags`：OP flag。
- `parent_fuid`：大多数 name-based OP 的父目录。
- `target_fuid`：直接作用于对象的 OP。
- `name`：目录项名称或 xattr 名称。
- `out_attr` / `out_fuid` / `out_count` 等常见输出指针。

差异较大的参数放在 union 分支中，例如 create、rename、rw、xattr、statfs 等。

## OP spec 表

`fops_op_spec_t` 是 dispatch 的规则源，至少描述：

- OP 是否存在实现。
- 允许的 flag 集合。
- 互斥 flag 集合。
- 是否需要 parent/name/target。
- name 是否允许 `.` 和 `..`。
- 是否需要输入缓冲区、输出缓冲区或请求结构体。

这样做的好处是：新增 OP 时先补 spec，再补实现；调用规则可以集中审查，不需要散落在每个 OP 中反复写相同判断。

## 细粒度接口

保留 `fops_lookup()`、`fops_create()` 等细粒度接口是必要的。它们用于：

- dispatch 内部复用。
- 单元测试直接覆盖某个 OP。
- 模块内部轻量调用，避免必须构造完整 args。

约定：细粒度接口也必须遵守同一语义，不允许绕开核心安全检查产生不一致行为。
