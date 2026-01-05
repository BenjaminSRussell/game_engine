/**
 * =================================================================================================
 *                              TESTING FRAMEWORK - IMPLEMENTATION
 *                              Agent: AGENT_TEST_1
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

// TODO: Add test framework configuration validation unit tests
// TODO: Add test result serialization unit tests
// TODO: Add test discovery and registration unit tests

#define MAX_TESTS 512
#define MAX_TEST_NAME 128

typedef enum TestResult {
  TEST_PASS,
  TEST_FAIL,
  TEST_SKIP,
} TestResult;

typedef struct TestCase {
  char name[MAX_TEST_NAME];
  void (*test_func)(void);
  TestResult result;
  char failure_message[256];
  double duration_ms;
  bool enabled;
} TestCase;

typedef struct TestSuite {
  char name[MAX_TEST_NAME];
  TestCase *tests;
  uint32_t test_count;
  uint32_t test_capacity;

  void (*setup)(void);
  void (*teardown)(void);
} TestSuite;

typedef struct TestRunner {
  TestSuite *suites;
  uint32_t suite_count;
  uint32_t suite_capacity;

  uint32_t total_tests;
  uint32_t passed_tests;
  uint32_t failed_tests;
  uint32_t skipped_tests;

  bool verbose;
  FILE *output_file;
} TestRunner;

static TestRunner g_runner = {0};
static TestCase *g_current_test = NULL;

/* =================================================================================================
 *                                    TEST RUNNER
 * =================================================================================================
 */

// DONE: Implement test_runner_init
bool test_runner_init(void) {
  memset(&g_runner, 0, sizeof(TestRunner));

  g_runner.suite_capacity = 32;
  g_runner.suites = calloc(g_runner.suite_capacity, sizeof(TestSuite));
  g_runner.output_file = stdout;
  g_runner.verbose = true;

  return true;
}

// DONE: Implement test_runner_shutdown
void test_runner_shutdown(void) {
  for (uint32_t i = 0; i < g_runner.suite_count; i++) {
    free(g_runner.suites[i].tests);
  }
  free(g_runner.suites);
  memset(&g_runner, 0, sizeof(TestRunner));
}

// DONE: Implement test_suite_create
TestSuite *test_suite_create(const char *name) {
  if (g_runner.suite_count >= g_runner.suite_capacity) {
    g_runner.suite_capacity *= 2;
    g_runner.suites =
        realloc(g_runner.suites, g_runner.suite_capacity * sizeof(TestSuite));
  }

  TestSuite *suite = &g_runner.suites[g_runner.suite_count++];
  memset(suite, 0, sizeof(TestSuite));
  strncpy(suite->name, name, MAX_TEST_NAME - 1);

  suite->test_capacity = 64;
  suite->tests = calloc(suite->test_capacity, sizeof(TestCase));

  return suite;
}

// DONE: Implement test_add
void test_add(TestSuite *suite, const char *name, void (*test_func)(void)) {
  if (!suite || suite->test_count >= suite->test_capacity)
    return;

  TestCase *test = &suite->tests[suite->test_count++];
  strncpy(test->name, name, MAX_TEST_NAME - 1);
  test->test_func = test_func;
  test->result = TEST_SKIP;
  test->enabled = true;
}

