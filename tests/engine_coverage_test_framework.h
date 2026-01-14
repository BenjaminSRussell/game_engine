#ifndef ENGINE_COVERAGE_TEST_FRAMEWORK_H
#define ENGINE_COVERAGE_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

// Coverage tracking structure
typedef struct {
    char function_name[256];
    char file_name[256];
    uint32_t line_start;
    uint32_t line_end;
    uint32_t lines_executed;
    uint32_t total_lines;
    float coverage_percentage;
    bool is_covered;
} FunctionCoverage;

typedef struct {
    char file_name[256];
    FunctionCoverage *functions;
    uint32_t function_count;
    uint32_t total_functions_covered;
    float file_coverage_percentage;
    uint32_t total_lines;
    uint32_t lines_executed;
} FileCoverage;

typedef struct {
    FileCoverage *files;
    uint32_t file_count;
    float total_coverage_percentage;
    uint32_t total_functions;
    uint32_t covered_functions;
    uint32_t total_lines;
    uint32_t lines_executed;
    time_t analysis_time;
} CoverageReport;

// Test result structure
typedef struct {
    char test_name[256];
    char suite_name[128];
    bool passed;
    double execution_time_ms;
    char error_message[512];
    uint32_t assertions_run;
    uint32_t assertions_passed;
} TestResult;

typedef struct {
    char suite_name[128];
    TestResult *tests;
    uint32_t test_count;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t tests_skipped;
    double total_execution_time_ms;
    float suite_coverage_percentage;
} TestSuite;

// Engine system test configuration
typedef struct {
    bool enable_memory_testing;
    bool enable_performance_testing;
    bool enable_thread_safety_testing;
    bool enable_integration_testing;
    bool enable_stress_testing;
    bool enable_coverage_analysis;
    float minimum_coverage_threshold;
    uint32_t stress_test_entities;
    uint32_t stress_test_duration_seconds;
    char coverage_output_directory[256];
    bool verbose_output;
} EngineTestConfig;

// Main test framework functions
bool engine_test_framework_init(const EngineTestConfig *config);
void engine_test_framework_shutdown(void);
bool engine_run_all_tests(void);
bool engine_run_test_suite(const char *suite_name);
CoverageReport engine_generate_coverage_report(void);
bool engine_export_coverage_report(const char *filename, const char *format);

// Individual test suite functions
TestSuite engine_run_memory_tests(void);
TestSuite engine_run_audio_tests(void);
TestSuite engine_run_physics_tests(void);
TestSuite engine_run_rendering_tests(void);
TestSuite engine_run_ai_tests(void);
TestSuite engine_run_animation_tests(void);
TestSuite engine_run_input_tests(void);
TestSuite engine_run_network_tests(void);
TestSuite engine_run_filesystem_tests(void);
TestSuite engine_run_math_tests(void);

// Stress testing functions
TestSuite engine_run_entity_stress_tests(void);
TestSuite engine_run_memory_stress_tests(void);
TestSuite engine_run_physics_stress_tests(void);
TestSuite engine_run_rendering_stress_tests(void);

// Integration testing functions
TestSuite engine_run_audio_physics_integration_tests(void);
TestSuite engine_run_rendering_physics_integration_tests(void);
TestSuite engine_run_ai_animation_integration_tests(void);
TestSuite engine_run_input_audio_integration_tests(void);

// Coverage analysis functions
void coverage_mark_function_executed(const char *function_name, const char *file_name);
void coverage_mark_line_executed(const char *file_name, uint32_t line_number);
void coverage_reset(void);
CoverageReport coverage_analyze_source_files(const char *source_directory);

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        g_current_test->assertions_run++; \
        if (!(condition)) { \
            snprintf(g_current_test->error_message, sizeof(g_current_test->error_message), \
                    "Assertion failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
            return false; \
        } \
        g_current_test->assertions_passed++; \
    } while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) \
    do { \
        g_current_test->assertions_run++; \
        if ((expected) != (actual)) { \
            snprintf(g_current_test->error_message, sizeof(g_current_test->error_message), \
                    "Expected %lld, got %lld at %s:%d", (long long)(expected), (long long)(actual), __FILE__, __LINE__); \
            return false; \
        } \
        g_current_test->assertions_passed++; \
    } while(0)

#define ASSERT_NEQ(expected, actual) \
    do { \
        g_current_test->assertions_run++; \
        if ((expected) == (actual)) { \
            snprintf(g_current_test->error_message, sizeof(g_current_test->error_message), \
                    "Expected values to be different at %s:%d", __FILE__, __LINE__); \
            return false; \
        } \
        g_current_test->assertions_passed++; \
    } while(0)

#define ASSERT_FLOAT_EQ(expected, actual, tolerance) \
    do { \
        g_current_test->assertions_run++; \
        if (fabsf((expected) - (actual)) > (tolerance)) { \
            snprintf(g_current_test->error_message, sizeof(g_current_test->error_message), \
                    "Expected %f, got %f (tolerance %f) at %s:%d", (expected), (actual), (tolerance), __FILE__, __LINE__); \
            return false; \
        } \
        g_current_test->assertions_passed++; \
    } while(0)

#define ASSERT_NULL(ptr) ASSERT_EQ(NULL, ptr)
#define ASSERT_NOT_NULL(ptr) ASSERT_NEQ(NULL, ptr)

// Performance testing macros
#define MEASURE_PERFORMANCE_START() \
    uint64_t perf_start_time = get_time_ns()

#define MEASURE_PERFORMANCE_END(result_ns) \
    do { \
        uint64_t perf_end_time = get_time_ns(); \
        *(result_ns) = perf_end_time - perf_start_time; \
    } while(0)

// Memory testing macros
#define ASSERT_NO_MEMORY_LEAKS() \
    do { \
        uint32_t leak_count; \
        if (memory_leak_detector_check_leaks(&leak_count)) { \
            snprintf(g_current_test->error_message, sizeof(g_current_test->error_message), \
                    "Memory leaks detected: %u at %s:%d", leak_count, __FILE__, __LINE__); \
            return false; \
        } \
    } while(0)

// Global variables (extern declarations)
extern TestResult *g_current_test;
extern EngineTestConfig g_test_config;
extern CoverageReport g_coverage_report;

// Utility functions
uint64_t get_time_ns(void);
double get_time_ms(void);
void log_test_message(const char *level, const char *suite, const char *format, ...);
bool memory_leak_detector_init(void);
bool memory_leak_detector_check_leaks(uint32_t *leak_count);
void memory_leak_detector_shutdown(void);

// Test suite registration
typedef bool (*TestFunction)(void);
typedef struct {
    const char *name;
    TestFunction function;
    const char *description;
} TestCase;

void register_test_case(const char *suite_name, const char *test_name, TestFunction function, const char *description);

#endif // ENGINE_COVERAGE_TEST_FRAMEWORK_H
