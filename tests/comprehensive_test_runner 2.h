/**
 * @file comprehensive_test_runner.h
 * @brief Comprehensive test runner with full documentation
 *
 * Unified test runner that orchestrates all test suites and provides
 * complete documentation for all functions and parameters.
 */

#ifndef COMPREHENSIVE_TEST_RUNNER_H
#define COMPREHENSIVE_TEST_RUNNER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Include all test headers
#include "memory_leak_detector.h"
#include "thread_safety_verifier.h"
#include "math_unit_tests.h"
#include "physics_integration_tests_extended.h"
#include "rendering_tests.h"
#include "ai_behavior_tree_tests.h"
#include "entity_stress_test.h"
#include "memory_allocation_test.h"
#include "physics_stability_test.h"

// New comprehensive test suites
#include "test_vulkan_backend.h"
#include "test_audio_system.h"
#include "test_asset_management.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Test Suite Enumeration
// ========================================

typedef enum TestSuite {
    TEST_SUITE_MEMORY_LEAK_DETECTION,   // Memory leak detection tests
    TEST_SUITE_THREAD_SAFETY,          // Thread safety verification tests
    TEST_SUITE_MATH_UNIT_TESTS,         // Mathematics unit tests
    TEST_SUITE_PHYSICS_INTEGRATION,     // Physics integration tests
    TEST_SUITE_RENDERING,               // Rendering tests
    TEST_SUITE_AI_BEHAVIOR_TREES,      // AI behavior tree tests
    TEST_SUITE_ENTITY_STRESS,           // Entity stress tests
    TEST_SUITE_MEMORY_ALLOCATION,        // Memory allocation tests
    TEST_SUITE_PHYSICS_STABILITY,       // Physics stability tests
    
    // New comprehensive test suites
    TEST_SUITE_VULKAN_BACKEND,         // Vulkan backend tests
    TEST_SUITE_AUDIO_SYSTEM,            // Audio system tests
    TEST_SUITE_ASSET_MANAGEMENT,       // Asset management tests
    
    TEST_SUITE_ALL,                    // Run all test suites
    TEST_SUITE_COUNT                   // Total number of test suites
} TestSuite;

// ========================================
// Test Runner Configuration
// ========================================

typedef struct ComprehensiveTestConfig {
    bool enable_all_suites;            // Enable all test suites
    bool enable_memory_leak_detection;   // Enable memory leak detection
    bool enable_thread_safety;          // Enable thread safety tests
    bool enable_math_tests;              // Enable math tests
    bool enable_physics_tests;           // Enable physics tests
    bool enable_rendering_tests;         // Enable rendering tests
    bool enable_ai_tests;                // Enable AI tests
    bool enable_stress_tests;            // Enable stress tests
    bool enable_verbose_output;          // Enable verbose output
    bool enable_detailed_logging;        // Enable detailed logging
    bool generate_reports;               // Generate test reports
    bool stop_on_failure;               // Stop on first failure
    const char *output_directory;        // Output directory for reports
    const char *log_file;               // Log file path
    uint32_t timeout_seconds;            // Test timeout in seconds
} ComprehensiveTestConfig;

// ========================================
// Test Suite Results
// ========================================

typedef struct TestSuiteResults {
    TestSuite suite_type;                // Type of test suite
    char suite_name[64];                // Name of test suite
    bool was_executed;                  // Whether suite was executed
    bool passed;                        // Whether suite passed
    uint32_t total_tests;               // Total tests in suite
    uint32_t passed_tests;              // Tests that passed
    uint32_t failed_tests;              // Tests that failed
    uint32_t skipped_tests;             // Tests that were skipped
    double execution_time_ms;            // Suite execution time
    char error_messages[1024];          // Suite error messages
    char performance_summary[512];       // Performance summary
} TestSuiteResults;

// ========================================
// Comprehensive Test Results
// ========================================

