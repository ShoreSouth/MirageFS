#include "object/obj_sub.h"

static const char *g_obj_sub_names[] = {

#define OBJ_SUB_NAME(name, str) [OBJ_SUB_##name] = str,

        OBJ_SUB_TABLE(OBJ_SUB_NAME)

#undef OBJ_SUB_NAME

};

const char *obj_sub_name(obj_sub_t sub)
{
    if (!obj_sub_valid(sub))
    {
        return "UNKNOWN";
    }

    return g_obj_sub_names[sub];
}

bool obj_sub_valid(uint32_t sub)
{
    return sub < OBJ_SUB_MAX;
}
