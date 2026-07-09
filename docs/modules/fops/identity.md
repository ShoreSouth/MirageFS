# FOPS 对象身份

FOPS 使用 `obj_fuid_t` 表示对象身份。FUID 由 objectid 和 gen 组成，用于区分对象编号和对象代际。

## objectid

objectid 由 MirageFS 对象体系分配，不能直接复用 Linux `stat.st_ino` 作为全局身份。

原因：

- `st_ino` 只在单个底层文件系统内有意义，不天然跨后端全局唯一。
- MirageFS 需要统一管理普通文件、目录、特殊文件等对象身份。
- FOPS 上层不应该暴露底层 inode 的稳定性假设。

底层 inode 可以作为 LSA/backend locator 的一部分参与映射，但不直接成为 FOPS objectid。

## gen

`gen` 表示 objectid 的代际。objectid 回收后再次分配时，gen 应递增，从而避免旧引用误命中新对象。

典型语义：

```text
(objectid=42, gen=1) 旧对象删除
(objectid=42, gen=2) 新对象创建
```

只比较 objectid 会误判为同一对象；比较完整 FUID 可以识别代际变化。

## 与 ObjMgr 的关系

ObjMgr 负责对象身份、对象元数据和运行时对象的统一管理。FOPS 在创建、lookup、link 等 OP 中消费 ObjMgr 能力，但不额外维护另一套对象身份表。

FOPS 文档中的“对象”均指 MirageFS 对象，不直接等同于 Linux 文件描述符、inode 或路径字符串。
