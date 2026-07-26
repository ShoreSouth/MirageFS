#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FS_METRICS_NAME_MAX 63U
#define FS_METRICS_SCOPE_MAX 31U
#define FS_METRICS_MAX 128U
#define FS_METRICS_BUCKET_NR 16U
#define FS_METRIC_ID_INVALID UINT32_MAX

typedef enum fs_metrics_mode
{
    FS_METRICS_OFF = 0,
    FS_METRICS_CORE,
    FS_METRICS_DETAILED,
    FS_METRICS_MODE_MAX,
} fs_metrics_mode_t;

typedef uint32_t fs_metric_id_t;

typedef struct fs_metrics_token
{
    uint64_t start_ns;
    fs_metric_id_t id;
    bool active;
} fs_metrics_token_t;

typedef struct fs_metrics_value
{
    char name[FS_METRICS_NAME_MAX + 1U];
    char scope[FS_METRICS_SCOPE_MAX + 1U];
    uint64_t count;
    uint64_t errors;
    uint64_t bytes;
    uint64_t total_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t inflight;
    uint64_t peak_inflight;
    uint64_t buckets[FS_METRICS_BUCKET_NR];
    bool detailed;
} fs_metrics_value_t;

typedef struct fs_metrics_snapshot
{
    uint64_t captured_realtime_ns;
    uint64_t captured_monotonic_ns;
    size_t count;
    fs_metrics_value_t values[FS_METRICS_MAX];
} fs_metrics_snapshot_t;

/*
 * 初始化进程级统计核心。mode 决定热点路径是否实际采样。
 */
void fs_metrics_init(fs_metrics_mode_t mode);
void fs_metrics_deinit(void);

bool fs_metrics_mode_is_valid(fs_metrics_mode_t mode);
const char *fs_metrics_mode_to_str(fs_metrics_mode_t mode);
fs_metrics_mode_t fs_metrics_get_mode(void);
void fs_metrics_set_mode(fs_metrics_mode_t mode);

/*
 * 注册固定、低基数指标。初始化阶段注册完成后应调用 freeze。
 */
bool fs_metrics_register(const char *scope, const char *name, bool detailed,
                         fs_metric_id_t *id_out);
void fs_metrics_freeze(void);

fs_metrics_token_t fs_metrics_begin(fs_metric_id_t id);
void fs_metrics_end(fs_metrics_token_t token, bool success, uint64_t bytes);
void fs_metrics_snapshot(fs_metrics_snapshot_t *snapshot);

uint64_t fs_metrics_bucket_upper_ns(size_t bucket);
uint64_t fs_metrics_percentile_ns(const fs_metrics_value_t *value,
                                  uint32_t percentile);