typedef struct ComprehensiveTestResults {
    TestSuiteResults suite_results[TEST_SUITE_COUNT]; // Results for each suite
    uint32_t total_suites_run;         // Total suites run
    uint32_t suites_passed;             // Suites that passed
    uint32_t suites_failed;             // Suites that failed
    uint32_t total_tests;               // Total tests across all suites
    uint32_t total_passed;              // Total tests passed
    uint32_t total_failed;              // Total tests failed
    uint32_t total_skipped;             // Total tests skipped
    double total_execution_time_ms;       // Total execution time
    double average_suite_time_ms;         // Average suite execution time
    double max_suite_time_ms;            // Maximum suite execution time
    double min_suite_time_ms;            // Minimum suite execution time
    bool overall_success;                // Overall test success
    char summary_report[4096];          // Summary report
    char detailed_report[8192];         // Detailed report
} ComprehensiveTestResults;

// ========================================
// Test Runner State
// ========================================

typedef struct TestRunnerState {
    ComprehensiveTestConfig config;       // Test configuration
    ComprehensiveTestResults results;      // Test results
    uint64_t start_time;                // Test start time
    uint64_t end_time;                  // Test end time
    bool is_running;                    // Whether tests are running
    bool should_stop;                   // Whether tests should stop
    char current_suite[64];             // Currently running suite
    uint32_t current_test;               // Current test index
} TestRunnerState;

// ========================================
// Global State
// ========================================

extern TestRunnerState g_test_runner_state;

// ========================================
// Main Test Runner Functions
// ========================================

/**
 * Initialize comprehensive test runner
 * @param config Test configuration
 * @return True if initialization successful
 * 
 * Purpose: Initialize the comprehensive test runner with the provided configuration.
 * Sets up all necessary data structures, logging, and output directories.
 * 
 * Parameters:
 * - config: Pointer to test configuration structure containing test settings
 * 
 * Returns: True if initialization was successful, false otherwise
 */
bool comprehensive_test_runner_init(const ComprehensiveTestConfig *config);

/**
 * Shutdown comprehensive test runner
 * @param generate_final_report Whether to generate final comprehensive report
 * 
 * Purpose: Clean up all test runner resources and optionally generate final report.
 * Releases memory, closes files, and performs any necessary cleanup.
 * 
 * Parameters:
 * - generate_final_report: Boolean flag indicating whether to generate comprehensive report
 * 
 * Returns: None
 */
void comprehensive_test_runner_shutdown(bool generate_final_report);

/**
 * Run comprehensive test suite
 * @param suites_to_run Bitmask of suites to run (use TEST_SUITE_ALL for all)
 * @return True if all requested tests passed
 * 
 * Purpose: Execute the requested test suites and collect results.
 * Orchestrates execution of individual test suites and aggregates results.
 * 
 * Parameters:
 * - suites_to_run: Bitmask indicating which test suites to execute
 * 
 * Returns: True if all executed test suites passed, false otherwise
 */
bool comprehensive_test_runner_run(uint32_t suites_to_run);

/**
 * Run specific test suite
 * @param suite_type Test suite to run
 * @return True if test suite passed
 * 
 * Purpose: Execute a single test suite and return its results.
 * Provides fine-grained control over which tests to execute.
 * 
 * Parameters:
 * - suite_type: Enumeration value indicating which test suite to run
 * 
 * Returns: True if the specified test suite passed, false otherwise
 */
bool comprehensive_test_runner_run_suite(TestSuite suite_type);

/**
 * Get comprehensive test results
 * @return Complete test results structure
 * 
 * Purpose: Retrieve the complete results from all executed test suites.
 * Provides access to detailed results including performance metrics and error messages.
 * 
 * Parameters: None
 * 
 * Returns: ComprehensiveTestResults structure containing all test results
 */
ComprehensiveTestResults comprehensive_test_runner_get_results(void);

/**
 * Print comprehensive test summary
 * 
 * Purpose: Display a formatted summary of all test results to console.
 * Shows overall success status, suite-by-suite results, and performance metrics.
 * 
 * Parameters: None
 * 
 * Returns: None
 */
void comprehensive_test_runner_print_summary(void);

