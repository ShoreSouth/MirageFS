# MirageFS UT 测试框架总纲

本文档描述 MirageFS 当前 UT 框架的落地方式、测试点清单、运行命令和覆盖率策略。目标是让功能验证从“启动项目后手动观察”逐步迁移到“可重复、可筛选、可覆盖率统计”的自动化测试。

## 目标

- 所有模块都必须有 UT 入口，新增模块时同步新增测试目录和 case list。
- case 能直接构造非常规场景，例如空指针、非法 flag、过小 buffer、重复释放、缺失文件等。
- 每个 case 都写明：场景、注入条件或故障、期望结果。
- 覆盖率使用 lcov/genhtml 生成 HTML 报告；长期目标是各模块 90% 以上。
- 当前先建立骨架和第一批高价值边界测试，不为了数字硬凑脆弱测试。

## 工具选择

当前采用轻量自研 C runner，加上 Makefile、lcov、genhtml：

- `tests/framework/test_framework.h` 提供 `TEST_CASE` 和断言宏。
- `tests/framework/test_framework.c` 负责打印 suite/case/scenario/fault/expected/result，并支持单 case 过滤。
- `tests/Makefile` 负责构建所有测试二进制、运行测试、生成覆盖率。
- `tools/test/run-ut.sh` 是日常入口，封装全部、单模块、单 case、列表和覆盖率命令，并输出最终汇总表。

cmocka 已安装，可以后续在需要 mock syscall、复杂 fixture、setup/teardown 时引入；当前第一版先保持依赖少、接入快。

## 目录结构

```text
tests/
  framework/
    test_framework.h
    test_framework.c
  config/test_config.c
  common/test_common.c
  lsa/test_lsa.c
  object/test_object.c
  fsc/test_fsc.c
  fops/test_fops.c
  namei/test_namei.c
  runtime/test_runtime.c
  msh/test_msh.c
tools/test/run-ut.sh
docs/testing/ut-framework.md
.cache/development-plan/ut-test-framework-plan.md
```

## 当前测试点

| 模块 | case 数 | 已覆盖测试点 |
| --- | ---: | --- |
| config | 2 | 默认配置加载；重复初始化幂等性 |
| common | 7 | fs_error_t 布局 round trip；模块名/操作名 helper 边界；flag 位检测；路径拼接 slash 归一；过小 buffer；路径 normalize dot/dotdot |
| lsa | 3 | 临时目录真实 create/write/read；缺失 lookup 映射 ENOENT；互斥 flag 提前拒绝 |
| object | 9 | FUID 构造/类型/flags；ObjKey 从 FUID 转换；ObjMeta 与 LSA handle 往返；ObjMeta init/equal/deinit；ObjRuntime 状态读取；ObjTable 插入/查找/删除/重复插入/非法参数 |
| fsc | 7 | FSC 错误码布局；FSID 分配/释放/重复释放 stale；NULL 输出参数；Namespace init/deinit/状态迁移/root 校验；FSTable 通过 fsid/name 双索引插入/查找/删除 |
| fops | 8 | init/deinit 生命周期；dispatch NULL args；非法 op 参数校验；name 校验；flag 冲突和未知位；类型 flag 不匹配；create mode 掩码；create attr valid_mask 校验 |
| namei | 3 | init/deinit 生命周期；ctx 保存 root/cwd；lookup NULL ctx |
| runtime | 3 | deinit 初始状态；未初始化保护；root/cwd getter NULL 输出 |
| msh | 3 | 命令行 quoted 参数解析；注释行；参数默认值 |

当前合计 45 个 case。

## 写 case 的格式

推荐每个 case 都使用这种结构：

```c
TEST_CASE(test_xxx,
          "中文场景名",
          "注入什么边界/故障",
          "期望什么结果")
```

编码使用 UTF-8，不影响 gcc 编译。Windows 终端或 Codex 工具输出里偶尔会显示乱码，但文件本身和 VSCode 正常识别即可。

