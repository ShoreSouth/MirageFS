#include "common/module/fs_module.h"

const char *fs_module_name(fs_module_t module)
{
    switch (module) {

#define FS_MODULE_CASE(name, str) \
    case FS_MODULE_##name: return str;

    FS_MODULE_TABLE(FS_MODULE_CASE)

#undef FS_MODULE_CASE

    default:
        return "UNKNOWN";
    }
}

bool fs_module_valid(uint32_t module)
{
    return module > FS_MODULE_NONE &&
           module < FS_MODULE_MAX;
}
