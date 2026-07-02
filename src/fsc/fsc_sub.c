#include "fsc/fsc_sub.h"

static const char *g_fsc_sub_names[] = {

#define FSC_SUB_NAME(name, str) \
    [FSC_SUB_##name] = str,

    FSC_SUB_TABLE(FSC_SUB_NAME)

#undef FSC_SUB_NAME

};

const char *fsc_sub_name(fsc_sub_t sub)
{
    if (!fsc_sub_valid(sub)) {
        return "UNKNOWN";
    }

    return g_fsc_sub_names[sub];
}

bool fsc_sub_valid(uint32_t sub)
{
    return sub < FSC_SUB_MAX;
}