## 运行命令

全部 UT：

```sh
make test
# 或
tools/test/run-ut.sh
# 或
tools/test/run-ut.sh all
```

单模块：

```sh
make test-common
tools/test/run-ut.sh common
```

单 case：

```sh
MIRAGEFS_TEST_CASE=test_error_layout_round_trip make test-common
tools/test/run-ut.sh common test_error_layout_round_trip
```

列出模块和 case：

```sh
tools/test/run-ut.sh list
tools/test/run-ut.sh list common
```

覆盖率：

```sh
make coverage
# 或
tools/test/run-ut.sh coverage
```

`tools/test/run-ut.sh coverage` 会在执行完 case 后额外输出两张汇总表：

- UT 结果汇总：按模块列出 passed/total/status。
- 覆盖率汇总：列出 TOTAL 以及 config/common/lsa/object/fsc/fops/namei/runtime/msh 各模块的行覆盖率和函数覆盖率。

覆盖率阈值检查：

```sh
make coverage-check COVERAGE_MIN=90
# 或
tools/test/run-ut.sh coverage-check 90
```

HTML 报告路径：

```text
output/coverage/html/index.html
```

## 覆盖率策略

当前已接入 `lcov` 和 `genhtml`，覆盖率命令会：

1. 清理旧构建和旧 `.gcda/.gcno`。
2. 用 `--coverage -O0` 重新构建项目。
3. 构建并运行 UT。
4. 捕获覆盖率数据。
5. 过滤 `/usr/*`、`tests/*`、`output/tests/*`。
6. 生成 `output/coverage/html/index.html`。

当前第一批补测后的基线为：行覆盖率 19.6%，函数覆盖率 29.8%。这是框架骨架阶段的正常结果，不代表目标完成。后续每个模块补业务 case 时，逐步把模块覆盖率推进到 90% 以上，再把 `COVERAGE_MIN=90` 固化到 CI 或默认检查中。

当前按模块覆盖率基线：

| 模块 | 行覆盖率 | 函数覆盖率 |
| --- | ---: | ---: |
| TOTAL | 19.6% | 29.8% |
| config | 47.4% | 66.7% |
| common | 30.8% | 38.0% |
| lsa | 9.4% | 13.0% |
| object | 28.5% | 39.8% |
| fsc | 38.9% | 58.0% |
| fops | 6.7% | 11.7% |
| namei | 11.6% | 22.0% |
| runtime | 11.9% | 28.9% |
| msh | 8.3% | 8.6% |

## make clean 行为

`make clean` 现在会清理：

- 根目录 `output/`
- `tests/output/`
- `src` 下的 `*.o`、`*.gcda`、`*.gcno`
- Python `__pycache__`

这样 coverage 或 UT 运行后不会留下旧测试输出影响下一轮验证。

## VSCode include 爆红说明

`#include "framework/test_framework.h"` 爆红的原因通常是 clangd 只看到了主程序的 `compile_commands.json`，不知道 `tests` 目录的 include path。

当前已通过 `.clangd` 增加：

```yaml
CompileFlags:
  Add:
    - -Isrc
    - -Itests
    - -D_GNU_SOURCE
    - -std=c17
```

如果 VSCode 仍爆红，执行 `clangd: Restart language server`，或重新打开窗口。

## 后续扩展顺序

1. common/config/lsa/object/fsc/fops/namei/runtime/msh 已有首批 UT，后续按模块补深。
2. 优先补“纯函数、参数校验、错误码、状态机”测试。
3. 再补“临时目录真实后端”的集成型 UT。
4. 最后引入 mock/fault injection，用于 syscall 失败、分配失败、并发锁等更难构造的场景。
5. 新功能开发时同步新增 case，并在 PR 前跑 `make test` 和必要的 `make coverage-check COVERAGE_MIN=90`。
