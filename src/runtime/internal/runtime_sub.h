#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Runtime sub-error。
 *
 * sub 字段用于标识 Runtime 内部失败发生在哪个职责区域，便于
 * fs_error_str() 打印 RUNTIME::SESSION、RUNTIME::OP 等可读错误。
 */
typedef enum runtime_sub {

    RUNTIME_SUB_NONE = 0,
    RUNTIME_SUB_INIT,      /* 运行时初始化/反初始化 */
    RUNTIME_SUB_SESSION,   /* 当前 namespace 会话 */
    RUNTIME_SUB_NAMESPACE, /* namespace 管理 */
    RUNTIME_SUB_CTX,       /* namei_ctx_t 构造 */
    RUNTIME_SUB_PATH,      /* cwd/path 展示路径处理 */
    RUNTIME_SUB_OP,        /* FOPS dispatch 参数组织 */
    RUNTIME_SUB_HANDLE,    /* 后端 handle 辅助操作 */

    RUNTIME_SUB_MAX

} runtime_sub_t;

/* 返回 Runtime sub-error 名称。 */
const char *runtime_sub_name(runtime_sub_t sub);

/* 判断 Runtime sub-error 是否有效。 */
bool runtime_sub_valid(uint32_t sub);