/**
 * Export comprehensive test report
 * @param filename Output filename for the report
 * @param format Report format (text, json, xml, csv)
 * @return True if export successful
 * 
 * Purpose: Generate and export a comprehensive test report to specified file.
 * Creates detailed report including all test results, performance metrics, and analysis.
 * 
 * Parameters:
 * - filename: Path to output file for the report
 * - format: String specifying output format ("text", "json", "xml", "csv")
 * 
 * Returns: True if report was successfully exported, false otherwise
 */
bool comprehensive_test_runner_export_report(const char *filename, const char *format);

// ========================================
// Individual Test Suite Execution Functions
// ========================================

/**
 * Execute memory leak detection tests
 * @return Test suite results
 * 
 * Purpose: Run comprehensive memory leak detection tests.
 * Tests memory allocation tracking, leak detection, and memory usage analysis.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing memory leak test results
 */
TestSuiteResults execute_memory_leak_detection_tests(void);

/**
 * Execute thread safety verification tests
 * @return Test suite results
 * 
 * Purpose: Run thread safety verification tests.
 * Tests race condition detection, deadlock detection, and thread usage analysis.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing thread safety test results
 */
TestSuiteResults execute_thread_safety_tests(void);

/**
 * Execute mathematics unit tests
 * @return Test suite results
 * 
 * Purpose: Run comprehensive mathematics library unit tests.
 * Tests vector operations, quaternion operations, matrix operations, and SIMD optimizations.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing math test results
 */
TestSuiteResults execute_math_unit_tests(void);

/**
 * Execute physics integration tests
 * @return Test suite results
 * 
 * Purpose: Run physics system integration tests.
 * Tests gravity simulation, collision detection, physics stability, and performance.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing physics test results
 */
TestSuiteResults execute_physics_integration_tests(void);

/**
 * Execute rendering tests
 * @return Test suite results
 * 
 * Purpose: Run rendering system tests.
 * Tests framebuffer validation, shader correctness, rendering pipeline, and performance.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing rendering test results
 */
TestSuiteResults execute_rendering_tests(void);

/**
 * Execute AI behavior tree tests
 * @return Test suite results
 * 
 * Purpose: Run AI behavior tree execution tests.
 * Tests node execution, tree traversal, behavior validation, and performance.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing AI test results
 */
TestSuiteResults execute_ai_behavior_tree_tests(void);

/**
 * Execute entity stress tests
 * @return Test suite results
 * 
 * Purpose: Run entity system stress tests.
 * Tests entity creation, component management, simulation performance, and memory usage.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing entity stress test results
 */
TestSuiteResults execute_entity_stress_tests(void);

/**
 * Execute memory allocation tests
 * @return Test suite results
 * 
 * Purpose: Run memory allocation and deallocation stress tests.
 * Tests allocation patterns, memory leaks, fragmentation, and performance.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing memory allocation test results
 */
TestSuiteResults execute_memory_allocation_tests(void);

/**
 * Execute physics stability tests
 * @return Test suite results
 * 
 * Purpose: Run physics stability tests at high frequency.
 * Tests 180 Hz physics simulation, numerical stability, and energy conservation.
 * 
 * Parameters: None
 * 
 * Returns: TestSuiteResults structure containing physics stability test results
 */
TestSuiteResults execute_physics_stability_tests(void);

// ========================================
// Utility and Analysis Functions
// ========================================

/**
 * Get test suite name from enumeration
 * @param suite_type Test suite enumeration value
 * @return String name of test suite
 * 
 * Purpose: Convert test suite enumeration to human-readable name.
 * Used for logging and report generation.
 * 
 * Parameters:
 * - suite_type: Enumeration value of test suite
 * 
 * Returns: String containing the name of the test suite
 */
const char* get_test_suite_name(TestSuite suite_type);

/**
 * Check if test suite should be executed
 * @param suite_type Test suite to check
 * @param suite_mask Bitmask of enabled suites
 * @return True if suite should be executed
 * 
 * Purpose: Determine if a specific test suite is enabled in the execution mask.
 * Used to filter which tests to run based on configuration.
 * 
 * Parameters:
 * - suite_type: Test suite enumeration value to check
 * - suite_mask: Bitmask of enabled test suites
 * 
 * Returns: True if the test suite should be executed, false otherwise
 */
