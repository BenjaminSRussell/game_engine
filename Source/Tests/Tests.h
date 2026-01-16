#ifndef ULTIMATE_ENGINE_TESTS_H
#define ULTIMATE_ENGINE_TESTS_H

#include <stdbool.h>
#include <stdio.h>

// Simple Test Framework Shim
typedef void (*TestFunc)(void);

#define RUN_TEST_SUITE(SuiteName, passed_ptr, total_ptr)                       \
  do {                                                                         \
    printf("Running Suite: " #SuiteName "\n");                                 \
    SuiteName(passed_ptr, total_ptr);                                          \
  } while (0)

#define TEST_ASSERT(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("  FAILED: " #cond " at %s:%d\n", __FILE__, __LINE__);            \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_CASE(name)                                                        \
  printf("  Test: " #name "\n");                                               \
  if (name()) {                                                                \
    printf("    PASSED\n");                                                    \
    (*passed)++;                                                               \
  } else {                                                                     \
    printf("    FAILED\n");                                                    \
  }                                                                            \
  (*total)++;

// Forward declare test suites
void Core_Engine_Tests(int *passed, int *total);
void Memory_Tests(int *passed, int *total);
void Threading_Tests(int *passed, int *total);
void ECS_Tests(int *passed, int *total);
void Render_Tests(int *passed, int *total);
void Physics_Tests(int *passed, int *total);
void AI_Tests(int *passed, int *total);
void Audio_Tests(int *passed, int *total);
void Animation_Tests(int *passed, int *total);
void Network_Tests(int *passed, int *total);
void UI_Tests(int *passed, int *total);

#endif // ULTIMATE_ENGINE_TESTS_H
