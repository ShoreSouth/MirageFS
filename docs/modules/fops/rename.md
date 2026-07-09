# Rename 操作

rename 负责把一个目录项移动或改名到另一个父目录下。

## 参数语义

必要输入：

- `old_parent_fuid`：源父目录。
- `old_name`：源名称。
- `new_parent_fuid`：目标父目录。
- `new_name`：目标名称。
- `flags`：替换控制标志。

## flag 语义

rename 推荐支持：

- `FS_FLAG_REPLACE`：目标存在时允许替换。
- `FS_FLAG_EXCLUSIVE`：目标存在时失败。

`REPLACE` 与 `EXCLUSIVE` 互斥。

## 原子性

rename 应尽量保持底层 rename 的原子语义。跨目录移动时，FOPS 不应拆成 unlink + create 的非原子组合，除非后端明确不支持且上层可接受弱语义。

## 目录约束

目录 rename 需要额外处理：

- 不能把目录移动到自身子树中。
- 目标存在且为非空目录时应失败。
- 移动目录后 `..` 语义必须正确。
