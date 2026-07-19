# 读代码路线

本文给出 MirageFS 的建议读代码路线。目标不是一次读完所有文件，而是按调用链逐步建立心智模型。

## 准备工作

先确认环境：

```sh
cd /home/shore/work/github/MirageFS
make
make test
```

推荐使用 `rg` 搜索：

```sh
rg "fops_dispatch" src tests docs
rg "typedef struct obj_meta" src/object
```

读代码时建议同时打开：

- 源码：`src/`
- 模块文档：`docs/modules/`
- UT：`tests/`
- 模块地图：[模块地图](module-map.md)

## 路线一：从主程序往下读

适合想知道程序如何启动的人。

阅读顺序：

```text
src/app/main.c
src/runtime/
src/namei/
src/fops/
src/fsc/ + src/object/
src/lsa/
src/common/
```

关注问题：

- 主程序调用哪个 runtime 入口。
- runtime 如何初始化各模块。
- namei 如何组织路径解析。
- fops 如何分发单步操作。
- fsc/object 如何提供上下文和对象身份。
- lsa 如何落到 Linux 后端。

## 路线二：从一次文件操作往下读

适合理解核心业务链路。

建议从 `lookup` 或 `create` 开始：

```text
namei lookup/create wrapper
        ↓
fops_dispatch()
        ↓
fops lookup/create implementation
        ↓
object/fsc helper
        ↓
lsa syscall wrapper
```

可以搜索：

```sh
rg "lookup" src/namei src/fops src/lsa
rg "create" src/namei src/fops src/lsa
```

读的时候重点看三件事：

1. 输入参数在哪里校验。
2. 错误码在哪里构造，是否直接向上传播。
3. 资源失败时是否有清理路径。

## 路线三：从对象模型开始读

适合理解 MirageFS 自己如何标识对象。

阅读顺序：

```text
src/object/fuid/
src/object/objkey/
src/object/objmeta/
src/object/objruntime/
src/object/objtable/
src/object/objpool/
src/object/objmgr/
```

对应文档：

```text
docs/modules/object/fuid/fuid.md
docs/modules/object/objkey/objkey.md
docs/modules/object/meta/objmeta.md
docs/modules/object/objruntime/objruntime.md
docs/modules/object/objtable/objtable.md
docs/modules/object/objpool/objpool.md
docs/modules/object/objmgr/objmgr.md
```

建议先把这几个概念分清：

- `fuid_t` 是对象身份。
- `obj_key_t` 是索引用的 key。
- `obj_meta_t` 是对象元数据。
- `obj_runtime_t` 是带生命周期和引用计数的运行时实例。

## 路线四：从错误系统开始读

适合理解项目如何表达失败。

阅读顺序：

```text
src/common/error/
src/common/module/
src/common/op/
src/object/obj_error.c
src/fsc/fsc_error.c
src/lsa/internal/lsa_error.c
src/fops/core/fops_error.c
src/namei/core/namei_error.c
src/runtime/core/runtime_error.c
```

关注问题：

- `fs_error_t` 的 bit layout。
- module/sub/errno 如何编码。
- `fs_failed()` 和 `fs_succeeded()` 如何判断。
- 子模块错误名如何注册。

错误系统是读其他模块的基础。看到 `return err;` 时，要判断它是在创建新错误，还是传播下层错误。

## 路线五：从 UT 反推行为

适合学习项目和验证边界。

先列出 case：

```sh
tools/test/run-ut.sh list
tools/test/run-ut.sh list fops
```

再读对应文件：

```text
tests/common/test_common.c
tests/fops/test_fops.c
tests/namei/test_namei.c
```

UT 通常比主流程更容易暴露模块边界，因为它会直接构造：

- NULL 输入
- 非法 flag
- 缺失文件
- 重复释放
- 过小 buffer
- 状态机非法迁移

建议阅读时把 case 描述中的“场景 / 注入 / 期望”对应到源码分支。

## 推荐第一次阅读路径

如果只读一轮，推荐：

1. [模块地图](module-map.md)
2. `src/common/error/`
3. `src/object/fuid/`、`src/object/objmeta/`
4. `src/fsc/namespace/`、`src/fsc/fsid/`
5. `src/fops/core/fops_dispatch.c`
6. 一个具体操作，例如 `src/fops/ops/fops_lookup.c`
7. `src/namei/core/namei_walk.c`
8. 对应 UT，例如 `tests/fops/test_fops.c`

## 阅读时的判断标准

读到一个函数时，建议问：

- 这个函数属于哪个模块职责。
- 它是否处在模块公共边界。
- 它拥有哪个资源，释放责任在哪里。
- 它返回的是 `fs_error_t`、指针、bool 还是值对象。
- 它是否应该记录日志。
- 它是否应该被 UT 直接覆盖。

这样读代码会比按文件名从上到下扫更稳定。
