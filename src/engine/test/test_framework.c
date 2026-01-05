// test/test_framework.c - Test framework implementation
#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TESTS 1024
#define MAX_SUITE_NAME 64
#define MAX_TEST_NAME 128

// ANSI color codes
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"

// Test registry
static TestCase tests[MAX_TESTS];
static uint32_t test_count = 0;

// Memory tracking (simple implementation)
static size_t allocated_bytes = 0;
static size_t freed_bytes = 0;
static bool leak_detection_enabled = false;

// Current test context
static const char *current_file = NULL;
static int current_line = 0;

// Initialize test framework
void test_init(void) {
  test_count = 0;
  allocated_bytes = 0;
  freed_bytes = 0;
  leak_detection_enabled = false;
}

// Register a test
void test_register(const char *suite, const char *name, TestFunc func,
                   TestFunc setup, TestFunc teardown) {
  if (test_count >= MAX_TESTS) {
    fprintf(stderr, "Error: Maximum test count (%d) exceeded\n", MAX_TESTS);
    return;
  }

  TestCase *test = &tests[test_count++];
  test->suite = suite;
  test->name = name;
  test->func = func;
  test->setup = setup;
  test->teardown = teardown;
}

// Run a single test
static TestResult run_test(const TestCase *test, bool verbose) {
  TestResult result = TEST_PASS;

  if (verbose) {
    printf("  %s%-60s%s", COLOR_CYAN, test->name, COLOR_RESET);
    fflush(stdout);
  }

  // Run setup if provided
  if (test->setup) {
    result = test->setup();
    if (result != TEST_PASS) {
      if (verbose) {
        printf(" [%sSETUP FAILED%s]\n", COLOR_RED, COLOR_RESET);
      }
      return result;
    }
  }

  // Run test
  result = test->func();

  // Run teardown if provided
  if (test->teardown) {
    TestResult teardown_result = test->teardown();
    if (teardown_result != TEST_PASS && result == TEST_PASS) {
      result = teardown_result;
      if (verbose) {
        printf(" [%sTEARDOWN FAILED%s]\n", COLOR_RED, COLOR_RESET);
      }
      return result;
    }
  }

  // Print result
  if (verbose) {
    switch (result) {
    case TEST_PASS:
      printf(" [%sPASS%s]\n", COLOR_GREEN, COLOR_RESET);
      break;
    case TEST_FAIL:
      printf(" [%sFAIL%s]\n", COLOR_RED, COLOR_RESET);
      break;
    case TEST_SKIP:
      printf(" [%sSKIP%s]\n", COLOR_YELLOW, COLOR_RESET);
      break;
    }
  }

  return result;
}

// Run all tests
TestStats test_run_all(void) {
  TestStats stats = {0};
  clock_t start = clock();

  const char *current_suite = NULL;

  for (uint32_t i = 0; i < test_count; i++) {
    const TestCase *test = &tests[i];

    // Print suite header if changed
    if (current_suite == NULL || strcmp(current_suite, test->suite) != 0) {
      current_suite = test->suite;
      printf("\n%s[%s]%s\n", COLOR_MAGENTA, current_suite, COLOR_RESET);
    }

    // Run test
    TestResult result = run_test(test, true);

    // Update statistics
    stats.total++;
    switch (result) {
    case TEST_PASS:
      stats.passed++;
      break;
    case TEST_FAIL:
      stats.failed++;
      break;
    case TEST_SKIP:
      stats.skipped++;
      break;
    }
  }

  clock_t end = clock();
  stats.duration_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

  return stats;
}

// Run tests in a specific suite
TestStats test_run_suite(const char *suite) {
  TestStats stats = {0};
  clock_t start = clock();

  printf("\n%s[%s]%s\n", COLOR_MAGENTA, suite, COLOR_RESET);

  for (uint32_t i = 0; i < test_count; i++) {
    const TestCase *test = &tests[i];

    // Skip tests not in this suite
    if (strcmp(test->suite, suite) != 0) {
      continue;
    }

    // Run test
    TestResult result = run_test(test, true);

    // Update statistics
    stats.total++;
    switch (result) {
    case TEST_PASS:
      stats.passed++;
      break;
    case TEST_FAIL:
      stats.failed++;
      break;
    case TEST_SKIP:
      stats.skipped++;
      break;
    }
  }

  clock_t end = clock();
  stats.duration_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

  return stats;
}

// Cleanup test framework
void test_cleanup(void) { test_count = 0; }

// Failure reporting
void test_fail(const char *file, int line, const char *message) {
  printf("\n    %sAssertion failed:%s %s\n", COLOR_RED, COLOR_RESET, message);
  printf("    %sLocation:%s %s:%d\n", COLOR_YELLOW, COLOR_RESET, file, line);
}

void test_fail_eq(const char *file, int line, const char *message,
                  long long expected, long long actual) {
  printf("\n    %sAssertion failed:%s %s\n", COLOR_RED, COLOR_RESET, message);
  printf("    %sExpected:%s %lld\n", COLOR_YELLOW, COLOR_RESET, expected);
  printf("    %sActual:%s %lld\n", COLOR_YELLOW, COLOR_RESET, actual);
  printf("    %sLocation:%s %s:%d\n", COLOR_YELLOW, COLOR_RESET, file, line);
}

void test_fail_float(const char *file, int line, const char *message,
                     double expected, double actual, double epsilon) {
  printf("\n    %sAssertion failed:%s %s\n", COLOR_RED, COLOR_RESET, message);
  printf("    %sExpected:%s %.6f\n", COLOR_YELLOW, COLOR_RESET, expected);
  printf("    %sActual:%s %.6f\n", COLOR_YELLOW, COLOR_RESET, actual);
  printf("    %sDifference:%s %.6f (epsilon: %.6f)\n", COLOR_YELLOW,
         COLOR_RESET, fabs(expected - actual), epsilon);
  printf("    %sLocation:%s %s:%d\n", COLOR_YELLOW, COLOR_RESET, file, line);
}

// Memory leak detection (simple implementation)
void test_enable_leak_detection(void) {
  leak_detection_enabled = true;
  allocated_bytes = 0;
  freed_bytes = 0;
}

void test_disable_leak_detection(void) { leak_detection_enabled = false; }

bool test_check_leaks(void) {
  if (!leak_detection_enabled) {
    return false;
  }

  if (allocated_bytes != freed_bytes) {
    printf("\n%sMemory leak detected:%s\n", COLOR_RED, COLOR_RESET);
    printf("  Allocated: %zu bytes\n", allocated_bytes);
    printf("  Freed: %zu bytes\n", freed_bytes);
    printf("  Leaked: %zu bytes\n", allocated_bytes - freed_bytes);
    return true;
  }

  return false;
}
