# FOPS Dispatch 设计

`fops_dispatch(args)` 是 FOPS 的统一入口。它负责把上层构造的 `fops_args_t` 请求转换为具体 FOPS 操作，并在分发前完成公共校验。

## 入口职责

`fops_dispatch()` 的职责包括：

1. 校验 `args` 是否为空。
2. 校验 `args->op` 是否是合法 `fs_op_t`。
3. 查询当前 OP 对应的 `fops_op_spec_t`。
4. 根据规格校验必填字段、name、flag、类型和属性。
5. 调用具体 `fops_do_*` 或对应操作实现。
6. 保持错误码、日志和返回语义一致。

上层正式调用方应优先使用 `fops_dispatch()`，避免直接调用细粒度 FOPS API。

## 参数模型

FOPS 请求统一放在：

```c
fops_args_t
```

该结构体表达一次单步文件操作需要的上下文，例如：

- `op`：操作类型。
- `parent`：父目录对象或父对象身份。
- `name`：当前操作的 basename。
- `target`：目标对象或目标路径相关信息。
- `flags`：操作 flag。
- `attr`：create/setattr 等操作需要的属性。
- `out`：输出结果。

具体字段以 `src/fops/include/fops_types.h` 为准。

## 操作规格

每个 OP 应在规格表中描述自己的约束。规格通常包括：

- 是否需要 parent。
- 是否需要 name。
- 是否允许 `.` 或 `..`。
- 允许的 flag 集合。
- 冲突的 flag 集合。
- 期望的对象类型。
- 是否需要 create attr。

这样可以把公共校验集中在 dispatch 路径，避免每个操作重复写散落的参数检查。

## 分发流程

推荐流程：

```text
fops_dispatch(args)
        ↓
fops_op_spec_get(args->op)
        ↓
fops_validate_args(args, spec)
        ↓
fops_validate_flags(args, spec)
        ↓
fops_validate_name(args, spec)
        ↓
具体 FOPS 操作实现
```

如果任一校验失败，应返回 FOPS 模块错误码，并在必要时记录日志。

## 错误处理

Dispatch 层应遵守：

- `args == NULL` 返回 `FS_MODULE_FOPS / EINVAL`。
- 非法 `op` 返回 `FS_MODULE_FOPS / EINVAL`。
- 参数缺失、flag 冲突、类型不匹配等统一返回结构化 `fs_error_t`。
- 下层已经返回 `fs_error_t` 时直接向上传播。
- 不使用裸 `-1` 或 Linux `errno` 作为 FOPS 内部返回值。

## 调用边界

允许：

- NAMEI 构造 `fops_args_t` 后调用 `fops_dispatch()`。
- RUNTIME/MSH 通过 NAMEI 或正式封装间接进入 FOPS。
- UT 直接调用细粒度 helper 构造边界场景。

不建议：

- 上层正式模块直接调用 `fops_create_plus()`、`fops_mkdir_plus()` 等细粒度接口。
- FOPS 绕过 FSC/Object/LSA 的边界直接承担其他模块职责。
- 在具体操作中重复实现 dispatch 已经覆盖的公共校验。

## UT 覆盖点

Dispatch 相关 UT 应覆盖：

- `NULL args`。
- 非法 `op`。
- 规格表查询。
- 缺失必填字段。
- 非法 name。
- 不支持或互斥 flag。
- 类型约束不匹配。

当前 case 可通过下面命令检索：

```sh
tools/test/list-ut.py --module fops --name dispatch
tools/test/list-ut.py --list 0x06021000
```
