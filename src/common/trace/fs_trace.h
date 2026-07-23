#pragma once

#include "common/os/fs_os.h"

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

/* =========================
 * 基础类型
 * ========================= */

typedef struct fs_trace_ctx
{
    uint64_t trace_id;  // 一条完整请求（全局唯一）
    uint64_t span_id;   // 当前操作
    uint64_t parent_id; // 上级调用
} fs_trace_ctx_t;

/* =========================
 * TLS 存储（Thread Local Storage）
 * ========================= */

extern __thread fs_trace_ctx_t g_fs_trace_tls;

/* 获取当前 trace */
static inline fs_trace_ctx_t *fs_trace_get(void)
{
    return &g_fs_trace_tls;
}

/* 设置 trace（用于入口 or 跨线程恢复） */
static inline void fs_trace_set(fs_trace_ctx_t *ctx)
{
    if (ctx)
    {
        g_fs_trace_tls = *ctx;
    }
}

/* 清理 trace */
static inline void fs_trace_clear(void)
{
    g_fs_trace_tls.trace_id = 0;
    g_fs_trace_tls.span_id = 0;
    g_fs_trace_tls.parent_id = 0;
}

/* =========================
 * ID 生成（轻量实现）
 * ========================= */

static inline uint64_t fs_trace_gen_id(void)
{
    static __thread uint64_t seq = 0;

    uint64_t tv_sec = fs_get_time_s();
    uint64_t tv_nsec = fs_get_time_ns();

    uint64_t tid = (uint64_t)syscall(SYS_gettid);

    return ((uint64_t)tv_sec << 32) ^ ((uint64_t)tv_nsec) ^ (tid << 16) ^
           (++seq);
}

/* =========================
 * trace 初始化
 * ========================= */

/* 创建新 trace */
static inline void fs_trace_init(fs_trace_ctx_t *ctx)
{
    if (!ctx)
        return;

    ctx->trace_id = fs_trace_gen_id();
    ctx->span_id = ctx->trace_id;
    ctx->parent_id = 0;
}

/* 初始化生成trace并写入 TLS */
static inline void fs_trace_begin(fs_trace_ctx_t *ctx)
{
    fs_trace_init(ctx);

    fs_trace_set(ctx);
}

/* 离开 trace */
static inline void fs_trace_end(void)
{
    fs_trace_clear();
}

/* =========================
 * span 机制
 * ========================= */

typedef struct fs_trace_span_guard
{
    fs_trace_ctx_t saved;
} fs_trace_span_guard_t;

/* span begin */
static inline fs_trace_span_guard_t fs_trace_span_begin(const char *name)
{
    fs_trace_span_guard_t guard;

    fs_trace_ctx_t *cur = fs_trace_get();
    guard.saved = *cur;

    uint64_t new_span = fs_trace_gen_id();

    cur->parent_id = cur->span_id;
    cur->span_id = new_span;

    /* 打日志（你后续可以接 fs_log） */
    fprintf(stderr, "[TRACE] BEGIN span=%lu parent=%lu trace=%lu name=%s\n",
            cur->span_id, cur->parent_id, cur->trace_id, name);

    return guard;
}

/* span end */
static inline void fs_trace_span_end(fs_trace_span_guard_t *guard,
                                     const char *name)
{
    fs_trace_ctx_t *cur = fs_trace_get();

    fprintf(stderr, "[TRACE] END   span=%lu parent=%lu trace=%lu name=%s\n",
            cur->span_id, cur->parent_id, cur->trace_id, name);

    *cur = guard->saved;
}

/* =========================
 * 宏封装（核心！）
 * ========================= */

/* 自动作用域 span（推荐用这个） */
#define FS_TRACE_SPAN(name)                                                    \
    fs_trace_span_guard_t __trace_guard = fs_trace_span_begin(name);           \
    __attribute__((cleanup(fs_trace_span_auto_end)))                           \
    fs_trace_span_guard_t *__trace_guard_ptr = &__trace_guard

static inline void fs_trace_span_auto_end(fs_trace_span_guard_t **guard)
{
    if (guard && *guard)
    {
        fs_trace_span_end(*guard, "auto");
    }
}

/* =========================
 * 对外接口辅助
 * ========================= */

/* API入口模板 */

#define FS_TRACE_BEGIN(ctx) fs_trace_begin(ctx)

#define FS_TRACE_END() fs_trace_end()

#define FS_TRACE_GET() fs_trace_get()
