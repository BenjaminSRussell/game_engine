/**
 * @file physics_unit_tests.h
 * @brief Physics unit tests
 *
 * Comprehensive unit test suite for physics systems including
 * collision detection, constraint solving, and performance validation.
 */

#ifndef PHYSICS_UNIT_TESTS_H
#define PHYSICS_UNIT_TESTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Test Categories
// ========================================

typedef enum TestCategory {
    TEST_CATEGORY_CORE,               // Core physics tests
    TEST_CATEGORY_COLLISION,          // Collision detection tests
    TEST_CATEGORY_SOLVER,             // Constraint solver tests
    TEST_CATEGORY_DYNAMICS,           // Dynamics integration tests
    TEST_CATEGORY_PERFORMANCE,        // Performance tests
    TEST_CATEGORY_STRESS,             // Stress tests
    TEST_CATEGORY_REGRESSION,         // Regression tests
    TEST_CATEGORY_INTEGRATION,        // Integration tests
    TEST_CATEGORY_ALL                 // All categories
} TestCategory;

// ========================================
// Test Result
// ========================================

typedef enum TestResult {
    TEST_RESULT_PASS,                 // Test passed
    TEST_RESULT_FAIL,                 // Test failed
    TEST_RESULT_SKIP,                 // Test skipped
    TEST_RESULT_ERROR,                // Test error
    TEST_RESULT_TIMEOUT,              // Test timeout
    TEST_RESULT_CRASH                 // Test crash
} TestResult;

// ========================================
// Test Status
// ========================================

typedef enum TestStatus {
    TEST_STATUS_PENDING,              // Test pending
    TEST_STATUS_RUNNING,              // Test running
    TEST_STATUS_COMPLETED,            // Test completed
    TEST_STATUS_CANCELLED,            // Test cancelled
    TEST_STATUS_ABORTED               // Test aborted
} TestStatus;

// ========================================
// Test Configuration
// ========================================

typedef struct TestConfig {
    bool verbose_output;              // Verbose output
    bool stop_on_failure;             // Stop on first failure
    bool enable_profiling;            // Enable profiling
    bool enable_memory_tracking;       // Enable memory tracking
    bool enable_performance_tests;     // Enable performance tests
    bool enable_stress_tests;         // Enable stress tests
    float timeout_seconds;             // Test timeout
    int max_iterations;               // Maximum iterations
    float tolerance;                  // Numerical tolerance
    char output_file[256];            // Output file path
    char log_file[256];               // Log file path
} TestConfig;

// ========================================
// Test Statistics
// ========================================

typedef struct TestStatistics {
    int total_tests;                  // Total tests
    int passed_tests;                 // Passed tests
    int failed_tests;                 // Failed tests
    int skipped_tests;                // Skipped tests
    int error_tests;                  // Error tests
    int timeout_tests;                // Timeout tests
    float total_time_seconds;         // Total time
    float average_time_seconds;        // Average time
    float max_time_seconds;           // Maximum time
    float min_time_seconds;           // Minimum time
    size_t total_memory_usage;        // Total memory usage
    size_t peak_memory_usage;          // Peak memory usage
    uint64_t total_allocations;       // Total allocations
    uint64_t total_deallocations;     // Total deallocations
} TestStatistics;

// ========================================
// Test Case
// ========================================

typedef struct TestCase {
    char name[128];                  // Test name
    char description[256];            // Test description
    TestCategory category;            // Test category
    TestResult result;                // Test result
    TestStatus status;                // Test status
    float execution_time_seconds;      // Execution time
    char error_message[512];          // Error message
    char assertion_message[512];      // Assertion message
    int assertion_line;               // Assertion line
    char assertion_file[256];         // Assertion file
    uint64_t start_time;              // Start time
    uint64_t end_time;                // End time
    bool (*test_function)(void);       // Test function
    void *test_data;                  // Test data
    bool enabled;                     // Test enabled
    int priority;                     // Test priority
} TestCase;

// ========================================
// Test Suite
// ========================================

typedef struct TestSuite {
    char name[128];                  // Suite name
    char description[256];            // Suite description
    TestCase *tests;                  // Test array
    int test_count;                   // Number of tests
    int test_capacity;                // Test capacity
    TestStatistics statistics;        // Statistics
    bool enabled;                     // Suite enabled
    TestConfig config;                // Configuration
    void *user_data;                 // User data
} TestSuite;

// ========================================
// Test Runner
// ========================================

typedef struct TestRunner {
    TestSuite *suites;                // Suite array
    int suite_count;                  // Number of suites
    int suite_capacity;               // Suite capacity
    TestStatistics global_stats;      // Global statistics
    TestConfig config;                // Global configuration
    bool running;                     // Currently running
    uint64_t start_time;              // Start time
    uint64_t end_time;                // End time
    char output_buffer[4096];          // Output buffer
    int output_buffer_size;           // Output buffer size
    void *user_data;                 // User data
} TestRunner;

