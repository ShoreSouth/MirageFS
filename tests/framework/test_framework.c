#include <framework/test_framework.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_parse_no(const char *text, ut_no_t *out)
{
    char *end = NULL;
    unsigned long value;

    if ((text == NULL) || (text[0] == 0) || (out == NULL))
    {
        return 0;
    }

    errno = 0;
    value = strtoul(text, &end, 0);
    if ((errno != 0) || (end == text) || (*end != 0) || (value > UINT32_MAX))
    {
        return 0;
    }

    *out = (ut_no_t)value;
    return 1;
}

static int test_case_matches_filter(const test_case_t *test_case,
                                    const char *filter)
{
    ut_no_t filter_no;

    if ((filter == NULL) || (filter[0] == 0))
    {
        return 1;
    }

    if (strcmp(test_case->name, filter) == 0)
    {
        return 1;
    }

    if (!test_parse_no(filter, &filter_no))
    {
        return 0;
    }

    return (test_case->case_no == filter_no) ||
           (test_case->list_no == filter_no);
}

int test_run_suite(const char *suite, const test_case_t *cases, size_t nr_cases)
{
    const char *filter = getenv("MIRAGEFS_TEST_CASE");
    size_t passed = 0;
    size_t selected = 0;

    printf("[suite] %s\n", suite);

    for (size_t i = 0; i < nr_cases; ++i)
    {
        const test_case_t *test_case = &cases[i];

        if (!test_case_matches_filter(test_case, filter))
        {
            continue;
        }

        ++selected;

        printf("  [case] 0x%08x %s\n", test_case->case_no, test_case->name);
        printf("    list_no:  0x%08x\n", test_case->list_no);
        printf("    scenario: %s\n", test_case->scenario);
        printf("    fault:    %s\n", test_case->fault);
        printf("    expected: %s\n", test_case->expected);

        if (test_case->fn() == 0)
        {
            ++passed;
            printf("    result:   PASS\n");
        }
        else
        {
            printf("    result:   FAIL\n");
        }
    }

    if ((filter != NULL) && (filter[0] != 0) && (selected == 0))
    {
        fprintf(stderr, "[suite] %s: no case matched filter %s\n", suite,
                filter);
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
    fprintf(stderr, "%s:%d: expected equal integers, got %ld vs %ld\n", file,
            line, actual, expected);
    return 1;
}

int test_fail_eq_str(const char *file, int line, const char *actual,
                     const char *expected)
{
    fprintf(stderr, "%s:%d: expected equal strings, got %s vs %s\n", file, line,
            actual ? actual : "(null)", expected ? expected : "(null)");
    return 1;
}

int test_str_eq(const char *actual, const char *expected)
{
    if ((actual == NULL) || (expected == NULL))
    {
        return actual == expected;
    }

    return strcmp(actual, expected) == 0;
}
