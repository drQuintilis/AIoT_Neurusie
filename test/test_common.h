#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>

/* Tiny assert-style test helpers — no external framework needed. */

static int g_test_failed = 0;

#define ASSERT_TRUE(cond) do {                                          \
    if (!(cond)) {                                                      \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
        g_test_failed = 1;                                              \
    }                                                                   \
} while (0)

#define ASSERT_EQ(a, b) do {                                            \
    long _a = (long)(a);                                                \
    long _b = (long)(b);                                                \
    if (_a != _b) {                                                     \
        printf("  FAIL %s:%d  %s == %s  (got %ld vs %ld)\n",            \
               __FILE__, __LINE__, #a, #b, _a, _b);                     \
        g_test_failed = 1;                                              \
    }                                                                   \
} while (0)

#define RUN_TEST(fn) do {                                               \
    int before = g_test_failed;                                         \
    printf("-- %s\n", #fn);                                             \
    fn();                                                               \
    if (g_test_failed == before) printf("  ok\n");                      \
} while (0)

#define TEST_RETURN() return g_test_failed ? 1 : 0

#endif /* TEST_COMMON_H */
