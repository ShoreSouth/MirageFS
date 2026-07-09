# FOPS 错误语义

FOPS 对外统一返回 `fs_error_t`。内部可以调用 LSA、ObjMgr、FSC 等模块，但不能把底层 `errno`、`0/-1` 或模块私有错误直接泄漏给上层。

## 基本规则

- 成功返回 `FS_OK` 或项目约定的成功码。
- 参数非法返回 invalid argument 类错误。
- 对象不存在返回 not found 类错误。
- 目标已存在返回 already exists 类错误。
- 权限不足返回 permission denied 类错误。
- 后端不支持返回 unsupported 类错误。
- I/O 失败返回 I/O 类错误。

## errno 映射

FOPS 应通过统一转换函数把 LSA 的 `lsa_ret_t` 或 errno 映射成 `fs_error_t`。

常见映射建议：

| errno | FOPS 语义 |
| --- | --- |
| `ENOENT` | 对象或目录项不存在 |
| `EEXIST` | 目标已存在 |
| `ENOTDIR` | 路径中需要目录但不是目录 |
| `EISDIR` | 目标是目录但 OP 要求非目录 |
| `ENOTEMPTY` | 目录非空 |
| `EACCES` / `EPERM` | 权限不足 |
| `EINVAL` | 参数或 flag 组合非法 |
| `EOPNOTSUPP` / `ENOTSUP` | 后端不支持 |
| `ERANGE` | 缓冲区过小或范围不足 |
| `EIO` | 底层 I/O 错误 |

## dispatch 校验错误

dispatch 阶段发现的问题应早返回，不进入具体 OP：

- `args == NULL`。
- `op` 越界或未注册实现。
- flag 包含未知位。
- flag 组合互斥。
- 必需指针为空。
- name 为空、过长或包含当前 OP 不允许的特殊值。

## plus 接口错误

`*_plus` 接口如果主操作成功但属性读取失败，需要明确策略：

- 推荐默认整体失败，避免上层拿到“创建成功但属性无效”的半成功结果。
- 如果未来需要弱语义，应在返回结构中增加属性有效位，而不是偷偷返回未初始化属性。
