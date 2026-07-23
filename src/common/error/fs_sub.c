#include "common/error/fs_sub.h"

#include <stdio.h>

/*
 * ============================================================
 * sub-name 注册表
 *
 * 以 module ID 为索引的数组。
 * 未注册的模块对应 NULL。
 * ============================================================
 */

static fs_sub_name_fn g_sub_name_fns[FS_MODULE_MAX];

void fs_sub_register(fs_module_t module, fs_sub_name_fn fn)
{
    if (!fs_module_valid(module))
    {
        return;
    }

    g_sub_name_fns[module] = fn;
}

const char *fs_sub_name(fs_module_t module, uint32_t sub)
{
    fs_sub_name_fn fn;

    if (!fs_module_valid(module))
    {
        return NULL;
    }

    fn = g_sub_name_fns[module];
    if (fn == NULL)
    {
        return NULL;
    }

    return fn(sub);
}