// ========================================
// Assertion Macros
// ========================================

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            test_fail(__FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            test_fail_eq(__FILE__, __LINE__, #expected, #actual, (expected), (actual)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            test_fail_ne(__FILE__, __LINE__, #expected, #actual, (expected), (actual)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            test_fail_lt(__FILE__, __LINE__, #a, #b, (a), (b)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_LE(a, b) \
    do { \
        if (!((a) <= (b))) { \
            test_fail_le(__FILE__, __LINE__, #a, #b, (a), (b)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            test_fail_gt(__FILE__, __LINE__, #a, #b, (a), (b)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            test_fail_ge(__FILE__, __LINE__, #a, #b, (a), (b)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(expected, actual, tolerance) \
    do { \
        float diff = (expected) - (actual); \
        if (diff < 0) diff = -diff; \
        if (diff > (tolerance)) { \
            test_fail_float_eq(__FILE__, __LINE__, #expected, #actual, (expected), (actual), (tolerance)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            test_fail_null(__FILE__, __LINE__, #ptr); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            test_fail_not_null(__FILE__, __LINE__, #ptr); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            test_fail_true(__FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            test_fail_false(__FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create test runner
 * @param max_suites Maximum number of suites
 * @return Test runner or NULL on failure
 */
TestRunner* test_runner_create(int max_suites);

/**
 * Destroy test runner
 * @param runner Test runner
 */
void test_runner_destroy(TestRunner *runner);

// ========================================
// Suite Management
// ========================================

/**
 * Create test suite
 * @param runner Test runner
 * @param name Suite name
 * @param description Suite description
 * @return Suite pointer or NULL on failure
 */
TestSuite* test_runner_create_suite(TestRunner *runner, const char *name, const char *description);

/**
 * Add test to suite
 * @param suite Test suite
 * @param name Test name
 * @param description Test description
 * @param category Test category
 * @param test_function Test function
 * @return True if successful
 */
bool test_suite_add_test(TestSuite *suite, const char *name, const char *description,
                         TestCategory category, bool (*test_function)(void));

/**
 * Remove test from suite
 * @param suite Test suite
 * @param name Test name
 * @return True if successful
 */
bool test_suite_remove_test(TestSuite *suite, const char *name);

/**
 * Get test by name
 * @param suite Test suite
 * @param name Test name
 * @return Test pointer or NULL if not found
 */
TestCase* test_suite_get_test(TestSuite *suite, const char *name);

// ========================================
// Test Execution
// ========================================

/**
 * Run all tests
 * @param runner Test runner
 * @return True if all tests passed
 */
bool test_runner_run_all(TestRunner *runner);

/**
 * Run test suite
 * @param runner Test runner
 * @param suite_name Suite name
 * @return True if all tests passed
 */
bool test_runner_run_suite(TestRunner *runner, const char *suite_name);

/**
 * Run test by name
 * @param runner Test runner
 * @param suite_name Suite name
 * @param test_name Test name
 * @return True if test passed
 */
bool test_runner_run_test(TestRunner *runner, const char *suite_name, const char *test_name);

/**
 * Run tests by category
 * @param runner Test runner
 * @param category Test category
 * @return True if all tests passed
 */
bool test_runner_run_category(TestRunner *runner, TestCategory category);

// ========================================
// Configuration
// ========================================

/**
 * Set test configuration
 * @param runner Test runner
 * @param config Test configuration
 */
void test_runner_set_config(TestRunner *runner, const TestConfig *config);

/**
 * Get test configuration
 * @param runner Test runner
 * @return Test configuration
 */
TestConfig test_runner_get_config(TestRunner *runner);

/**
 * Enable/disable verbose output
 * @param runner Test runner
 * @param enabled Enable verbose output
 */
void test_runner_set_verbose(TestRunner *runner, bool enabled);

// ========================================
// Statistics
// ========================================

/**
 * Get global statistics
 * @param runner Test runner
 * @return Global statistics
 */
TestStatistics test_runner_get_statistics(TestRunner *runner);

/**
 * Get suite statistics
 * @param suite Test suite
 * @return Suite statistics
 */
TestStatistics test_suite_get_statistics(TestSuite *suite);

/**
 * Reset statistics
 * @param runner Test runner
 */
void test_runner_reset_statistics(TestRunner *runner);

// ========================================
// Output
// ========================================

/**
 * Generate test report
 * @param runner Test runner
 * @param format Output format (text, xml, json)
 * @param filename Output filename
 * @return True if successful
 */
bool test_runner_generate_report(TestRunner *runner, const char *format, const char *filename);

/**
 * Print test results
 * @param runner Test runner
 */
void test_runner_print_results(TestRunner *runner);

/**
 * Print suite results
 * @param suite Test suite
 */
void test_suite_print_results(TestSuite *suite);

// ========================================
// Built-in Tests
// ========================================

/**
 * Register core physics tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_core_tests(TestRunner *runner);

/**
 * Register collision detection tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_collision_tests(TestRunner *runner);

/**
 * Register constraint solver tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_solver_tests(TestRunner *runner);

/**
 * Register dynamics integration tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_dynamics_tests(TestRunner *runner);

/**
 * Register performance tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_performance_tests(TestRunner *runner);

/**
 * Register stress tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_stress_tests(TestRunner *runner);

/**
 * Register regression tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_regression_tests(TestRunner *runner);

/**
 * Register all physics tests
 * @param runner Test runner
 * @return True if successful
 */
bool physics_register_all_tests(TestRunner *runner);

// ========================================
// Test Utilities
// ========================================

/**
 * Initialize test environment
 * @return True if successful
 */
bool physics_test_initialize(void);

/**
 * Cleanup test environment
 */
void physics_test_cleanup(void);

/**
 * Create test physics world
 * @return Physics world or NULL on failure
 */
void* physics_test_create_world(void);

/**
 * Destroy test physics world
 * @param world Physics world
 */
void physics_test_destroy_world(void *world);

/**
 * Create test objects
 * @param world Physics world
 * @param count Number of objects
 * @return Object array or NULL on failure
 */
void* physics_test_create_objects(void *world, int count);

/**
 * Create test collision shapes
 * @param count Number of shapes
 * @return Shape array or NULL on failure
 */
void* physics_test_create_shapes(int count);

/**
 * Create test constraints
 * @param world Physics world
 * @param count Number of constraints
 * @return Constraint array or NULL on failure
 */
void* physics_test_create_constraints(void *world, int count);

/**
 * Generate random positions
 * @param positions Output positions
 * @param count Number of positions
 * @param bounds World bounds
 */
void physics_test_generate_positions(float *positions, int count, const float *bounds);

/**
 * Generate random velocities
 * @param velocities Output velocities
 * @param count Number of velocities
 * @param max_speed Maximum speed
 */
void physics_test_generate_velocities(float *velocities, int count, float max_speed);

/**
 * Measure performance
 * @param function Function to measure
 * @param iterations Number of iterations
 * @param time_ms Output time in milliseconds
 * @return True if successful
 */
bool physics_test_measure_performance(bool (*function)(void), int iterations, float *time_ms);

/**
 * Validate physics state
 * @param world Physics world
 * @return True if state is valid
 */
bool physics_test_validate_state(void *world);

// ========================================
// Assertion Functions (Internal)
// ========================================

/**
 * Report test failure
 * @param file Source file
 * @param line Source line
 * @param condition Failed condition
 */
void test_fail(const char *file, int line, const char *condition);

/**
 * Report equality failure
 * @param file Source file
 * @param line Source line
 * @param expected Expected value
 * @param actual Actual value
 */
void test_fail_eq(const char *file, int line, const char *expected, const char *actual,
                 long long expected_val, long long actual_val);

/**
 * Report inequality failure
 * @param file Source file
 * @param line Source line
 * @param expected Expected value
 * @param actual Actual value
 */
void test_fail_ne(const char *file, int line, const char *expected, const char *actual,
                 long long expected_val, long long actual_val);

/**
 * Report less than failure
 * @param file Source file
 * @param line Source line
 * @param a First value
 * @param b Second value
 */
void test_fail_lt(const char *file, int line, const char *a, const char *b,
                 long long a_val, long long b_val);

/**
 * Report less than or equal failure
 * @param file Source file
 * @param line Source line
 * @param a First value
 * @param b Second value
 */
void test_fail_le(const char *file, int line, const char *a, const char *b,
                 long long a_val, long long b_val);

/**
 * Report greater than failure
 * @param file Source file
 * @param line Source line
 * @param a First value
 * @param b Second value
 */
void test_fail_gt(const char *file, int line, const char *a, const char *b,
                 long long a_val, long long b_val);

/**
 * Report greater than or equal failure
 * @param file Source file
 * @param line Source line
 * @param a First value
 * @param b Second value
 */
void test_fail_ge(const char *file, int line, const char *a, const char *b,
                 long long a_val, long long b_val);

/**
 * Report float equality failure
 * @param file Source file
 * @param line Source line
 * @param expected Expected value
 * @param actual Actual value
 * @param tolerance Tolerance
 */
void test_fail_float_eq(const char *file, int line, const char *expected, const char *actual,
                       float expected_val, float actual_val, float tolerance);

/**
 * Report null failure
 * @param file Source file
 * @param line Source line
 * @param ptr Pointer
 */
void test_fail_null(const char *file, int line, const char *ptr);

/**
 * Report not null failure
 * @param file Source file
 * @param line Source line
 * @param ptr Pointer
 */
void test_fail_not_null(const char *file, int line, const char *ptr);

/**
 * Report true failure
 * @param file Source file
 * @param line Source line
 * @param condition Condition
 */
void test_fail_true(const char *file, int line, const char *condition);

/**
 * Report false failure
 * @param file Source file
 * @param line Source line
 * @param condition Condition
 */
void test_fail_false(const char *file, int line, const char *condition);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_UNIT_TESTS_H */
