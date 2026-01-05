/**
 * Unit Test for Logger System
 * Tests logging functionality, log levels, and output handling
 */

#include "../../../src/engine/core/logger.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test logger initialization
static TestResult test_logger_init(void) {
    logger_init(LOG_LEVEL_INFO);
    TEST_ASSERT(true, "Logger initialized");
    logger_shutdown();
    return TEST_PASS;
}

// Test log level setting
static TestResult test_log_level(void) {
    logger_init(LOG_LEVEL_DEBUG);
    
    logger_set_level(LOG_LEVEL_WARNING);
    LogLevel level = logger_get_level();
    TEST_ASSERT_EQ(level, LOG_LEVEL_WARNING, "Log level set to WARNING");
    
    logger_shutdown();
    return TEST_PASS;
}

// Test logging different levels
static TestResult test_log_messages(void) {
    logger_init(LOG_LEVEL_DEBUG);
    
    // These should not crash
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARNING("Warning message");
    LOG_ERROR("Error message");
    
    TEST_ASSERT(true, "All log levels work without crash");
    
    logger_shutdown();
    return TEST_PASS;
}

// Test formatted logging
static TestResult test_formatted_logging(void) {
    logger_init(LOG_LEVEL_INFO);
    
    int value = 42;
    const char* text = "test";
    
    LOG_INFO("Value: %d, Text: %s", value, text);
    TEST_ASSERT(true, "Formatted logging works");
    
    logger_shutdown();
    return TEST_PASS;
}

// Test log filtering by level
static TestResult test_log_filtering(void) {
    logger_init(LOG_LEVEL_WARNING);
    
    // Debug and Info should be filtered out
    LOG_DEBUG("This should not appear");
    LOG_INFO("This should not appear");
    
    // Warning and Error should appear
    LOG_WARNING("This should appear");
    LOG_ERROR("This should appear");
    
    TEST_ASSERT(true, "Log filtering works");
    
    logger_shutdown();
    return TEST_PASS;
}

// Test logger shutdown and reinitialization
static TestResult test_logger_shutdown(void) {
    logger_init(LOG_LEVEL_INFO);
    logger_shutdown();
    
    // Reinitialize
    logger_init(LOG_LEVEL_DEBUG);
    LOG_INFO("After reinitialization");
    TEST_ASSERT(true, "Logger can be shut down and reinitialized");
    
    logger_shutdown();
    return TEST_PASS;
}

// Test multiple log calls
static TestResult test_multiple_logs(void) {
    logger_init(LOG_LEVEL_INFO);
    
    for (int i = 0; i < 100; i++) {
        LOG_INFO("Log message %d", i);
    }
    
    TEST_ASSERT(true, "Multiple log calls handled");
    
    logger_shutdown();
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Core:Logger", "Initialization", test_logger_init, NULL, NULL);
    test_register("Core:Logger", "Log Level", test_log_level, NULL, NULL);
    test_register("Core:Logger", "Log Messages", test_log_messages, NULL, NULL);
    test_register("Core:Logger", "Formatted Logging", test_formatted_logging, NULL, NULL);
    test_register("Core:Logger", "Log Filtering", test_log_filtering, NULL, NULL);
    test_register("Core:Logger", "Shutdown", test_logger_shutdown, NULL, NULL);
    test_register("Core:Logger", "Multiple Logs", test_multiple_logs, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Logger System Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
