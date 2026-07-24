# FSMgr 模块设计

## 1. 模块定位

`fsmgr/` 是 FSC 的门面模块，负责 filesystem instance 的创建、
查找和销毁。

FSMgr 自己不实现底层数据结构，而是协调 FSC 内部组件：

```text
FSMgr
  ├── FSID
  ├── Namespace
  ├── NSPool
  ├── FSTable
  └── Sysroot/LSA 边界
```

它在 FSC 中对应 Object Layer 的 `objmgr`：负责生命周期编排，
不把所有存储、索引、分配逻辑堆在自己内部。

## 2. 模块职责

FSMgr 负责：

- 初始化内部 table 和 lock；
- 创建 namespace；
- 在 sysroot 下创建文件系统根目录；
- 为根目录分配并返回 FUID；
- 保存根目录 backend handle，供后续 Object 映射衔接；
- 销毁 namespace，并删除对应的根目录；
- 通过 name / FSID 查找 namespace；
- 维护 namespace 数量；
- 协调失败路径回滚。

FSMgr 不负责：

- FSID 分配算法；
- namespace 字段组织；
- namespace 内存池实现；
- hash table 实现；
- 普通文件/目录对象的创建；
- 将 fd/path 暴露给 FSC 上层。

## 3. 创建语义

`fsmgr_create()` 是文件系统创建的唯一入口。

当前签名：

```c
fs_error_t fsmgr_create(const char *name, fuid_t *root_out);
```

语义重点：

- 调用方只传入 filesystem 名称；
- FSC 内部从 sysroot 出发创建同名根目录；
- 创建成功后输出根目录 FUID；
- 不要求调用方提前创建目录或传入 root handle；
- API 中不出现 fd/path。

创建流程：

```text
fsmgr_create(name, root_out)
    │
    ├── 校验 name / root_out
    ├── 加 manager 锁
    ├── 检查 name 是否已注册
    ├── fsid_alloc()
    ├── fuid_make(fsid, root-object-id, gen=1, DIR)
    ├── 打开 sysroot handle 对应的临时 fd
    ├── lsa_mkdir(sysroot_fd, name)
    ├── lsa_name_to_handle_at(sysroot_fd, name)
    ├── nspool_alloc()
    ├── fsc_namespace_init(root_fuid, root_handle)
    ├── fstable_insert()
    ├── fsc_namespace_change_state(ACTIVE)
    ├── namespace_count++
    ├── root_out = root_fuid
    └── 解锁并返回 FS_OK
```

其中 fd 只在 `fsmgr.c` 的局部 helper 中短暂存在，来源和关闭都通过
LSA API 完成，不进入 `fsc_namespace_t` 或 FSC public API。

## 4. 创建失败回滚

创建过程中任一步失败，都按已经获得的资源逆序回滚：

```text
fsid_alloc 成功
root dir 创建失败
    └── fsid_free()

root dir 创建成功
nspool_alloc 失败
    ├── lsa_rmdir(sysroot_fd, name)
    └── fsid_free()

namespace 已分配
fstable_insert 失败
    ├── nspool_free()
    ├── lsa_rmdir(sysroot_fd, name)
    └── fsid_free()
```

这样可以保证失败路径不会泄漏 FSID、namespace 对象或磁盘目录。

## 5. 销毁语义

`fsmgr_destroy(fsid)` 通过 FSID 销毁 namespace。

当前流程：

```text
fsmgr_destroy(fsid)
    │
    ├── 加 manager 锁
    ├── fstable_lookup_fsid()
    ├── 检查 refcnt == 0
    ├── lsa_rmdir(sysroot_fd, ns->name)
    ├── fsc_namespace_change_state(DELETING)
    ├── fstable_remove()
    ├── namespace_count--
    ├── fsid_free()
    ├── nspool_free()
    └── 解锁
```

当前只支持删除空文件系统根目录。如果根目录下已经有对象，底层
`lsa_rmdir()` 会返回 `ENOTEMPTY` 类错误，namespace 保持注册状态。

递归删除整个文件系统不是 FSMgr/LSA 语义。上层应先通过 Runtime 的路径删除
链路逐项释放对象，使 Object/FOPS 生命周期正常闭合；根目录清空后再调用
`fsmgr_destroy()`。

## 6. 重命名与恢复语义

`fsmgr_rename(old, new)` 用于重命名 namespace，并同步重命名 sysroot 下的
后端根目录。重命名成功后，旧名称不再可 lookup，新名称沿用原 FSID 和 root
FUID。该接口只管理 namespace/root 绑定，不移动普通业务对象。

`fsmgr_recover()` 用于启动恢复：扫描 sysroot 下已有目录，并把尚未注册的目录
导入为 ACTIVE namespace。它只发生在 FSC 初始化/恢复边界，避免运行期间业务路径
绕过 Runtime/NAMEI/FOPS。

`fsmgr_list()` 返回当前已注册 ACTIVE namespace 名称，供 Runtime/MSH 展示。

## 7. 查询语义

FSMgr 提供两类 lookup：

```text
fsmgr_lookup(name)
fsmgr_lookup_fsid(fsid)
```

lookup 只返回 ACTIVE namespace，返回值是借用指针，不增加引用计数。
调用方不能释放该指针，也不能长期保存。

根目录查询拆成两个接口：

```c
fs_error_t fsmgr_get_root_fuid(fsc_fsid_t fsid, fuid_t *root_out);
fs_error_t fsmgr_get_root_handle(fsc_fsid_t fsid, obj_handle_t *handle_out);
```

业务层应优先使用 `fsmgr_get_root_fuid()`。`fsmgr_get_root_handle()` 只用于
FSC/Object 内部衔接，不作为面向业务的主入口。

## 8. 与 sysroot / LSA 的关系

sysroot 是项目系统根目录，位于所有 filesystem root 之上。

FSMgr 创建文件系统时，不接收外部 dirfd，而是：

1. 从 sysroot 子模块获取 sysroot 的 `obj_handle_t`；
2. 通过 `objmeta_handle_to_lsa()` 转换为 LSA file handle；
3. 调用 `lsa_open_by_handle_id()` 获取临时目录 fd；
4. 在该 fd 下调用 `lsa_mkdir()` 创建 filesystem root；
5. 调用 `lsa_name_to_handle_at()` 获取新根目录 handle；
6. 通过 `objmeta_handle_from_lsa()` 转回 `obj_handle_t`；
7. 关闭临时 fd。

mount fd 由 LSA 内部注册表保存，FSC 只持有 mount id + handle。

## 9. 当前 API

```text
fsmgr_init()
fsmgr_deinit()
fsmgr_create()
fsmgr_destroy()
fsmgr_rename()
fsmgr_recover()
fsmgr_list()
fsmgr_lookup()
fsmgr_lookup_fsid()
fsmgr_exists()
fsmgr_get_root_fuid()
fsmgr_get_root_handle()
fsmgr_count()
```

## 10. 并发模型

当前版本使用单把 manager 互斥锁保护 create/destroy/lookup 与 fstable。
实现简单，能保证生命周期一致性。后续如果 namespace 数量和并发访问增加，
可以演进为 RWLock 或 bucket lock，但不影响当前 FUID 优先的 API 语义。
