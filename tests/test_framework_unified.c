/**
 * UNIFIED TEST FRAMEWORK - Implementation
 * Comprehensive Testing Framework for Game Engine TDD Suite
 */

#include "test_framework_unified.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// CONFIGURATION
// =============================================================================

#define MAX_TESTS 2048
#define MAX_MOCKS 64

// =============================================================================
// TEST REGISTRY
// =============================================================================

static TestCase g_tests[MAX_TESTS];
static uint32_t g_test_count = 0;

// =============================================================================
// MEMORY TRACKING
// =============================================================================

static size_t g_allocated_bytes = 0;
static size_t g_freed_bytes = 0;
static bool g_leak_detection_enabled = false;

// =============================================================================
// MOCK REGISTRY
// =============================================================================

static MockEntry g_mocks[MAX_MOCKS];
static uint32_t g_mock_count = 0;

// =============================================================================
// FRAMEWORK LIFECYCLE
// =============================================================================

void test_init(void) {
    g_test_count = 0;
    g_mock_count = 0;
    g_allocated_bytes = 0;
    g_freed_bytes = 0;
    g_leak_detection_enabled = false;
}

void test_cleanup(void) {
    mock_reset_all();
    g_test_count = 0;
    g_mock_count = 0;
}

// =============================================================================
// TEST REGISTRATION
// =============================================================================

void test_register(const char *suite, const char *name, TestFunc func,
                   TestFunc setup, TestFunc teardown) {
    if (g_test_count >= MAX_TESTS) {
        fprintf(stderr, COLOR_RED "Error: Maximum test count (%d) exceeded" 
                COLOR_RESET "\n", MAX_TESTS);
        return;
    }
    
    TestCase *test = &g_tests[g_test_count++];
    test->suite = suite;
    test->name = name;
    test->func = func;
    test->setup = setup;
    test->teardown = teardown;
}

// =============================================================================
// TEST EXECUTION
// =============================================================================

static TestResult run_single_test(const TestCase *test, bool verbose) {
    TestResult result = TEST_PASS;
    
    if (verbose) {
        printf("  " COLOR_CYAN "%-55s" COLOR_RESET, test->name);
        fflush(stdout);
    }
    
    // Run setup
    if (test->setup) {
        result = test->setup();
        if (result != TEST_PASS) {
            if (verbose) {
                printf(" [" COLOR_RED "SETUP FAILED" COLOR_RESET "]\n");
            }
            return result;
        }
    }
    
    // Run test
    result = test->func();
    
    // Run teardown
    if (test->teardown) {
        TestResult td_result = test->teardown();
        if (td_result != TEST_PASS && result == TEST_PASS) {
            result = td_result;
            if (verbose) {
                printf(" [" COLOR_RED "TEARDOWN FAILED" COLOR_RESET "]\n");
            }
            return result;
        }
    }
    
    // Print result
    if (verbose) {
        switch (result) {
        case TEST_PASS:
            printf(" [" COLOR_GREEN "PASS" COLOR_RESET "]\n");
            break;
        case TEST_FAIL:
            printf(" [" COLOR_RED "FAIL" COLOR_RESET "]\n");
            break;
        case TEST_SKIP:
            printf(" [" COLOR_YELLOW "SKIP" COLOR_RESET "]\n");
            break;
        }
    }
    
    return result;
}

TestStats test_run_all(void) {
    TestStats stats = {0};
    clock_t start = clock();
    
    const char *current_suite = NULL;
    
    for (uint32_t i = 0; i < g_test_count; i++) {
        const TestCase *test = &g_tests[i];
        
        // Print suite header on change
        if (!current_suite || strcmp(current_suite, test->suite) != 0) {
            current_suite = test->suite;
            printf("\n" COLOR_MAGENTA COLOR_BOLD "[%s]" COLOR_RESET "\n", current_suite);
        }
        
        TestResult result = run_single_test(test, true);
        
        stats.total++;
        switch (result) {
        case TEST_PASS:  stats.passed++;  break;
        case TEST_FAIL:  stats.failed++;  break;
        case TEST_SKIP:  stats.skipped++; break;
        }
    }
    
    clock_t end = clock();
    stats.duration_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    
    return stats;
}

TestStats test_run_suite(const char *suite) {
    TestStats stats = {0};
    clock_t start = clock();
    
    printf("\n" COLOR_MAGENTA COLOR_BOLD "[%s]" COLOR_RESET "\n", suite);
    
    for (uint32_t i = 0; i < g_test_count; i++) {
        const TestCase *test = &g_tests[i];
        
        if (strcmp(test->suite, suite) != 0) {
            continue;
        }
        
        TestResult result = run_single_test(test, true);
        
        stats.total++;
        switch (result) {
        case TEST_PASS:  stats.passed++;  break;
        case TEST_FAIL:  stats.failed++;  break;
        case TEST_SKIP:  stats.skipped++; break;
        }
    }
    
    clock_t end = clock();
    stats.duration_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    
    return stats;
}

