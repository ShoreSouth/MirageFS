#include "namei_test_common.h"

void test_namei_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}
