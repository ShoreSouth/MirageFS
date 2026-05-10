#pragma once

#include <stdint.h>

/* =========================
 * 线程 / 进程
 * ========================= */

/* 获取线程ID（轻量封装） */
uint64_t fs_get_tid(void);

/* 获取线程名（带缓存，线程安全） */
const char* fs_get_thread_name(void);

/* 获取进程名（进程级缓存） */
const char* fs_get_process_name(void);

/* =========================
 * 时间
 * ========================= */

/* 获取当前时间（秒） */
uint64_t fs_get_time_s(void);

/* 获取当前时间（毫秒） */
uint64_t fs_get_time_ms(void);

/* 获取当前时间（微秒） */
uint64_t fs_get_time_us(void);

/* 获取当前时间（纳秒） */
uint64_t fs_get_time_ns(void);

/* 获取单调时间（秒） */
uint64_t fs_get_monotonic_s(void);

/* 获取单调时间（毫秒） */
uint64_t fs_get_monotonic_ms(void);

/* 获取单调时间（微秒） */
uint64_t fs_get_monotonic_us(void);

/* 获取单调时间（纳秒） */
uint64_t fs_get_monotonic_ns(void);