# FSC Sysroot 设计说明

## 定位

sysroot 是 MirageFS 运行期的项目系统根目录。它是全局唯一资源，位于所有文件系统根目录之上。

目录层级约束如下：

    sysroot/
      fs-a/
      fs-b/

sysroot 下的一级目录只能是各个文件系统根目录。普通文件、普通目录仍归属于具体文件系统，不应直接创建在 sysroot 下。

## 模块归属

sysroot 放在 FSC 模块内部的 sysroot 子模块中，路径为：

    src/fsc/sysroot/

它没有进入 OBJECT 的 objtable，也没有进入 FSC 的 fstable。原因是 sysroot 是更高一层的全局锚点，不是普通文件对象集合中的一员，也不是某个文件系统根目录资源。

当前 sysroot 子模块单独保存一份运行态，包括：

- 保留 FUID；
- sysroot 目录对应的 obj_handle_t；
- 初始化后的绝对路径；
- 简单生命周期状态。

## 路径例外

项目原则是：LSA 之上的模块不暴露 fd/path，对象访问应围绕 FUID 与 obj_handle_t 展开。

sysroot 是唯一例外。项目启动时还没有任何上层根对象可以作为入口，因此 fsc_sysroot_init() 允许接收路径，并委托 LSA 创建目录、获取 Linux file handle。初始化完成后，FSC 及其上层模块仍应回到 FUID/handle 模型，不继续传播路径或 fd。

默认路径为：

    ./miragefs.root

初始化时会转换为绝对路径保存。当前实现不会在 fsc_sysroot_deinit() 中删除磁盘目录，只释放内存运行态。

## LSA 适配

LSA 新增启动专用接口：

    lsa_ret_t lsa_bootstrap_root(
                    const char *path,
                    lsa_file_handle_t *handle,
                    int32_t *mount_id);

该接口做三件事：

1. 创建 sysroot 目录，若目录已存在则复用；
2. 校验目标确实是目录；
3. 通过 lsa_name_to_handle_at() 取得 lsa_file_handle_t 与 mount id。

这个接口只服务启动边界，不改变其他 LSA API 的 handle 优先原则。

## FSC 初始化顺序

FSC 当前初始化顺序为：

    fsc_sysroot_init
    fsid_init
    nspool_init
    fsmgr_init

销毁时按逆序释放：

    fsmgr_deinit
    nspool_deinit
    fsid_deinit
    fsc_sysroot_deinit

如果 sysroot 之后的任意步骤失败，FSC 会回滚已经初始化的资源并释放 sysroot 运行态。

## 对 FSC 后续开发的影响

后续重构 fsmgr_create 时，不建议在对外 args 中出现 dirfd 或路径字段。更合适的方向是：

- 创建文件系统时由 FSC 内部取得 sysroot 的 obj_handle_t；
- 通过 LSA 的 handle 打开能力获得必要的底层 fd；
- 在 sysroot 下创建文件系统根目录；
- 为新文件系统根目录分配 FUID，并写入 fstable 映射；
- 对外返回 FUID，后续业务通过 FUID 查询 meta/runtime/handle。

这样可以保持 LSA 上层 API 不泄漏 fd，同时让文件系统创建动作真正落在 FSC 内部完成。
