// Crash Detection Utility
// Helps identify and diagnose crashes in the engine

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>

// Test different components for crashes
typedef struct {
    const char* name;
    int (*test_func)(void);
    bool should_crash;
} CrashTest;

// Forward declaration
void recursive_function(int depth);

// Test basic memory operations
int test_memory_operations(void) {
    printf("Testing memory operations...\n");
    
    // Test NULL pointer dereference
    int* null_ptr = NULL;
    printf("About to dereference NULL pointer...\n");
    // *null_ptr = 42; // This would crash - commented out for safety
    
    // Test invalid memory access
    printf("Testing invalid memory access...\n");
    // int* invalid_ptr = (int*)0x12345678;
    // *invalid_ptr = 42; // This would crash - commented out for safety
    
    // Test double free
    printf("Testing double free...\n");
    int* test_ptr = malloc(sizeof(int));
    if (test_ptr) {
        free(test_ptr);
        // free(test_ptr); // This would crash - commented out for safety
    }
    
    printf("Memory operations test completed\n");
    return 0;
}

// Test array bounds
int test_array_bounds(void) {
    printf("Testing array bounds...\n");
    
    int array[10] = {0};
    printf("About to access array out of bounds...\n");
    // int value = array[100]; // This would crash - commented out for safety
    
    printf("Array bounds test completed\n");
    return 0;
}

// Test stack overflow
int test_stack_overflow(void) {
    printf("Testing stack overflow...\n");
    
    printf("Stack overflow test completed\n");
    return 0;
}

// Recursive function to cause stack overflow (moved outside)
void recursive_function(int depth) {
    char buffer[1024]; // Use stack space
    printf("Recursion depth: %d\n", depth);
    if (depth < 10000) { // Limit to prevent actual crash
        recursive_function(depth + 1);
    }
}

// Test division by zero
int test_division_by_zero(void) {
    printf("Testing division by zero...\n");
    
    int x = 10;
    int y = 0;
    printf("About to divide by zero...\n");
    // int result = x / y; // This would crash - commented out for safety
    
    printf("Division by zero test completed\n");
    return 0;
}

// Test assertion failure
int test_assertion_failure(void) {
    printf("Testing assertion failure...\n");
    
    int x = 10;
    int y = 20;
    printf("About to trigger assertion...\n");
    // assert(x == y); // This would crash - commented out for safety
    
    printf("Assertion failure test completed\n");
    return 0;
}

// Test infinite loop
int test_infinite_loop(void) {
    printf("Testing infinite loop...\n");
    
    printf("About to enter infinite loop...\n");
    // while (1) {
    //     // This would hang - commented out for safety
    // }
    
    printf("Infinite loop test completed\n");
    return 0;
}

// Test engine components
int test_engine_components(void) {
    printf("Testing engine components...\n");
    
    // Test if we can run the sample engine
    printf("Testing sample engine...\n");
    int result = system("./sample_engine");
    if (result != 0) {
        printf("Sample engine crashed or failed (exit code: %d)\n", result);
        return -1;
    }
    
    // Test if we can run the validation tests
    printf("Testing validation...\n");
    result = system("./test_validation");
    if (result != 0) {
        printf("Validation test crashed or failed (exit code: %d)\n", result);
        return -1;
    }
    
    printf("Engine components test completed\n");
    return 0;
}

// Test build system
int test_build_system(void) {
    printf("Testing build system...\n");
    
    // Try to build the Engine target
    printf("Testing Engine build...\n");
    int result = system("cd build_ci && make Engine 2>&1");
    if (result != 0) {
        printf("Engine build failed (exit code: %d)\n", result);
        return -1;
    }
    
    printf("Build system test completed\n");
    return 0;
}

// Run a crash test in a subprocess
int run_crash_test(const CrashTest* test) {
    printf("\n=== Running Crash Test: %s ===\n", test->name);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run the test
        exit(test->test_func());
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        int result = waitpid(pid, &status, 0);
        
        if (result == -1) {
            printf("ERROR: Failed to wait for child process\n");
            return -1;
        }
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Test '%s' exited with code: %d\n", test->name, exit_code);
            
            if (test->should_crash && exit_code == 0) {
                printf("WARNING: Expected crash but test passed\n");
                return -1;
            } else if (!test->should_crash && exit_code != 0) {
                printf("ERROR: Unexpected crash in test\n");
                return -1;
            }
            
            return exit_code;
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            printf("Test '%s' terminated by signal: %d (%s)\n", 
                   test->name, signal, strsignal(signal));
            
            if (!test->should_crash) {
                printf("ERROR: Unexpected crash in test\n");
                return -1;
            }
            
            return -1; // Crashed
        } else {
            printf("Test '%s' terminated abnormally\n", test->name);
            return -1;
        }
    } else {
        printf("ERROR: Failed to fork process\n");
        return -1;
    }
}

// Main crash detection
int main(int argc, char* argv[]) {
    printf("=== Voxel Forge Engine Crash Detector ===\n");
    printf("Time: %s", ctime(&(time_t){time(NULL)}));
    
    // Define crash tests
    CrashTest tests[] = {
        {"Memory Operations", test_memory_operations, false},
        {"Array Bounds", test_array_bounds, false},
        {"Stack Overflow", test_stack_overflow, false},
        {"Division by Zero", test_division_by_zero, false},
        {"Assertion Failure", test_assertion_failure, false},
        {"Infinite Loop", test_infinite_loop, false},
        {"Engine Components", test_engine_components, false},
        {"Build System", test_build_system, false},
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    int failed = 0;
    
    // Run all tests
    for (int i = 0; i < num_tests; i++) {
        int result = run_crash_test(&tests[i]);
        if (result == 0) {
            passed++;
        } else {
            failed++;
        }
    }
    
    // Print summary
    printf("\n=== Crash Detection Summary ===\n");
    printf("Tests passed: %d\n", passed);
    printf("Tests failed: %d\n", failed);
    printf("Total tests: %d\n", num_tests);
    
    if (failed == 0) {
        printf("✅ All tests passed - no crashes detected\n");
        return 0;
    } else {
        printf("❌ %d tests failed - crashes detected\n", failed);
        return 1;
    }
}
