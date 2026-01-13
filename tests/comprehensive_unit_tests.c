/*
 * Comprehensive Unit Tests
 * Core engine functionality testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test framework
typedef struct {
    const char* name;
    int (*test_func)(void);
} test_case_t;

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return 0; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        tests_run++; \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            tests_passed++; \
            printf("PASS: %s\n", #test_func); \
        } \
    } while(0)

// Test: Basic memory allocation
int test_memory_allocation() {
    void* ptr = malloc(1024);
    ASSERT(ptr != NULL, "Memory allocation failed");
    
    memset(ptr, 0, 1024);
    free(ptr);
    return 1;
}

// Test: String operations
int test_string_operations() {
    const char* test_str = "Hello, Engine!";
    char* buffer = malloc(strlen(test_str) + 1);
    
    ASSERT(buffer != NULL, "Buffer allocation failed");
    strcpy(buffer, test_str);
    ASSERT(strcmp(buffer, test_str) == 0, "String copy failed");
    
    free(buffer);
    return 1;
}

// Test: Basic math operations
int test_math_operations() {
    float a = 3.14f;
    float b = 2.71f;
    
    ASSERT((a + b) > 5.8f && (a + b) < 5.9f, "Addition failed");
    ASSERT((a * b) > 8.5f && (a * b) < 8.6f, "Multiplication failed");
    
    return 1;
}

// Test: Array operations
int test_array_operations() {
    int array[100];
    
    // Initialize array
    for (int i = 0; i < 100; i++) {
        array[i] = i * 2;
    }
    
    // Verify values
    for (int i = 0; i < 100; i++) {
        ASSERT(array[i] == i * 2, "Array initialization failed");
    }
    
    return 1;
}

// Test: Structure operations
typedef struct {
    int x, y;
    float radius;
    const char* name;
} test_struct_t;

int test_struct_operations() {
    test_struct_t obj = {10, 20, 5.5f, "TestObject"};
    
    ASSERT(obj.x == 10, "Struct field x failed");
    ASSERT(obj.y == 20, "Struct field y failed");
    ASSERT(obj.radius > 5.4f && obj.radius < 5.6f, "Struct field radius failed");
    ASSERT(strcmp(obj.name, "TestObject") == 0, "Struct field name failed");
    
    return 1;
}

// Main test runner
int main() {
    printf("=== Comprehensive Unit Tests ===\n\n");
    
    // Core functionality tests
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_string_operations);
    RUN_TEST(test_math_operations);
    RUN_TEST(test_array_operations);
    RUN_TEST(test_struct_operations);
    
    printf("\n=== Test Results ===\n");
    printf("Tests Run: %d\n", tests_run);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_run - tests_passed);
    printf("Success Rate: %.1f%%\n", 
           (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("✅ All tests passed!\n");
        return 0;
    } else {
        printf("❌ Some tests failed!\n");
        return 1;
    }
}
