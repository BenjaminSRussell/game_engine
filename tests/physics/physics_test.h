/**
 * Minimal C Testing Framework
 * Simple assertion-based testing for physics systems
 */

#ifndef PHYSICS_TEST_H
#define PHYSICS_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Assertions
#define ASSERT_TRUE(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            printf("  ❌ FAIL: %s:%d - %s\n", __FILE__, __LINE__, #expr); \
        } \
    } while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
    do { \
        tests_run++; \
        if (fabsf((a) - (b)) < (epsilon)) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            printf("  ❌ FAIL: %s:%d - Expected %.6f, got %.6f (diff: %.6f)\n", \
                   __FILE__, __LINE__, (float)(b), (float)(a), fabsf((a)-(b))); \
        } \
    } while(0)

#define ASSERT_INT_EQ(a, b) \
    do { \
        tests_run++; \
        if ((a) == (b)) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            printf("  ❌ FAIL: %s:%d - Expected %d, got %d\n", \
                   __FILE__, __LINE__, (int)(b), (int)(a)); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)

// Test runner
#define RUN_TEST(test_func) \
    do { \
        printf("Running: %s\n", #test_func); \
        test_func(); \
    } while(0)

#define TEST_SUITE_BEGIN(name) \
    printf("\n=== Test Suite: %s ===\n", name); \
    tests_run = tests_passed = tests_failed = 0;

#define TEST_SUITE_END() \
    printf("\n--- Results ---\n"); \
    printf("Tests Run: %d\n", tests_run); \
    printf("Passed: %d ✅\n", tests_passed); \
    printf("Failed: %d ❌\n", tests_failed); \
    printf("Success Rate: %.1f%%\n", tests_run > 0 ? (100.0f * tests_passed / tests_run) : 0.0f); \
    return tests_failed;

#endif // PHYSICS_TEST_H
