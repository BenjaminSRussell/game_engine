#include "test_framework.h"
#include <stdio.h>

// Forward declarations of test registration functions
void register_memory_tests(void);
void register_ecs_tests(void);
// void register_math_vec3_tests(void); // Example

int main(int argc, char **argv) {
    test_init();
    
    printf("Initializing Test Runner...\n");
    
    // Register all suites
// Register all suites
    register_memory_tests();
    register_ecs_tests();
    // register_math_vec3_tests();
    
    TestStats stats = test_run_all();
    
    test_cleanup();
    
    printf("\nTest Summary:\n");
    printf("  Total:   %d\n", stats.total);
    printf("  Passed:  %d\n", stats.passed);
    printf("  Failed:  %d\n", stats.failed);
    printf("  Skipped: %d\n", stats.skipped);
    
    return (stats.failed > 0) ? 1 : 0;
}
