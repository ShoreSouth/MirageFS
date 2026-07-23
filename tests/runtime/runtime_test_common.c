#include "runtime_test_common.h"

void test_runtime_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}
