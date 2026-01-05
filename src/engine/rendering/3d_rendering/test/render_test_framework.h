/**
 * @file render_test_framework.h
 * @brief Unit testing framework for the 3D rendering pipeline
 * @details Provides testing utilities, assertions, and test harness for rendering tests
 */

#ifndef RENDER_TEST_FRAMEWORK_H
#define RENDER_TEST_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* ==================== Test Framework Configuration ==================== */

#define RENDER_TEST_MAX_TESTS 1000
#define RENDER_TEST_MAX_ASSERTIONS 10000
#define RENDER_TEST_MAX_TEST_SUITES 100

/* ==================== Test Result Tracking ==================== */

typedef enum {
    TEST_STATUS_PENDING = 0,
    TEST_STATUS_PASSED = 1,
    TEST_STATUS_FAILED = 2,
    TEST_STATUS_SKIPPED = 3,
    TEST_STATUS_ERROR = 4,
} TestStatus;

typedef struct {
    const char* test_name;
    const char* test_suite;
    TestStatus status;
    const char* failure_message;
    uint32_t assertion_count;
    double execution_time_ms;
} TestResult;

typedef struct {
    const char* suite_name;
    TestResult* results;
    uint32_t test_count;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t skipped_count;
    double total_time_ms;
} TestSuiteResult;

typedef struct {
    TestSuiteResult* suites;
    uint32_t suite_count;
    uint32_t total_tests;
    uint32_t total_passed;
    uint32_t total_failed;
    uint32_t total_skipped;
    double total_time_ms;
} TestRunResult;

/* ==================== Test Function Types ==================== */

typedef bool (*TestFunction)(void);
typedef bool (*TestSetupFunction)(void);
typedef bool (*TestTeardownFunction)(void);

/* ==================== Test Context ==================== */

typedef struct {
    const char* current_test_name;
    const char* current_suite_name;
    TestStatus current_status;
    char error_buffer[2048];
    uint32_t assertion_count;
} TestContext;

/**
 * @brief Initialize the test framework
 * @return true on success
 */
bool render_test_framework_init(void);

/**
 * @brief Shutdown the test framework
 */
void render_test_framework_shutdown(void);

/**
 * @brief Create a new test suite
 * @param[in] suite_name Name of the test suite
 * @return Suite ID, 0 on failure
 */
uint32_t render_test_suite_create(const char* suite_name);

/**
 * @brief Register a test function
 * @param[in] suite_id Suite ID
 * @param[in] test_name Name of the test
 * @param[in] test_func Test function pointer
 * @return true on success
 */
bool render_test_register(uint32_t suite_id,
                         const char* test_name,
                         TestFunction test_func);

/**
 * @brief Register setup/teardown functions for a suite
 * @param[in] suite_id Suite ID
 * @param[in] setup Setup function (called before each test)
 * @param[in] teardown Teardown function (called after each test)
 * @return true on success
 */
bool render_test_suite_set_fixtures(uint32_t suite_id,
                                    TestSetupFunction setup,
                                    TestTeardownFunction teardown);

/**
 * @brief Run all registered tests
 * @return Pointer to test results, NULL on failure
 */
TestRunResult* render_test_run_all(void);

/**
 * @brief Run tests for a specific suite
 * @param[in] suite_id Suite ID
 * @return Pointer to suite results, NULL on failure
 */
TestSuiteResult* render_test_suite_run(uint32_t suite_id);

/**
 * @brief Skip current test
 * @param[in] reason Reason for skipping
 */
void render_test_skip(const char* reason);

/**
 * @brief Get current test context
 * @return Pointer to current test context
 */
TestContext* render_test_get_context(void);

/**
 * @brief Print test results in human-readable format
 * @param[in] results Test results to print
 */
void render_test_print_results(const TestRunResult* results);

/**
 * @brief Generate JSON report of test results
 * @param[in] results Test results
 * @param[in] filename File to write JSON to
 * @return true on success
 */
bool render_test_generate_json_report(const TestRunResult* results, const char* filename);

/**
 * @brief Free test results
 * @param[in] results Results to free
 */
void render_test_free_results(TestRunResult* results);

/* ==================== Assertion Macros ==================== */

/**
 * @brief Assert that condition is true
 */
#define RENDER_TEST_ASSERT(condition, message) \
    do { \
        TestContext* ctx = render_test_get_context(); \
        ctx->assertion_count++; \
        if (!(condition)) { \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Assertion failed: %s (file: %s, line: %d)", (message), __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
    } while(0)

/**
 * @brief Assert that two integers are equal
 */
#define RENDER_TEST_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            TestContext* ctx = render_test_get_context(); \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Expected %lld, got %lld (file: %s, line: %d)", \
                    (long long)(expected), (long long)(actual), __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
        render_test_get_context()->assertion_count++; \
    } while(0)

/**
 * @brief Assert that two integers are not equal
 */
#define RENDER_TEST_ASSERT_NE(actual, unexpected) \
    do { \
        if ((actual) == (unexpected)) { \
            TestContext* ctx = render_test_get_context(); \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Expected != %lld, got %lld (file: %s, line: %d)", \
                    (long long)(unexpected), (long long)(actual), __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
        render_test_get_context()->assertion_count++; \
    } while(0)

/**
 * @brief Assert that pointer is not NULL
 */
#define RENDER_TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            TestContext* ctx = render_test_get_context(); \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Expected non-NULL pointer (file: %s, line: %d)", __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
        render_test_get_context()->assertion_count++; \
    } while(0)

/**
 * @brief Assert that pointer is NULL
 */
#define RENDER_TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            TestContext* ctx = render_test_get_context(); \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Expected NULL pointer (file: %s, line: %d)", __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
        render_test_get_context()->assertion_count++; \
    } while(0)

/**
 * @brief Assert that float values are approximately equal
 */
#define RENDER_TEST_ASSERT_FLOAT_EQ(actual, expected, epsilon) \
    do { \
        float diff = (float)((actual) - (expected)); \
        if (diff < 0) diff = -diff; \
        if (diff > (epsilon)) { \
            TestContext* ctx = render_test_get_context(); \
            snprintf(ctx->error_buffer, sizeof(ctx->error_buffer), \
                    "Expected %f +/- %f, got %f (file: %s, line: %d)", \
                    (float)(expected), (float)(epsilon), (float)(actual), __FILE__, __LINE__); \
            ctx->current_status = TEST_STATUS_FAILED; \
            return false; \
        } \
        render_test_get_context()->assertion_count++; \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* RENDER_TEST_FRAMEWORK_H */
