#include "runtime/internal/runtime_sub.h"

const char *runtime_sub_name(runtime_sub_t sub)
{
    switch (sub)
    {
    case RUNTIME_SUB_NONE:
        return "NONE";
    case RUNTIME_SUB_INIT:
        return "INIT";
    case RUNTIME_SUB_SESSION:
        return "SESSION";
    case RUNTIME_SUB_NAMESPACE:
        return "NAMESPACE";
    case RUNTIME_SUB_CTX:
        return "CTX";
    case RUNTIME_SUB_PATH:
        return "PATH";
    case RUNTIME_SUB_OP:
        return "OP";
    case RUNTIME_SUB_HANDLE:
        return "HANDLE";
    default:
        return "UNKNOWN";
    }
}

bool runtime_sub_valid(uint32_t sub)
{
    return sub < RUNTIME_SUB_MAX;
}
