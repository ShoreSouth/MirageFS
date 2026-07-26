# Attr 操作

attr 类操作负责对象属性读取、修改、权限检查和截断。

## getattr

`fops_getattr()` 读取对象当前属性。

必要输入：

- `target_fuid`：目标对象。
- `flags`：属性读取控制标志。

输出：

- `out_attr`：当前属性。

稳定属性快照包括：

- 类型、mode、uid、gid、size 和硬链接数。
- 已分配块数和首选 IO 块大小。
- atime、mtime、ctime 的秒与纳秒部分。
- 后端支持时的 birth time；调用方必须先检查 `btime_valid`。

Linux 后端优先通过 LSA `statx` 获取扩展属性。内核不支持 `statx`
时允许降级为 `fstat`，此时 birth time 无效。FOPS 不向上暴露
Linux `struct stat` 或 `struct statx`。

推荐 flag：

- `FS_FLAG_DIRECTORY`：要求目标是目录。
- `FS_FLAG_REGULAR`：要求目标是普通文件。

## setattr

`fops_setattr()` 修改对象属性。

必要输入：

- `target_fuid`：目标对象。
- `attr`：待修改属性。
- `valid_mask`：声明 attr 中哪些字段有效。
- `flags`：修改控制标志。

setattr 不应把未声明有效的字段写回，避免调用者栈上的默认值误覆盖真实属性。

## access

`fops_access()` 检查调用上下文对对象是否具备指定权限。

必要输入：

- `target_fuid`：目标对象。
- `mode`：访问模式，例如读、写、执行。

access 只做检查，不改变对象状态。

## truncate

`fops_truncate()` 修改普通文件大小。

必要输入：

- `target_fuid`：目标对象。
- `size`：目标大小。

目录、符号链接和不支持截断的特殊文件应返回明确错误。

## 实现约束

FOPS 属性字段应与 LSA stat 结果有稳定转换关系，但不直接向上暴露底层 `struct stat`，避免后续支持非 Linux 后端时接口失控。
