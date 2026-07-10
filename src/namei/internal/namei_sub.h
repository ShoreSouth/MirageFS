#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum namei_sub {

    NAMEI_SUB_NONE = 0,
    NAMEI_SUB_INIT,
    NAMEI_SUB_CTX,
    NAMEI_SUB_PATH,
    NAMEI_SUB_WALK,
    NAMEI_SUB_LOOKUP,
    NAMEI_SUB_PARENT,
    NAMEI_SUB_OP,

    NAMEI_SUB_MAX

} namei_sub_t;

const char *namei_sub_name(namei_sub_t sub);
bool namei_sub_valid(uint32_t sub);
