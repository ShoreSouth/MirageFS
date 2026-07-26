# MirageFS agent instructions

## Scope and project identity

- 本文件是仓库根级、始终生效的 Agent 工作约定，适用于整个工作树。
- MirageFS 是使用 C17 编写的 Linux 用户态文件系统模拟器，同时也是用于
  学习文件系统分层、对象身份、错误模型和工程实践的教学项目。
- 面向人的概念解释和使用教程以 `docs/guides/` 为准；本文件只保留 Agent
  每次工作都必须知道的简短约束和路由信息。

## Skill routing

- 生成、修改、评审或设计 MirageFS 代码和架构时，必须加载
  `.agents/skills/miragefs-core/SKILL.md`。
- 其他 `.agents/skills/*/SKILL.md` 是按任务触发的可选工作流，不因为存放在
  仓库中就全部生效。仅在用户点名或任务符合其 description 时加载。
- `AGENTS.md` 规定“始终遵守什么”；`SKILL.md` 规定“某类任务如何完成”。
  详细边界和维护方法见 `docs/guides/agent-instructions-and-skills.md`。
- 用户的明确要求优先于仓库默认工作流；遇到冲突时应说明冲突及采用的处理。

## Task working memory

- 完成较大的任务后、向用户交付前，在 `.cache/task-summaries/` 写一份
  Markdown 总结，记录目标、关键决策、修改范围、验证结果和后续事项。
- 总结文件使用可排序的日期时间前缀，只保留最近 5 份；写入新总结后删除
  更旧的总结。
- `.cache` 中用于一次性分析、迁移或修复的临时脚本和中间文件应随用随清。
- `.cache/development-plan/` 属于阶段性计划，不按临时文件处理；确认计划已经
  完成或失效后再清理。

## Commit handoff

- 完成较大的代码或文档任务后，向用户展示一份符合 `.gitmessage` 的候选
  commit message，包含标题，并按需包含背景、改动、验证和影响。
- 小任务只需提供 `<type>(<scope>): <简洁中文说明>` 标题。
- 不自动执行 `git commit`；由用户检查候选内容后手动提交。
