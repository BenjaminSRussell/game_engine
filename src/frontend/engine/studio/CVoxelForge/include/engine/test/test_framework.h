#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

// Test registration
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

// Memory leak detection
void test_enable_leak_detection(void);
void test_disable_leak_detection(void);
bool test_check_leaks(void);

#endif // TEST_FRAMEWORK_H
