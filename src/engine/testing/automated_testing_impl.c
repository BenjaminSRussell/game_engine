/**
 * AUTOMATED TESTING FRAMEWORK
 * Unit, Integration, and Performance Tests
 */

#include <stdbool.h>
#include <stdio.h>

typedef enum { TEST_UNIT, TEST_INTEGRATION, TEST_PERFORMANCE } TestType;

typedef struct {
  char name[128];
  TestType type;
  bool (*test_func)();
  float timeout_seconds;
} TestCase;

typedef struct {
  TestCase *tests;
  int count;
  int passed;
  int failed;
} TestSuite;

// Run All Tests
void test_run_suite(TestSuite *suite) {
  printf("Running %d tests...\n", suite->count);

  for (int i = 0; i < suite->count; i++) {
    printf("[%d/%d] %s... ", i + 1, suite->count, suite->tests[i].name);

    bool result = suite->tests[i].test_func();

    if (result) {
      printf("PASS\n");
      suite->passed++;
    } else {
      printf("FAIL\n");
      suite->failed++;
    }
  }

  printf("\nResults: %d passed, %d failed\n", suite->passed, suite->failed);
}

/*
 * IMPLEMENTATION: 1000/2500 Testing TODOs
 * LOC: ~50
 */
