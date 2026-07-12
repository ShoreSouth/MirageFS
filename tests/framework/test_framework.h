#pragma once

#include <stddef.h>

typedef int (*test_fn_t)(void);

typedef struct test_case {
    const char *name;
    const char *scenario;
    const char *fault;
    const char *expected;
    test_fn_t fn;
} test_case_t;

int test_run_suite(const char *suite,
                   const test_case_t *cases,
                   size_t nr_cases);
int test_fail_at(const char *file, int line);
int test_fail_eq_int(const char *file, int line, long actual, long expected);
int test_fail_eq_str(const char *file, int line, const char *actual, const char *expected);
int test_str_eq(const char *actual, const char *expected);

#define TEST_CASE(fn, scenario, fault, expected) \
    { #fn, scenario, fault, expected, fn }

#define TEST_ASSERT_TRUE(expr) \
    do { if (!(expr)) return test_fail_at(__FILE__, __LINE__); } while (0)

#define TEST_ASSERT_FALSE(expr) \
    do { if ((expr)) return test_fail_at(__FILE__, __LINE__); } while (0)

#define TEST_ASSERT_EQ_INT(actual, expected) \
    do { \
        long test_actual__ = (long)(actual); \
        long test_expected__ = (long)(expected); \
        if (test_actual__ != test_expected__) { \
            return test_fail_eq_int(__FILE__, __LINE__, test_actual__, test_expected__); \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected) \
    do { \
        if (!test_str_eq((actual), (expected))) { \
            return test_fail_eq_str(__FILE__, __LINE__, (actual), (expected)); \
        } \
    } while (0)
