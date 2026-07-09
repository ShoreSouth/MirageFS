# Readdir 操作

readdir 类操作负责枚举目录项。

## 接口

- `fops_readdir()`：返回目录项基础信息。
- `fops_readdirplus()`：返回目录项基础信息和属性。

两者应复用同一个内部基础枚举流程，区别只在是否解析属性。

## 参数语义

必要输入：

- `target_fuid`：目录对象。
- `offset`：枚举起点。
- `max_entries`：最多返回项数。
- `flags`：枚举控制标志。

输出：

- `entries`：目录项数组。
- `out_count`：实际返回数量。
- `next_offset`：下一次枚举起点。

## flag 语义

readdir 推荐支持：

- `FS_FLAG_DIRECTORY`：要求目标是目录。

## readdir 与 readdirplus

`readdir` 适合只需要文件名列表的场景；`readdirplus` 适合 UI 展示、同步扫描、缓存预热等需要属性的场景。

`readdirplus` 不应只是 `readdir` 的别名。它必须在返回结构中带上有效 stat/attr 信息，或明确标记该项属性不可用。

## 性能注意

readdirplus 可能导致大量 getattr。实现时应优先利用底层 `d_type`、批量 stat 或缓存能力，避免每个目录项都无条件触发昂贵路径。
