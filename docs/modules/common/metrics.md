# Metrics 模块

`common/metrics` 是 MirageFS 的通用进程内性能统计基础设施。它只提供固定、
低基数指标的注册、计时、计数、直方图和弱一致快照，不理解 FSID、namespace
或某个具体文件操作的业务含义。

## 模式

```text
OFF       极速模式：热点路径不读时钟、不更新原子计数
CORE      默认模式：统计 filesystem 操作总量和端到端延迟
DETAILED  详细模式：在 CORE 基础上统计 NAMEI、LSA 等内部阶段
```

调用方在初始化阶段通过 `fs_metrics_register()` 注册固定指标，全部模块完成注册
后调用 `fs_metrics_freeze()`。运行阶段使用：

```c
fs_metrics_token_t token = fs_metrics_begin(metric_id);
/* 执行业务操作 */
fs_metrics_end(token, success, actual_bytes);
```

`actual_bytes` 必须传实际完成字节数，不能传请求字节数。`OFF` 模式返回无效
token，`fs_metrics_end()` 随即成为空操作。

## 快照与延迟

`fs_metrics_snapshot()` 返回累计、弱一致快照，包括：

- count、errors、bytes；
- total/min/max latency；
- 固定桶延迟直方图及近似 P50/P95/P99；
- 当前 inflight 和进程生命周期 peak inflight。

累计 counter 和 histogram bucket 可以做差得到时间窗口值；累计 min/max 和
peak 不能做差。因此 Runtime 的 session 报告不会伪造窗口 min/max。

Metrics 使用完整的 `CLOCK_MONOTONIC` 纳秒值计算耗时。实时时钟只用于报告时间
戳，不能用于延迟计算。

## 边界

- 不允许把 path、FUID、trace ID 等高基数字段注册为 label。
- Metrics、Trace、Log 是三个独立设施，不能相互替代。
- `common/metrics` 不生成 JSON/HTML；业务聚合和报告属于 Runtime。
- 当前直方图分位数是桶上界近似值，不是精确采样排序结果。

