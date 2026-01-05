/**
 * =================================================================================================
 *                              TESTING FRAMEWORK - CORE
 *                                 Agent: AGENT_TEST_1
 * =================================================================================================
 *
 * Comprehensive testing framework for unit tests, integration tests,
 * performance benchmarks, and visual regression testing.
 *
 * =================================================================================================
 */

#ifndef TESTING_FRAMEWORK_CORE_H
#define TESTING_FRAMEWORK_CORE_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    TEST MACROS
 * =================================================================================================
 */

#define TEST_ASSERT(expr) test_assert((expr), #expr, __FILE__, __LINE__)
#define TEST_ASSERT_EQ(a, b)                                                   \
  test_assert_equal((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_NE(a, b)                                                   \
  test_assert_not_equal((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_LT(a, b)                                                   \
  test_assert_less_than((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_LE(a, b)                                                   \
  test_assert_less_equal((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_GT(a, b)                                                   \
  test_assert_greater_than((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_GE(a, b)                                                   \
  test_assert_greater_equal((a), (b), #a, #b, __FILE__, __LINE__)
#define TEST_ASSERT_NULL(ptr) test_assert_null((ptr), #ptr, __FILE__, __LINE__)
#define TEST_ASSERT_NOT_NULL(ptr)                                              \
  test_assert_not_null((ptr), #ptr, __FILE__, __LINE__)
#define TEST_ASSERT_STR_EQ(a, b)                                               \
  test_assert_string_equal((a), (b), __FILE__, __LINE__)
#define TEST_ASSERT_FLOAT_EQ(a, b, eps)                                        \
  test_assert_float_equal((a), (b), (eps), __FILE__, __LINE__)
#define TEST_FAIL(msg) test_fail((msg), __FILE__, __LINE__)
#define TEST_SKIP(reason) test_skip((reason), __FILE__, __LINE__)

// TODO(AGENT_TEST_1): Add memory comparison assertions [Difficulty: 4]
// TODO(AGENT_TEST_1): Add array comparison assertions [Difficulty: 4]
// TODO(AGENT_TEST_1): Add custom assertion messages [Difficulty: 3]
// TODO(AGENT_TEST_1): Add exception/signal handling assertions [Difficulty: 6]

/* =================================================================================================
 *                                    TEST RESULT
 * =================================================================================================
 */

typedef enum TestResult {
  TEST_RESULT_PASS,
  TEST_RESULT_FAIL,
  TEST_RESULT_SKIP,
  TEST_RESULT_ERROR,
  TEST_RESULT_TIMEOUT,
} TestResult;

typedef struct TestCaseResult {
  char test_name[128];
  char suite_name[64];
  TestResult result;
  char message[512];
  char file[256];
  int line;
  double duration_ms;
} TestCaseResult;

// TODO(AGENT_TEST_1): Implement result serialization to JSON [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement result serialization to JUnit XML [Difficulty:
// 5]
// TODO(AGENT_TEST_1): Implement result diff on failure [Difficulty: 5]

/* =================================================================================================
 *                                    TEST CASE
 * =================================================================================================
 */

typedef void (*TestFunction)(void);
typedef void (*TestSetupFunction)(void);
typedef void (*TestTeardownFunction)(void);

typedef struct TestCase {
  char name[128];
  TestFunction func;
  TestSetupFunction setup;
  TestTeardownFunction teardown;
  bool is_parameterized;
  void **param_values;
  uint32_t param_count;
  float timeout_ms;
  bool should_fail; // Expected to fail
  char *tags[8];
  uint32_t tag_count;
} TestCase;

// TODO(AGENT_TEST_1): Implement parameterized test support [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement test timeout handling [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement test expected failure [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement test tagging and filtering [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement test fixtures [Difficulty: 5]

/* =================================================================================================
 *                                    TEST SUITE
 * =================================================================================================
 */

typedef struct TestSuite {
  char name[64];
  TestCase *tests;
  uint32_t test_count;
  uint32_t test_capacity;
  TestSetupFunction suite_setup;
  TestTeardownFunction suite_teardown;
  bool is_parallel;
} TestSuite;

// TODO(AGENT_TEST_1): Implement suite registration [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement suite discovery from symbols [Difficulty: 7]
// TODO(AGENT_TEST_1): Implement suite parallel execution [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement suite dependency ordering [Difficulty: 5]

/* =================================================================================================
 *                                    TEST RUNNER
 * =================================================================================================
 */

typedef struct TestRunner {
  TestSuite **suites;
  uint32_t suite_count;
  TestCaseResult *results;
  uint32_t result_count;
  uint32_t passed;
  uint32_t failed;
  uint32_t skipped;
  double total_duration_ms;
  bool stop_on_first_fail;
  char filter_pattern[128];
  bool run_in_parallel;
  uint32_t thread_count;
} TestRunner;

// TODO(AGENT_TEST_1): Implement test_runner_init [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement test_runner_run_all [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement test_runner_run_suite [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement test_runner_run_filtered [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement test_runner_report [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement test isolation (fork per test) [Difficulty: 7]
// TODO(AGENT_TEST_1): Implement test randomization [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement test retry on flaky [Difficulty: 5]

/* =================================================================================================
 *                                    MOCKING
 * =================================================================================================
 */

typedef struct MockExpectation {
  char function_name[64];
  void *return_value;
  void **arg_matchers;
  uint32_t arg_count;
  uint32_t call_count;
  uint32_t expected_calls;
} MockExpectation;

// TODO(AGENT_TEST_1): Implement mock function creation [Difficulty: 7]
// TODO(AGENT_TEST_1): Implement expectation matching [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement return value stubs [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement call verification [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement argument capture [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement spy functions [Difficulty: 6]

/* =================================================================================================
 *                                    BENCHMARKING
 * =================================================================================================
 */

typedef struct BenchmarkResult {
  char name[128];
  uint64_t iterations;
  double total_time_ms;
  double avg_time_ns;
  double min_time_ns;
  double max_time_ns;
  double std_dev_ns;
  uint64_t memory_used;
  uint64_t allocations;
} BenchmarkResult;

// TODO(AGENT_TEST_1): Implement benchmark registration [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement benchmark warmup [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement benchmark iteration calibration [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement benchmark memory tracking [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement benchmark comparison (baseline) [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement benchmark report generation [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement benchmark CI integration [Difficulty: 6]

/* =================================================================================================
 *                                    VISUAL TESTING
 * =================================================================================================
 */

typedef struct ScreenshotTest {
  char name[128];
  char reference_path[256];
  char output_path[256];
  float tolerance;
  bool ignore_antialiasing;
  struct {
    int x, y, w, h;
  } compare_region;
} ScreenshotTest;

// TODO(AGENT_TEST_1): Implement screenshot capture [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement screenshot comparison [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement diff image generation [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement baseline update [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement region masking [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement perceptual diff (SSIM) [Difficulty: 7]

/* =================================================================================================
 *                                    CODE COVERAGE
 * =================================================================================================
 */

typedef struct CoverageData {
  char file_path[256];
  uint32_t *line_hits;
  uint32_t line_count;
  uint32_t lines_covered;
  float coverage_percent;
} CoverageData;

// TODO(AGENT_TEST_1): Implement coverage instrumentation [Difficulty: 8]
// TODO(AGENT_TEST_1): Implement coverage data collection [Difficulty: 6]
// TODO(AGENT_TEST_1): Implement coverage report HTML [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement coverage report LCOV [Difficulty: 5]
// TODO(AGENT_TEST_1): Implement coverage thresholds [Difficulty: 4]
// TODO(AGENT_TEST_1): Implement branch coverage [Difficulty: 7]

#endif // TESTING_FRAMEWORK_CORE_H