bool should_execute_suite(TestSuite suite_type, uint32_t suite_mask);

/**
 * Calculate test coverage metrics
 * @param results Test results to analyze
 * @param total_coverage Output total coverage percentage
 * @param functional_coverage Output functional coverage percentage
 * 
 * Purpose: Calculate test coverage metrics from test results.
 * Analyzes which areas of the codebase are covered by executed tests.
 * 
 * Parameters:
 * - results: Test results structure to analyze
 * - total_coverage: Output parameter for total coverage percentage
 * - functional_coverage: Output parameter for functional coverage percentage
 * 
 * Returns: None (results returned via output parameters)
 */
void calculate_test_coverage(const ComprehensiveTestResults *results,
                          float *total_coverage,
                          float *functional_coverage);

/**
 * Generate performance analysis report
 * @param results Test results to analyze
 * @param report Output buffer for report
 * @param max_size Maximum size of output buffer
 * @return True if report generated successfully
 * 
 * Purpose: Generate detailed performance analysis from test results.
 * Analyzes execution times, memory usage, and performance bottlenecks.
 * 
 * Parameters:
 * - results: Test results structure to analyze
 * - report: Output buffer for generated report
 * - max_size: Maximum size of output buffer
 * 
 * Returns: True if report was generated successfully, false otherwise
 */
bool generate_performance_analysis(const ComprehensiveTestResults *results,
                                char *report,
                                size_t max_size);

/**
 * Validate test environment
 * @return True if test environment is valid
 * 
 * Purpose: Validate that the test environment is properly configured.
 * Checks dependencies, permissions, and system requirements.
 * 
 * Parameters: None
 * 
 * Returns: True if test environment is valid, false otherwise
 */
bool validate_test_environment(void);

/**
 * Set test timeout callback
 * @param callback Function to call on timeout
 * 
 * Purpose: Set callback function to be called when test timeout occurs.
 * Allows custom timeout handling and cleanup.
 * 
 * Parameters:
 * - callback: Function pointer to timeout callback function
 * 
 * Returns: None
 */
void set_test_timeout_callback(void (*callback)(void));

// ========================================
// Logging and Reporting Functions
// ========================================

/**
 * Log test message
 * @param level Log level (INFO, WARNING, ERROR, DEBUG)
 * @param suite Test suite name
 * @param format Printf-style format string
 * @param ... Variable arguments
 * 
 * Purpose: Log test messages with specified level and context.
 * Provides structured logging for test execution and debugging.
 * 
 * Parameters:
 * - level: String indicating log level ("INFO", "WARNING", "ERROR", "DEBUG")
 * - suite: Name of test suite generating the message
 * - format: Printf-style format string
 * - ...: Variable arguments matching format string
 * 
 * Returns: None
 */
void log_test_message(const char *level, const char *suite, const char *format, ...);

/**
 * Start test timer
 * @param suite_name Name of test suite being timed
 * 
 * Purpose: Start timing execution for a specific test suite.
 * Used to measure performance and execution times.
 * 
 * Parameters:
 * - suite_name: Name of the test suite to time
 * 
 * Returns: None
 */
void start_test_timer(const char *suite_name);

/**
 * Stop test timer and record elapsed time
 * @param suite_name Name of test suite
 * @return Elapsed time in milliseconds
 * 
 * Purpose: Stop timing execution and record elapsed time for test suite.
 * Returns the elapsed time for performance analysis.
 * 
 * Parameters:
 * - suite_name: Name of the test suite
 * 
 * Returns: Elapsed time in milliseconds
 */
double stop_test_timer(const char *suite_name);

/**
 * Generate test suite bitmask
 * @param config Test configuration
 * @return Bitmask of enabled test suites
 * 
 * Purpose: Generate bitmask of enabled test suites from configuration.
 * Used to determine which tests to execute based on config settings.
 * 
 * Parameters:
 * - config: Test configuration structure
 * 
 * Returns: Bitmask with bits set for enabled test suites
 */
uint32_t generate_suite_mask(const ComprehensiveTestConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* COMPREHENSIVE_TEST_RUNNER_H */