void test_list_all(void) {
    printf("\nRegistered Tests (%u total):\n", g_test_count);
    printf("────────────────────────────────────────────────────────────────────\n");
    
    const char *current_suite = NULL;
    uint32_t suite_count = 0;
    
    for (uint32_t i = 0; i < g_test_count; i++) {
        const TestCase *test = &g_tests[i];
        
        if (!current_suite || strcmp(current_suite, test->suite) != 0) {
            if (current_suite) {
                printf("  └─ (%u tests)\n\n", suite_count);
            }
            current_suite = test->suite;
            suite_count = 0;
            printf(COLOR_CYAN "%s" COLOR_RESET "\n", current_suite);
        }
        
        printf("  ├─ %s\n", test->name);
        suite_count++;
    }
    
    if (current_suite) {
        printf("  └─ (%u tests)\n", suite_count);
    }
    printf("────────────────────────────────────────────────────────────────────\n");
}

// =============================================================================
// FAILURE REPORTING
// =============================================================================

void test_fail(const char *file, int line, const char *message) {
    printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s\n", message);
    printf("    " COLOR_YELLOW "Location:" COLOR_RESET " %s:%d\n", file, line);
}

void test_fail_eq(const char *file, int line, const char *message,
                  long long expected, long long actual) {
    printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s\n", message);
    printf("    " COLOR_YELLOW "Expected:" COLOR_RESET " %lld\n", expected);
    printf("    " COLOR_YELLOW "Actual:" COLOR_RESET "   %lld\n", actual);
    printf("    " COLOR_YELLOW "Location:" COLOR_RESET " %s:%d\n", file, line);
}

void test_fail_float(const char *file, int line, const char *message,
                     double expected, double actual, double epsilon) {
    printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s\n", message);
    printf("    " COLOR_YELLOW "Expected:" COLOR_RESET " %.6f\n", expected);
    printf("    " COLOR_YELLOW "Actual:" COLOR_RESET "   %.6f\n", actual);
    printf("    " COLOR_YELLOW "Diff:" COLOR_RESET "     %.6f (ε=%.6f)\n", 
           fabs(expected - actual), epsilon);
    printf("    " COLOR_YELLOW "Location:" COLOR_RESET " %s:%d\n", file, line);
}

void test_fail_str(const char *file, int line, const char *message,
                   const char *expected, const char *actual) {
    printf("\n    " COLOR_RED "Assertion failed:" COLOR_RESET " %s\n", message);
    printf("    " COLOR_YELLOW "Expected:" COLOR_RESET " \"%s\"\n", expected);
    printf("    " COLOR_YELLOW "Actual:" COLOR_RESET "   \"%s\"\n", actual);
    printf("    " COLOR_YELLOW "Location:" COLOR_RESET " %s:%d\n", file, line);
}

// =============================================================================
// MEMORY LEAK DETECTION
// =============================================================================

void test_enable_leak_detection(void) {
    g_leak_detection_enabled = true;
    g_allocated_bytes = 0;
    g_freed_bytes = 0;
}

void test_disable_leak_detection(void) {
    g_leak_detection_enabled = false;
}

bool test_check_leaks(void) {
    if (!g_leak_detection_enabled) {
        return false;
    }
    
    if (g_allocated_bytes != g_freed_bytes) {
        printf("\n" COLOR_RED "Memory leak detected:" COLOR_RESET "\n");
        printf("  Allocated: %zu bytes\n", g_allocated_bytes);
        printf("  Freed:     %zu bytes\n", g_freed_bytes);
        printf("  Leaked:    %zu bytes\n", g_allocated_bytes - g_freed_bytes);
        return true;
    }
    
    return false;
}

// =============================================================================
// MOCKING SUPPORT
// =============================================================================

void mock_register(const char *name, MockFunc original, MockFunc mock) {
    if (g_mock_count >= MAX_MOCKS) {
        fprintf(stderr, COLOR_RED "Error: Maximum mock count exceeded" COLOR_RESET "\n");
        return;
    }
    
    MockEntry *entry = &g_mocks[g_mock_count++];
    entry->name = name;
    entry->original = original;
    entry->mock = mock;
    entry->active = true;
}

void mock_reset(const char *name) {
    for (uint32_t i = 0; i < g_mock_count; i++) {
        if (strcmp(g_mocks[i].name, name) == 0) {
            g_mocks[i].active = false;
            return;
        }
    }
}

void mock_reset_all(void) {
    for (uint32_t i = 0; i < g_mock_count; i++) {
        g_mocks[i].active = false;
    }
    g_mock_count = 0;
}
