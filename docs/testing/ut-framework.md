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
- `tools/test/run-ut.sh` 是日常入口，封装全部、单模块、单 case、列表和覆盖率命令。

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

| 模块 | 已覆盖测试点 |
| --- | --- |
| config | 默认配置加载；重复初始化幂等性 |
| common | fs_error_t 布局 round trip；路径拼接 slash 归一；过小 buffer；路径 normalize dot/dotdot |
| lsa | 临时目录真实 create/write/read；缺失 lookup 映射 ENOENT；互斥 flag 提前拒绝 |
| object | FUID 构造/类型/flags；ObjKey 从 FUID 转换；ObjMeta 与 LSA handle 往返；NULL/超长 handle 参数校验 |
| fsc | FSC 错误码布局；FSID 分配/释放/重复释放 stale；NULL 输出参数 |
| fops | init/deinit 生命周期；dispatch NULL args；非法 op 参数校验 |
| namei | init/deinit 生命周期；ctx 保存 root/cwd；lookup NULL ctx |
| runtime | deinit 初始状态；未初始化保护；root/cwd getter NULL 输出 |
| msh | 命令行 quoted 参数解析；注释行；参数默认值 |

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

当前第一批基线约为行覆盖 11.3%。这是框架骨架阶段的正常结果，不代表目标完成。后续每个模块补业务 case 时，逐步把模块覆盖率推进到 90% 以上，再把 `COVERAGE_MIN=90` 固化到 CI 或默认检查中。

## VSCode include 爆红说明

`#include "framework/test_framework.h"` 爆红的原因是 clangd 当前只看到主程序的 `compile_commands.json`，不知道 `tests` 目录的 include path。

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