// Simple Test Framework for C
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// Test macros
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("  Running: %s... ", #name); \
        fflush(stdout); \
        tests_run++; \
        test_##name(); \
    } \
    static void test_##name(void)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            printf("    Assertion failed: %s (line %d)\n", #condition, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            printf("    Expected %s == %s (line %d)\n", #a, #b, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_NEQ(a, b) \
    do { \
        if ((a) == (b)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            printf("    Expected %s != %s (line %d)\n", #a, #b, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) ASSERT(condition)
#define ASSERT_FALSE(condition) ASSERT(!(condition))

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            printf("    Expected '%s' == '%s' (line %d)\n", (a), (b), __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
    do { \
        float diff = (a) - (b); \
        if (diff < 0) diff = -diff; \
        if (diff > (epsilon)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            printf("    Expected %f ~= %f (diff: %f, line %d)\n", (float)(a), (float)(b), diff, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
        tests_passed++; \
    } while(0)

#define RUN_TEST(name) run_test_##name()

#define TEST_SUITE_START(name) \
    printf("\n" COLOR_YELLOW "=== Test Suite: %s ===" COLOR_RESET "\n", name)

#define TEST_SUITE_END() \
    do { \
        printf("\n" COLOR_YELLOW "=== Results ===" COLOR_RESET "\n"); \
        printf("  Total:  %d\n", tests_run); \
        printf("  " COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed); \
        if (tests_failed > 0) { \
            printf("  " COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed); \
        } \
        printf("\n"); \
        return (tests_failed == 0) ? 0 : 1; \
    } while(0)

#endif // TEST_FRAMEWORK_H
