/**
 * UNIFIED TEST FRAMEWORK
 * Comprehensive Testing Framework for Game Engine TDD Suite
 * 
 * Features:
 * - Test registration with suites
 * - Setup/teardown per test
 * - Rich assertion macros
 * - Benchmarking support
 * - Memory leak detection
 * - Mocking support for headless testing
 * - Colored output
 */

#ifndef TEST_FRAMEWORK_UNIFIED_H
#define TEST_FRAMEWORK_UNIFIED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// =============================================================================
// TEST RESULT TYPES
// =============================================================================

typedef enum { 
    TEST_PASS = 0, 
    TEST_FAIL = 1, 
    TEST_SKIP = 2 
} TestResult;

typedef TestResult (*TestFunc)(void);

typedef struct {
    const char *name;
    const char *suite;
    TestFunc func;
    TestFunc setup;
    TestFunc teardown;
} TestCase;

typedef struct {
    uint32_t total;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
    double duration_ms;
} TestStats;

// =============================================================================
// ANSI COLOR CODES
// =============================================================================

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

// =============================================================================
// ASSERTION MACROS
// =============================================================================

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            test_fail(__FILE__, __LINE__, message); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_TRUE(condition, message) \
    TEST_ASSERT((condition), message)

#define TEST_ASSERT_FALSE(condition, message) \
    TEST_ASSERT(!(condition), message)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            test_fail_eq(__FILE__, __LINE__, message, \
                        (long long)(expected), (long long)(actual)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NEQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            test_fail(__FILE__, __LINE__, message); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_FLOAT_EQ(actual, expected, epsilon, message) \
    do { \
        double _diff = fabs((double)(actual) - (double)(expected)); \
        if (_diff > (epsilon)) { \
            test_fail_float(__FILE__, __LINE__, message, \
                           (double)(expected), (double)(actual), (epsilon)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_VEC3_EQ(actual, expected, epsilon, message) \
    do { \
        float _dx = fabsf((actual).x - (expected).x); \
        float _dy = fabsf((actual).y - (expected).y); \
        float _dz = fabsf((actual).z - (expected).z); \
        if (_dx > (epsilon) || _dy > (epsilon) || _dz > (epsilon)) { \
            test_fail_vec3(__FILE__, __LINE__, message, \
                          (expected), (actual), (epsilon)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_RANGE(val, min, max, message) \
    do { \
        if ((val) < (min) || (val) > (max)) { \
            char _buf[256]; \
            snprintf(_buf, sizeof(_buf), "%s (Value %lld not in [%lld, %lld])", \
                     message, (long long)(val), (long long)(min), (long long)(max)); \
            test_fail(__FILE__, __LINE__, _buf); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_EQ(actual, expected, message) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            test_fail_str(__FILE__, __LINE__, message, (expected), (actual)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_CONTAINS(haystack, needle, message) \
    do { \
        if (strstr((haystack), (needle)) == NULL) { \
            char _buf[256]; \
            snprintf(_buf, sizeof(_buf), "%s ('%.32s' not found)", message, (needle)); \
            test_fail(__FILE__, __LINE__, _buf); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_MEMORY_EQ(ptr1, ptr2, size, message) \
    do { \
        if (memcmp((ptr1), (ptr2), (size)) != 0) { \
            test_fail(__FILE__, __LINE__, message); \
            return TEST_FAIL; \
        } \
    } while (0)

#define TEST_ASSERT_ARRAY_EQ(arr1, arr2, count, message) \
    do { \
        for (size_t _i = 0; _i < (count); _i++) { \
            if ((arr1)[_i] != (arr2)[_i]) { \
                char _buf[256]; \
                snprintf(_buf, sizeof(_buf), "%s (mismatch at index %zu)", message, _i); \
                test_fail(__FILE__, __LINE__, _buf); \
                return TEST_FAIL; \
            } \
        } \
    } while (0)

// =============================================================================
// BENCHMARKING MACROS
// =============================================================================

#define TEST_BENCHMARK_START() \
    clock_t _bench_start = clock()

#define TEST_BENCHMARK_END(name) \
    do { \
        clock_t _bench_end = clock(); \
        double _duration = ((double)(_bench_end - _bench_start)) / CLOCKS_PER_SEC * 1000.0; \
        printf("    [BENCH] %s: %.4f ms\n", (name), _duration); \
    } while (0)

#define TEST_BENCHMARK_ITERATIONS(name, iterations, code) \
    do { \
        clock_t _start = clock(); \
        for (int _iter = 0; _iter < (iterations); _iter++) { code; } \
        clock_t _end = clock(); \
        double _total = ((double)(_end - _start)) / CLOCKS_PER_SEC * 1000.0; \
        double _per_iter = _total / (iterations); \
        printf("    [BENCH] %s: %.4f ms total, %.6f ms/iter (%d iters)\n", \
               (name), _total, _per_iter, (iterations)); \
    } while (0)

// =============================================================================
// TEST REGISTRATION MACROS
// =============================================================================

#define TEST_REGISTER(suite_name, test_name, test_func) \
    test_register(suite_name, test_name, test_func, NULL, NULL)

#define TEST_REGISTER_WITH_SETUP(suite_name, test_name, test_func, setup, teardown) \
    test_register(suite_name, test_name, test_func, setup, teardown)

// Suite helper macros for grouping tests
#define TEST_SUITE_BEGIN(name) \
    static const char *_current_suite = (name); \
    printf("\n" COLOR_MAGENTA "═══ %s ═══" COLOR_RESET "\n", name)

#define TEST_SUITE_END() \
    (void)_current_suite

// =============================================================================
// MOCKING SUPPORT
// =============================================================================

// Mock function pointer type
typedef void (*MockFunc)(void);

// Mock registry (simple implementation)
#define MOCK_MAX_FUNCS 64

typedef struct {
    const char *name;
    MockFunc original;
    MockFunc mock;
    bool active;
} MockEntry;

// Mock macros
#define MOCK_FUNCTION(func_name, mock_impl) \
    mock_register(#func_name, (MockFunc)(func_name), (MockFunc)(mock_impl))

#define MOCK_RESET(func_name) \
    mock_reset(#func_name)

#define MOCK_RESET_ALL() \
    mock_reset_all()

// =============================================================================
// SKIP/PENDING MACROS
// =============================================================================

#define TEST_SKIP_IF(condition, message) \
    do { \
        if (condition) { \
            printf("    [SKIP] %s\n", message); \
            return TEST_SKIP; \
        } \
    } while (0)

#define TEST_PENDING(message) \
    do { \
        printf("    [PENDING] %s\n", message); \
        return TEST_SKIP; \
    } while (0)

// =============================================================================
// CORE API DECLARATIONS
// =============================================================================

// Framework lifecycle
void test_init(void);
void test_cleanup(void);

// Test registration
void test_register(const char *suite, const char *name, TestFunc func,
                   TestFunc setup, TestFunc teardown);

// Test execution
TestStats test_run_all(void);
TestStats test_run_suite(const char *suite);
void test_list_all(void);

// Failure reporting (internal use)
void test_fail(const char *file, int line, const char *message);
void test_fail_eq(const char *file, int line, const char *message,
                  long long expected, long long actual);
void test_fail_float(const char *file, int line, const char *message,
                     double expected, double actual, double epsilon);
void test_fail_str(const char *file, int line, const char *message,
                   const char *expected, const char *actual);

// Memory leak detection
void test_enable_leak_detection(void);
void test_disable_leak_detection(void);
bool test_check_leaks(void);

// Mocking
void mock_register(const char *name, MockFunc original, MockFunc mock);
void mock_reset(const char *name);
void mock_reset_all(void);

// =============================================================================
// CONVENIENCE MACROS FOR COMMON PATTERNS
// =============================================================================

// Physics testing helpers
#define TEST_ASSERT_POSITION_NEAR(pos, expected, tolerance, msg) \
    TEST_ASSERT_VEC3_EQ(pos, expected, tolerance, msg)

// Rendering testing helpers
#define TEST_ASSERT_COLOR_EQ(c1, c2, msg) \
    do { \
        TEST_ASSERT_FLOAT_EQ((c1).r, (c2).r, 0.001f, msg " (red)"); \
        TEST_ASSERT_FLOAT_EQ((c1).g, (c2).g, 0.001f, msg " (green)"); \
        TEST_ASSERT_FLOAT_EQ((c1).b, (c2).b, 0.001f, msg " (blue)"); \
        TEST_ASSERT_FLOAT_EQ((c1).a, (c2).a, 0.001f, msg " (alpha)"); \
    } while (0)

// Audio testing helpers
#define TEST_ASSERT_SAMPLE_NEAR(sample, expected, tolerance, msg) \
    TEST_ASSERT_FLOAT_EQ(sample, expected, tolerance, msg)

// ECS testing helpers
#define TEST_ASSERT_ENTITY_VALID(entity, msg) \
    TEST_ASSERT((entity) != 0, msg)

#define TEST_ASSERT_ENTITY_INVALID(entity, msg) \
    TEST_ASSERT((entity) == 0, msg)

#endif // TEST_FRAMEWORK_UNIFIED_H