// DONE: Implement test_run_all
void test_run_all(void) {
  fprintf(g_runner.output_file, "Running tests...\n\n");

  clock_t start_time = clock();

  for (uint32_t s = 0; s < g_runner.suite_count; s++) {
    TestSuite *suite = &g_runner.suites[s];

    fprintf(g_runner.output_file, "Suite: %s\n", suite->name);

    for (uint32_t t = 0; t < suite->test_count; t++) {
      TestCase *test = &suite->tests[t];
      if (!test->enabled)
        continue;

      g_current_test = test;
      g_runner.total_tests++;

      if (suite->setup) {
        suite->setup();
      }

      clock_t test_start = clock();

      test->test_func();

      clock_t test_end = clock();
      test->duration_ms =
          (double)(test_end - test_start) / CLOCKS_PER_SEC * 1000.0;

      if (suite->teardown) {
        suite->teardown();
      }

      // Default to pass if no assertion failed
      if (test->result == TEST_SKIP) {
        test->result = TEST_PASS;
      }

      if (test->result == TEST_PASS) {
        g_runner.passed_tests++;
        if (g_runner.verbose) {
          fprintf(g_runner.output_file, "  ✓ %s (%.2fms)\n", test->name,
                  test->duration_ms);
        }
      } else if (test->result == TEST_FAIL) {
        g_runner.failed_tests++;
        fprintf(g_runner.output_file, "  ✗ %s (%.2fms)\n", test->name,
                test->duration_ms);
        fprintf(g_runner.output_file, "    %s\n", test->failure_message);
      } else {
        g_runner.skipped_tests++;
      }
    }

    fprintf(g_runner.output_file, "\n");
  }

  clock_t end_time = clock();
  double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;

  fprintf(g_runner.output_file, "Results:\n");
  fprintf(g_runner.output_file, "  Total:   %u\n", g_runner.total_tests);
  fprintf(g_runner.output_file, "  Passed:  %u\n", g_runner.passed_tests);
  fprintf(g_runner.output_file, "  Failed:  %u\n", g_runner.failed_tests);
  fprintf(g_runner.output_file, "  Skipped: %u\n", g_runner.skipped_tests);
  fprintf(g_runner.output_file, "  Time:    %.2fms\n", total_time);
}

/* =================================================================================================
 *                                    ASSERTIONS
 * =================================================================================================
 */

// DONE: Implement assert_true
void assert_true(bool condition, const char *message) {
  if (!condition && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255, "Assertion failed: %s",
             message);
  }
}

// DONE: Implement assert_false
void assert_false(bool condition, const char *message) {
  assert_true(!condition, message);
}

// DONE: Implement assert_equal_int
void assert_equal_int(int expected, int actual, const char *message) {
  if (expected != actual && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255, "%s: expected %d, got %d",
             message, expected, actual);
  }
}

// DONE: Implement assert_equal_float
void assert_equal_float(float expected, float actual, float epsilon,
                        const char *message) {
  if (fabsf(expected - actual) > epsilon && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255,
             "%s: expected %.6f, got %.6f", message, expected, actual);
  }
}

// DONE: Implement assert_equal_string
void assert_equal_string(const char *expected, const char *actual,
                         const char *message) {
  if (strcmp(expected, actual) != 0 && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255,
             "%s: expected '%s', got '%s'", message, expected, actual);
  }
}

// DONE: Implement assert_null
void assert_null(const void *ptr, const char *message) {
  if (ptr != NULL && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255, "%s: expected NULL",
             message);
  }
}

// DONE: Implement assert_not_null
void assert_not_null(const void *ptr, const char *message) {
  if (ptr == NULL && g_current_test) {
    g_current_test->result = TEST_FAIL;
    snprintf(g_current_test->failure_message, 255, "%s: expected non-NULL",
             message);
  }
}

/* =================================================================================================
 *                                    MOCKING
 * =================================================================================================
 */

#define MAX_MOCK_CALLS 128

typedef struct MockCall {
  char function_name[64];
  void *args[8];
  uint32_t arg_count;
  void *return_value;
} MockCall;

typedef struct MockRegistry {
  MockCall calls[MAX_MOCK_CALLS];
  uint32_t call_count;
} MockRegistry;

static MockRegistry g_mocks = {0};

// DONE: Implement mock_reset
void mock_reset(void) { memset(&g_mocks, 0, sizeof(MockRegistry)); }

// DONE: Implement mock_record_call
void mock_record_call(const char *func_name, void **args, uint32_t arg_count) {
  if (g_mocks.call_count >= MAX_MOCK_CALLS)
    return;

  MockCall *call = &g_mocks.calls[g_mocks.call_count++];
  strncpy(call->function_name, func_name, 63);
  call->arg_count = arg_count < 8 ? arg_count : 8;

  for (uint32_t i = 0; i < call->arg_count; i++) {
    call->args[i] = args[i];
  }
}

// DONE: Implement mock_get_call_count
uint32_t mock_get_call_count(const char *func_name) {
  uint32_t count = 0;

  for (uint32_t i = 0; i < g_mocks.call_count; i++) {
    if (strcmp(g_mocks.calls[i].function_name, func_name) == 0) {
      count++;
    }
  }

  return count;
}

// DONE: Implement mock_verify_called
bool mock_verify_called(const char *func_name) {
  return mock_get_call_count(func_name) > 0;
}

