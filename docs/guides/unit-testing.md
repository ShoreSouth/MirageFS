# UT 测试使用

本文说明 MirageFS UT 的日常使用方式：怎么运行全部测试、单模块测试、单 case 测试、覆盖率统计，以及新增 case 时应该遵守的基本格式。

UT 框架设计总纲见 [UT 测试框架总纲](../testing/ut-framework.md)。本文只关注使用方法。

## 快速开始

运行全部 UT：

```sh
make test
```

或使用脚本入口：

```sh
tools/test/run-ut.sh
tools/test/run-ut.sh all
```

脚本会在最后输出模块级汇总表，便于快速确认每个 suite 的通过情况。

## 单模块测试

使用 Makefile：

```sh
make test-common
make test-fops
make test-namei
```

使用脚本：

```sh
tools/test/run-ut.sh common
tools/test/run-ut.sh fops
tools/test/run-ut.sh namei
```

当前支持的模块：

```text
config common lsa object fsc fops namei runtime msh
```

## UT 编号

每个 UT case 都有两个编号：

- `list_no`：用例集编号，例如 `0x06041000`。
- `case_no`：具体用例编号，例如 `0x06041001`。

编号格式为 `0xMMCCLIII`：

```text
MM   模块编号
CC   组件编号
L    用例集编号
III  用例项编号
```

例如 `0x06041001` 表示：FOPS 模块、create 组件、第 1 个用例集、第 1 个 case。

编号校验：

```sh
tools/test/list-ut.py --check
```

## 单 case 测试

使用环境变量：

```sh
MIRAGEFS_TEST_CASE=test_error_layout_round_trip make test-common
```

使用脚本按函数名执行：

```sh
tools/test/run-ut.sh common test_error_layout_round_trip
```

使用脚本按 `case_no` 执行：

```sh
tools/test/run-ut.sh fops 0x06041001
```

使用脚本按 `list_no` 执行同一用例集：

```sh
tools/test/run-ut.sh fops 0x06041000
```

单 case 执行适合调试边界条件、复现失败和配合 GDB 使用。

## 列出测试

列出全部测试 case：

```sh
tools/test/run-ut.sh list
# 或
tools/test/list-ut.py
```

列出某个模块下的 case：

```sh
tools/test/run-ut.sh list common
tools/test/run-ut.sh list fops
tools/test/list-ut.py --module fops
```

按编号和名称检索：

```sh
tools/test/list-ut.py --list 0x06041000
tools/test/list-ut.py --case 0x06041001
tools/test/list-ut.py --name create_mode
```

需要给其他工具消费时可以输出 JSON：

```sh
tools/test/list-ut.py --json
```

## 覆盖率

生成覆盖率：

```sh
make coverage
```

或：

```sh
tools/test/run-ut.sh coverage
```

覆盖率 HTML 报告：

```text
output/coverage/html/index.html
```

覆盖率数据文件：

```text
output/coverage/coverage.info
output/coverage/coverage.filtered.info
```

`coverage.filtered.info` 会过滤系统头文件、测试代码和测试输出目录，更适合观察项目源码覆盖率。

## 覆盖率阈值检查

使用 Makefile：

```sh
make coverage-check COVERAGE_MIN=90
```

使用脚本：

```sh
tools/test/run-ut.sh coverage-check 90
```

当前项目还处于 UT 补齐阶段，覆盖率阈值可以先作为观察指标。等核心模块 case 足够稳定后，再把 90% 固化为默认门禁。

## 新增 case 的位置

每个模块对应一个测试文件：

```text
tests/config/test_config.c
tests/common/test_common.c
tests/lsa/test_lsa.c
tests/object/test_object.c
tests/fsc/test_fsc.c
tests/fops/test_fops.c
tests/namei/test_namei.c
tests/runtime/test_runtime.c
tests/msh/test_msh.c
```

新增模块时，应同步新增：

```text
tests/<module>/test_<module>.c
tests/Makefile 中的测试目标
docs/testing/ut-framework.md 中的测试点说明
```

## case 描述格式

推荐每个 case 都写清楚三件事：

- 场景：要验证什么行为
- 注入：构造什么边界、异常或故障
- 期望：应该返回什么结果，状态是否保持一致

示例：

```c
TEST_CASE(test_error_layout_round_trip,
          "错误码布局往返",
          "构造 severity/module/sub/errno 字段",
          "字段解析结果与构造输入一致")
```

case 描述可以使用中文。源文件保持 UTF-8 编码即可，不影响 gcc 编译。命令行或某些工具偶尔显示乱码时，优先确认文件本身在 VSCode 中是否按 UTF-8 正常打开。

## 断言风格

优先使用测试框架提供的断言宏，让失败信息能指向具体条件。

常见断言包括：

```c
TEST_ASSERT_TRUE(expr);
TEST_ASSERT_FALSE(expr);
TEST_ASSERT_EQ(actual, expected);
TEST_ASSERT_NE(actual, expected);
TEST_ASSERT_NULL(ptr);
TEST_ASSERT_NOT_NULL(ptr);
```

新增断言时，应保持失败输出简单明确，不要在 case 内部手写大量 `printf()`。

## 推荐测试点

优先补这些高价值 case：

- `NULL` 输入
- 越界长度
- 过小 buffer
- 非法 flag
- 重复初始化和重复释放
- 缺失文件或目录
- 已存在对象重复插入
- 状态机非法迁移
- 错误码 module/sub/errno 是否正确
- 资源失败后是否回滚

对于 LSA、FOPS、NAMEI 这类会碰真实文件系统的模块，优先使用临时目录构造测试场景，并保证 case 结束后清理。

## 推荐执行流程

日常开发：

```sh
make test
```

只改某个模块：

```sh
tools/test/run-ut.sh fops
```

修复单个失败 case：

```sh
tools/test/run-ut.sh fops test_fops_validate_name_rejects_bad_input
```

提交前观察覆盖率：

```sh
tools/test/run-ut.sh coverage
```

需要门禁检查：

```sh
tools/test/run-ut.sh coverage-check 90
```

## 和 GDB 配合

先构建测试二进制：

```sh
make test-fops
```

再进入 GDB：

```sh
MIRAGEFS_TEST_CASE=test_fops_validate_name_rejects_bad_input \
gdb --args output/tests/bin/fops_test
```

更多调试方式见 [GDB 调试](gdb-debugging.md)。
