#include <framework/test_framework.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int test_run_suite(const char *suite,
                   const test_case_t *cases,
                   size_t nr_cases)
{
    const char *filter = getenv("MIRAGEFS_TEST_CASE");
    size_t passed = 0;
    size_t selected = 0;

    printf("[suite] %s\n", suite);

    for (size_t i = 0; i < nr_cases; ++i) {
        const test_case_t *test_case = &cases[i];

        if (filter != NULL && filter[0] != 0 &&
            strcmp(test_case->name, filter) != 0) {
            continue;
        }

        ++selected;

        printf("  [case] %s\n", test_case->name);
        printf("    scenario: %s\n", test_case->scenario);
        printf("    fault:    %s\n", test_case->fault);
        printf("    expected: %s\n", test_case->expected);

        if (test_case->fn() == 0) {
            ++passed;
            printf("    result:   PASS\n");
        } else {
            printf("    result:   FAIL\n");
        }
    }

    if (filter != NULL && filter[0] != 0 && selected == 0) {
        fprintf(stderr, "[suite] %s: no case matched filter %s\n", suite, filter);
        return 1;
    }

    printf("[suite] %s: %zu/%zu passed\n", suite, passed, selected);

    return passed == selected ? 0 : 1;
}

int test_fail_at(const char *file, int line)
{
    fprintf(stderr, "%s:%d: assertion failed\n", file, line);
    return 1;
}

int test_fail_eq_int(const char *file, int line, long actual, long expected)
{
    fprintf(stderr,
            "%s:%d: expected equal integers, got %ld vs %ld\n",
            file,
            line,
            actual,
            expected);
    return 1;
}

int test_fail_eq_str(const char *file, int line, const char *actual, const char *expected)
{
    fprintf(stderr,
            "%s:%d: expected equal strings, got %s vs %s\n",
            file,
            line,
            actual ? actual : "(null)",
            expected ? expected : "(null)");
    return 1;
}

int test_str_eq(const char *actual, const char *expected)
{
    if (actual == NULL || expected == NULL) {
        return actual == expected;
    }

    return strcmp(actual, expected) == 0;
}
