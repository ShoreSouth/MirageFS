#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint32_t ut_no_t;

typedef int (*test_fn_t)(void);

typedef enum ut_module_no {
    UT_MOD_COMMON = 0x01,
    UT_MOD_CONFIG = 0x02,
    UT_MOD_LSA = 0x03,
    UT_MOD_OBJECT = 0x04,
    UT_MOD_FSC = 0x05,
    UT_MOD_FOPS = 0x06,
    UT_MOD_NAMEI = 0x07,
    UT_MOD_RUNTIME = 0x08,
    UT_MOD_MSH = 0x09,
} ut_module_no_t;

typedef struct test_case {
    ut_no_t list_no;
    ut_no_t case_no;
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
int test_fail_eq_int(const char *file,
                     int line,
                     long actual,
                     long expected);
int test_fail_eq_str(const char *file,
                     int line,
                     const char *actual,
                     const char *expected);
int test_str_eq(const char *actual, const char *expected);

#define UT_NO_MAKE(module, component, list, item)                       \
    (((((ut_no_t)(module)) & 0xffU) << 24) |                            \
     ((((ut_no_t)(component)) & 0xffU) << 16) |                         \
     ((((ut_no_t)(list)) & 0x0fU) << 12) |                              \
     (((ut_no_t)(item)) & 0x0fffU))

#define UT_LIST_NO(module, component, list) \
    UT_NO_MAKE((module), (component), (list), 0U)

#define UT_CASE_NO(module, component, list, item) \
    UT_NO_MAKE((module), (component), (list), (item))

#define UT_NO_MODULE(no) ((((ut_no_t)(no)) >> 24) & 0xffU)
#define UT_NO_COMPONENT(no) ((((ut_no_t)(no)) >> 16) & 0xffU)
#define UT_NO_LIST(no) ((((ut_no_t)(no)) >> 12) & 0x0fU)
#define UT_NO_ITEM(no) (((ut_no_t)(no)) & 0x0fffU)
#define UT_NO_TO_LIST(no) (((ut_no_t)(no)) & 0xfffff000U)

#define TEST_CASE(list_no, case_no, fn, scenario, fault, expected) \
    { (list_no), (case_no), #fn, scenario, fault, expected, fn }

#define TEST_ASSERT_TRUE(expr) \
    do { if (!(expr)) return test_fail_at(__FILE__, __LINE__); } while (0)

#define TEST_ASSERT_FALSE(expr) \
    do { if ((expr)) return test_fail_at(__FILE__, __LINE__); } while (0)

#define TEST_ASSERT_EQ_INT(actual, expected) \
    do { \
        long test_actual__ = (long)(actual); \
        long test_expected__ = (long)(expected); \
        if (test_actual__ != test_expected__) { \
            return test_fail_eq_int(__FILE__, \
                                    __LINE__, \
                                    test_actual__, \
                                    test_expected__); \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected) \
    do { \
        if (!test_str_eq((actual), (expected))) { \
            return test_fail_eq_str(__FILE__, __LINE__, (actual), (expected)); \
        } \
    } while (0)