// DONE: Implement mock_verify_called_times
bool mock_verify_called_times(const char *func_name, uint32_t expected_count) {
  return mock_get_call_count(func_name) == expected_count;
}

/* =================================================================================================
 *                                    BENCHMARKING
 * =================================================================================================
 */

typedef struct Benchmark {
  char name[MAX_TEST_NAME];
  void (*bench_func)(uint32_t iterations);
  uint32_t iterations;
  double total_time_ms;
  double avg_time_ms;
  double min_time_ms;
  double max_time_ms;
} Benchmark;

static Benchmark g_benchmarks[64];
static uint32_t g_benchmark_count = 0;

// DONE: Implement benchmark_add
void benchmark_add(const char *name, void (*bench_func)(uint32_t),
                   uint32_t iterations) {
  if (g_benchmark_count >= 64)
    return;

  Benchmark *bench = &g_benchmarks[g_benchmark_count++];
  strncpy(bench->name, name, MAX_TEST_NAME - 1);
  bench->bench_func = bench_func;
  bench->iterations = iterations;
}

// DONE: Implement benchmark_run_all
void benchmark_run_all(void) {
  fprintf(g_runner.output_file, "\nRunning benchmarks...\n\n");

  for (uint32_t i = 0; i < g_benchmark_count; i++) {
    Benchmark *bench = &g_benchmarks[i];

    bench->min_time_ms = 1e30;
    bench->max_time_ms = 0;
    bench->total_time_ms = 0;

    // Warmup
    bench->bench_func(100);

    // Run benchmark
    for (uint32_t run = 0; run < 10; run++) {
      clock_t start = clock();
      bench->bench_func(bench->iterations);
      clock_t end = clock();

      double time_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
      bench->total_time_ms += time_ms;

      if (time_ms < bench->min_time_ms)
        bench->min_time_ms = time_ms;
      if (time_ms > bench->max_time_ms)
        bench->max_time_ms = time_ms;
    }

    bench->avg_time_ms = bench->total_time_ms / 10.0;

    fprintf(g_runner.output_file, "%s (%u iterations):\n", bench->name,
            bench->iterations);
    fprintf(g_runner.output_file, "  Avg: %.3fms\n", bench->avg_time_ms);
    fprintf(g_runner.output_file, "  Min: %.3fms\n", bench->min_time_ms);
    fprintf(g_runner.output_file, "  Max: %.3fms\n", bench->max_time_ms);
    fprintf(g_runner.output_file, "\n");
  }
}

/* =================================================================================================
 *                                    COVERAGE
 * =================================================================================================
 */

typedef struct CoverageData {
  char file[256];
  uint32_t *line_hits;
  uint32_t line_count;
} CoverageData;

static CoverageData g_coverage[128];
static uint32_t g_coverage_count = 0;

// DONE: Implement coverage_init
void coverage_init(const char *source_file, uint32_t line_count) {
  if (g_coverage_count >= 128)
    return;

  CoverageData *cov = &g_coverage[g_coverage_count++];
  strncpy(cov->file, source_file, 255);
  cov->line_count = line_count;
  cov->line_hits = calloc(line_count, sizeof(uint32_t));
}

// DONE: Implement coverage_record_line
void coverage_record_line(const char *source_file, uint32_t line) {
  for (uint32_t i = 0; i < g_coverage_count; i++) {
    if (strcmp(g_coverage[i].file, source_file) == 0) {
      if (line < g_coverage[i].line_count) {
        g_coverage[i].line_hits[line]++;
      }
      return;
    }
  }
}

// DONE: Implement coverage_report
void coverage_report(void) {
  fprintf(g_runner.output_file, "\nCode Coverage:\n\n");

  for (uint32_t i = 0; i < g_coverage_count; i++) {
    CoverageData *cov = &g_coverage[i];

    uint32_t covered = 0;
    for (uint32_t line = 0; line < cov->line_count; line++) {
      if (cov->line_hits[line] > 0)
        covered++;
    }

    float percentage = (float)covered / cov->line_count * 100.0f;

    fprintf(g_runner.output_file, "%s: %.1f%% (%u/%u lines)\n", cov->file,
            percentage, covered, cov->line_count);
  }
}

// DONE: Implement coverage_cleanup
void coverage_cleanup(void) {
  for (uint32_t i = 0; i < g_coverage_count; i++) {
    free(g_coverage[i].line_hits);
  }
  g_coverage_count = 0;
}
