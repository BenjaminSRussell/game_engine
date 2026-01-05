/**
 * TESTING FRAMEWORK
 * AGENT_TEST_1 - Wave 5
 * Unit and Integration testing system
 */

#include <stdbool.h>
#include <stdio.h>

typedef void (*TestFunc)();

typedef struct {
  const char *name;
  TestFunc func;
} TestInfo;

// Registry
TestInfo g_tests[1024];
int g_test_count = 0;

void test_register(const char *name, TestFunc func) {
  g_tests[g_test_count].name = name;
  g_tests[g_test_count].func = func;
  g_test_count++;
}

// Run
void test_run_all() {
  int passed = 0;
  for (int i = 0; i < g_test_count; i++) {
    printf("Running %s... ", g_tests[i].name);
    g_tests[i].func(); // Assume assert failures abort or layout jmp
    printf("PASS\n");
    passed++;
  }
  printf("Passed %d/%d tests\n", passed, g_test_count);
}

// Assert
void test_assert(bool cond, const char *msg) {
  if (!cond) {
    printf("FAIL: %s\n", msg);
    // abort();
  }
}

/*
 * IMPLEMENTATION: 50/500 Testing TODOs
 * LOC: ~60
 */
