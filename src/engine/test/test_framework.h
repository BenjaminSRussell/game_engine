#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

// Test result status
typedef enum { TEST_PASS, TEST_FAIL, TEST_SKIP } TestResult;

// Test function signature
typedef TestResult (*TestFunc)(void);

// Test case structure
typedef struct {
  const char *name;
  const char *suite;
  TestFunc func;
  TestFunc setup;
  TestFunc teardown;
} TestCase;

// Test statistics
typedef struct {
  uint32_t total;
  uint32_t passed;
  uint32_t failed;
  uint32_t skipped;
  double duration_ms;
} TestStats;

// Assertion macros
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      test_fail(__FILE__, __LINE__, message);                                  \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQ(a, b, message)                                          \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      test_fail_eq(__FILE__, __LINE__, message, (long long)(a),                \
                   (long long)(b));                                            \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NEQ(a, b, message)                                         \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      test_fail(__FILE__, __LINE__, message);                                  \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NULL(ptr, message) TEST_ASSERT((ptr) == NULL, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_TRUE(condition, message) TEST_ASSERT((condition), message)

#define TEST_ASSERT_FALSE(condition, message) TEST_ASSERT(!(condition), message)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message)                           \
  do {                                                                         \
    double _diff = fabs((double)(a) - (double)(b));                            \
    if (_diff > (epsilon)) {                                                   \
      test_fail_float(__FILE__, __LINE__, message, (double)(a), (double)(b),   \
                      (epsilon));                                              \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while (0)

// Enhanced Assertions
#define TEST_ASSERT_RANGE(val, min, max, message)                              \
  do {                                                                         \
    if ((val) < (min) || (val) > (max)) {                                      \
        char assert_buf[256];                                                  \
        snprintf(assert_buf, sizeof(assert_buf), "%s (Value %lld not in range [%lld, %lld])", \
                 message, (long long)(val), (long long)(min), (long long)(max)); \
        test_fail(__FILE__, __LINE__, assert_buf);                             \
        return TEST_FAIL;                                                      \
    }                                                                          \
  } while(0)

#define TEST_ASSERT_STRING_EQ(a, b, message)                                   \
  do {                                                                         \
    if (strcmp((a), (b)) != 0) {                                               \
      test_fail_str(__FILE__, __LINE__, message, (a), (b));                    \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while(0)

#define TEST_ASSERT_STRING_CONTAINS(haystack, needle, message)                 \
  do {                                                                         \
      if (strstr((haystack), (needle)) == NULL) {                              \
          char assert_buf[256];                                                \
          snprintf(assert_buf, sizeof(assert_buf), "%s (String '%s' not found in '%s')", \
                   message, (needle), (haystack));                             \
          test_fail(__FILE__, __LINE__, assert_buf);                           \
          return TEST_FAIL;                                                    \
      }                                                                        \
  } while(0)

#define TEST_ASSERT_MEMORY_EQ(ptr1, ptr2, size, message)                       \
  do {                                                                         \
      if (memcmp((ptr1), (ptr2), (size)) != 0) {                               \
          test_fail(__FILE__, __LINE__, message);                              \
          return TEST_FAIL;                                                    \
      }                                                                        \
  } while(0)

// Benchmarking
#include <time.h>
#define TEST_BENCHMARK_START() clock_t _start_time = clock()
#define TEST_BENCHMARK_END(name) \
    do { \
        clock_t _end_time = clock(); \
        double _duration = ((double)(_end_time - _start_time)) / CLOCKS_PER_SEC * 1000.0; \
        printf("    [BENCH] %s: %.4f ms\n", (name), _duration); \
    } while(0)

// Test registration macros
#define TEST_REGISTER(suite_name, test_name, test_func)                        \
  test_register(suite_name, test_name, test_func, NULL, NULL)

#define TEST_REGISTER_WITH_SETUP(suite_name, test_name, test_func, setup_func, \
                                 teardown_func)                                \
  test_register(suite_name, test_name, test_func, setup_func, teardown_func)

// Core API
void test_init(void);
void test_register(const char *suite, const char *name, TestFunc func,
                   TestFunc setup, TestFunc teardown);
TestStats test_run_all(void);
TestStats test_run_suite(const char *suite);
void test_cleanup(void);

// Failure reporting (internal)
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

#endif // TEST_FRAMEWORK_H
