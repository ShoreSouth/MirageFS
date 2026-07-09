# FOPS Flag 语义

FOPS 使用 `common/flag` 中的 OP flag，并通过 `fops_op_spec_t` 为每个 OP 定义允许集合。

## 通用原则

- 未在 OP spec 中声明的 flag 必须拒绝。
- 互斥 flag 必须在 dispatch 校验阶段拒绝。
- flag 只表达控制语义，不表达对象身份或复杂请求体。
- 对创建类 OP，类型、mode、dev 等复杂字段应放在 attr 或专用 req 中。

## 常用 flag 推荐语义

| flag | 推荐语义 |
| --- | --- |
| `FS_FLAG_EXCLUSIVE` | 目标已存在时失败 |
| `FS_FLAG_REPLACE` | 要求替换或要求目标已存在，具体按 OP 定义 |
| `FS_FLAG_NOFOLLOW` | 不跟随末端符号链接 |
| `FS_FLAG_DIRECTORY` | 要求目标是目录，或声明操作目录对象 |
| `FS_FLAG_REGULAR` | 要求目标是普通文件 |
| `FS_FLAG_READ` | 请求读权限或读打开模式 |
| `FS_FLAG_WRITE` | 请求写权限或写打开模式 |
| `FS_FLAG_APPEND` | 写入追加到文件末尾 |
| `FS_FLAG_TRUNCATE` | 创建/打开时截断已有文件 |
| `FS_FLAG_SYNC` | 请求同步语义 |
| `FS_FLAG_DIRECT` | 请求直接 I/O 倾向 |

## OP 支持矩阵

| OP | 支持 flag | 互斥关系 |
| --- | --- | --- |
| lookup | NOFOLLOW, DIRECTORY, REGULAR | DIRECTORY / REGULAR |
| create | REPLACE, EXCLUSIVE, NOFOLLOW, SYNC, DIRECT, REGULAR, TRUNCATE, APPEND | REPLACE / EXCLUSIVE |
| mkdir | EXCLUSIVE, DIRECTORY | 无 |
| mknod | EXCLUSIVE | 无 |
| unlink | NOFOLLOW, REGULAR | 无 |
| rmdir | DIRECTORY | 无 |
| rename | REPLACE, EXCLUSIVE | REPLACE / EXCLUSIVE |
| link | EXCLUSIVE | 无 |
| symlink | EXCLUSIVE | 无 |
| open/openhandle | READ, WRITE, SYNC, DIRECT, APPEND, TRUNCATE, DIRECTORY, REGULAR | DIRECTORY / REGULAR |
| readdir/readdirplus | DIRECTORY | 无 |
| getattr/setattr/access/truncate | DIRECTORY, REGULAR | DIRECTORY / REGULAR |
| read/write/close/gethandle | 无 | 无 |
| getxattr/removexattr/listxattr | 无 | 无 |
| setxattr | REPLACE, EXCLUSIVE | REPLACE / EXCLUSIVE |
| statfs/syncfs | 无 | 无 |

该矩阵应与 `src/fops/core/fops_spec.c` 保持一致。新增 flag 时，必须同时更新 spec、专题文档和必要测试。
