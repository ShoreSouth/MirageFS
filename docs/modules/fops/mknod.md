# Mknod 操作

mknod 类操作负责创建特殊文件节点，例如 FIFO、字符设备、块设备等。

## 接口

- `fops_mknod()`：创建特殊节点并返回对象身份。
- `fops_mknod_plus()`：创建特殊节点并返回对象身份和属性。

## 参数语义

mknod 参数比 create/mkdir 更复杂，因此应使用专门请求结构体，例如 `fops_mknod_req_t`。

必要输入：

- `parent_fuid`：父目录对象。
- `name`：新节点名。
- `req`：节点创建请求，包含类型、mode、dev 等字段。
- `flags`：创建控制标志。

输出：

- `out_fuid`：新节点对象身份。
- `out_attr`：仅 plus 接口需要。

## flag 语义

mknod 推荐支持：

- `FS_FLAG_EXCLUSIVE`：目标已存在则失败。

特殊文件类型应从 `req` 中读取，不建议用多个 flag 表达节点类型。

## 实现约束

mknod 必须严格校验 `req`：

- 普通文件不应经 mknod 路径绕过 create 语义，除非明确兼容 POSIX mknod regular file 行为。
- 字符设备、块设备必须校验 dev 字段有效性。
- 后端不支持的节点类型应返回明确错误。
