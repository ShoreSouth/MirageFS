# Runtime 模块总纲

Runtime 是 MirageFS V1.0 的运行时编排层，位于 CLI/Shell/未来协议入口之下，NAMEI/FOPS/FSC/Object/LSA 之上。

## 职责

Runtime 负责：

- 统一初始化和反初始化底层模块。
- 管理当前 namespace 会话。
- 保存 root_fuid、cwd_fuid 和控制台显示用 cwd 路径。
- 构造 namei_ctx_t。
- 为 CLI、未来 Web UI 和未来协议适配层提供稳定 facade。
- 将路径版请求转换为 NAMEI 调用或 fops_dispatch 调用。

Runtime 不负责：

- 直接保存 backend fd/path。
- 绕过 NAMEI 解析路径。
- 绕过 fops_dispatch 进入 FOPS OP。
- 实现 CLI 文本命令解析。
- 实现 FUSE/9P/NFS/RPC 协议。

## 依赖方向

推荐调用链：

APP / CLI / Shell / Protocol Adapter
  -> Runtime
  -> NAMEI
  -> FOPS
  -> FSC / Object / LSA

## API 覆盖范围

Runtime V1 覆盖当前已实现的主要 FOPS 操作面：

- namespace: create、destroy、use、leave。
- path lookup: lookup、lookup_plus、lookup_parent。
- create/delete/name ops: create、mkdir、mknod、unlink、rmdir、rename、link、symlink、readlink。
- directory ops: readdir、readdirplus。
- attr/access: getattr、setattr、access、truncate。
- file handle ops: open、close、read、write。
- xattr: getxattr、setxattr、listxattr、removexattr。
- fs ops: statfs、syncfs。
- adapter-only handle ops: gethandle、openhandle。

其中 NAMEI 已有路径版封装的操作优先调用 NAMEI；NAMEI 尚未封装的路径操作由 Runtime 使用 namei_lookup 或 namei_lookup_parent 组织 fops_args_t，再通过 fops_dispatch 进入 FOPS。

## V1.0 入口策略

V1.0 以控制台 Shell 为唯一正式用户入口。Shell 应只依赖 Runtime，不直接调用 FOPS 细粒度 API，不直接操作 ObjMgr，也不保存 backend fd/path。
