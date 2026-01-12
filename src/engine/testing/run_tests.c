// Test Runner
// Main entry point for running all tests

#include "test_framework.h"
#include "core/memory_utils.h"
#include "core/logging_utils.h"

// External test suite declarations
extern TestSuite CoreSystems;
extern TestSuite RenderingTests;
extern TestSuite PhysicsTests;
extern TestSuite AudioTests;
extern TestSuite NetworkTests;
extern TestSuite AnimationTests;
extern TestSuite AssetTests;

int main(int argc, char *argv[]) {
    // Parse command line arguments
    bool run_all = true;
    const char *suite_name = NULL;
    const char *test_name = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
            suite_name = argv[++i];
            run_all = false;
        } else if (strcmp(argv[i], "--test") == 0 && i + 2 < argc) {
            suite_name = argv[++i];
            test_name = argv[++i];
            run_all = false;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --suite <name>    Run specific test suite\n");
            printf("  --test <suite> <test>  Run specific test\n");
            printf("  --help             Show this help\n");
            return 0;
        }
    }
    
    // Initialize systems
    memory_utils_init();
    logging_utils_init();
    test_framework_init();
    
    // Register all test suites
    test_framework_register_suite(&CoreSystems);
    test_framework_register_suite(&RenderingTests);
    test_framework_register_suite(&PhysicsTests);
    test_framework_register_suite(&AudioTests);
    test_framework_register_suite(&NetworkTests);
    test_framework_register_suite(&AnimationTests);
    test_framework_register_suite(&AssetTests);
    
    // Run tests
    TestStats stats = {0};
    
    if (test_name) {
        // Run specific test
        TestResult result = test_framework_run_test(suite_name, test_name);
        stats.total_tests = 1;
        stats.passed_tests = (result == TEST_RESULT_PASS) ? 1 : 0;
        stats.failed_tests = (result == TEST_RESULT_FAIL) ? 1 : 0;
        stats.skipped_tests = (result == TEST_RESULT_SKIP) ? 1 : 0;
        stats.error_tests = (result == TEST_RESULT_ERROR) ? 1 : 0;
        stats.all_passed = (result == TEST_RESULT_PASS);
    } else if (suite_name) {
        // Run specific suite
        stats = test_framework_run_suite(suite_name);
    } else {
        // Run all tests
        stats = test_framework_run_all_tests();
    }
    
    // Cleanup
    test_framework_shutdown();
    logging_utils_shutdown();
    memory_utils_shutdown();
    
    // Print final summary
    if (stats.all_passed) {
        printf("\n🎉 All tests passed! (%u/%u)\n", stats.passed_tests, stats.total_tests);
        return 0;
    } else {
        printf("\n❌ Tests failed! (%u passed, %u failed, %u skipped, %u errors)\n", 
               stats.passed_tests, stats.failed_tests, stats.skipped_tests, stats.error_tests);
        return 1;
    }
}
