#include "namei/internal/namei_sub.h"

static const char *g_namei_sub_names[] = {
    [NAMEI_SUB_NONE] = "NONE",
    [NAMEI_SUB_INIT] = "INIT",
    [NAMEI_SUB_CTX] = "CTX",
    [NAMEI_SUB_PATH] = "PATH",
    [NAMEI_SUB_WALK] = "WALK",
    [NAMEI_SUB_LOOKUP] = "LOOKUP",
    [NAMEI_SUB_PARENT] = "PARENT",
    [NAMEI_SUB_OP] = "OP",
};

const char *namei_sub_name(namei_sub_t sub)
{
    if (!namei_sub_valid(sub)) {
        return "UNKNOWN";
    }

    return g_namei_sub_names[sub];
}

bool namei_sub_valid(uint32_t sub)
{
    return sub < NAMEI_SUB_MAX;
}
