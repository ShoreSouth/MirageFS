/**
 * @file main.c
 * @brief MirageFS main entry.
 */

#include <stdio.h>

#include "common/fs_common.h"
#include "runtime/include/runtime.h"

int main(void)
{
    fs_error_t err;

    printf("MirageFS Starting...\n");

    err = runtime_init(NULL);
    if (fs_failed(err)) {
        printf("MirageFS init failed: %s (0x%x)\n",
               fs_error_str(err), err);
        return 1;
    }

    runtime_deinit();

    printf("MirageFS Exit.\n");
    return 0;
}
