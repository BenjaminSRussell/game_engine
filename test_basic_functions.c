#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Test basic memory allocation functions
int test_memory_functions() {
    printf("Testing memory allocation functions...\n");
    
    // Test malloc
    void *ptr = malloc(1024);
    if (!ptr) {
        printf("FAIL: malloc returned NULL\n");
        return 0;
    }
    printf("PASS: malloc allocated 1024 bytes\n");
    
    // Test calloc
    void *ptr2 = calloc(10, 100);
    if (!ptr2) {
        printf("FAIL: calloc returned NULL\n");
        free(ptr);
        return 0;
    }
    printf("PASS: calloc allocated 10x100 bytes\n");
    
    // Test realloc
    void *ptr3 = realloc(ptr, 2048);
    if (!ptr3) {
        printf("FAIL: realloc returned NULL\n");
        free(ptr);
        free(ptr2);
        return 0;
    }
    printf("PASS: realloc expanded to 2048 bytes\n");
    
    // Test memset
    memset(ptr2, 0xAA, 1000);
    printf("PASS: memset operation completed\n");
    
    // Test memcpy
    char buffer[100];
    memcpy(buffer, ptr2, 100);
    printf("PASS: memcpy operation completed\n");
    
    // Cleanup
    free(ptr3);
    free(ptr2);
    
    return 1;
}

// Test string functions
int test_string_functions() {
    printf("\nTesting string functions...\n");
    
    const char *test_str = "Hello, World!";
    char *copy = strdup(test_str);
    if (!copy) {
        printf("FAIL: strdup returned NULL\n");
        return 0;
    }
    
    if (strcmp(copy, test_str) != 0) {
        printf("FAIL: strcmp mismatch\n");
        free(copy);
        return 0;
    }
    printf("PASS: string duplication and comparison\n");
    
    size_t len = strlen(test_str);
    if (len != 13) {
        printf("FAIL: strlen returned %zu, expected 13\n", len);
        free(copy);
        return 0;
    }
    printf("PASS: strlen returned correct length\n");
    
    free(copy);
    return 1;
}

// Test math functions
int test_math_functions() {
    printf("\nTesting math functions...\n");
    
    // Test basic operations
    double result = sin(3.14159 / 2.0);
    if (result < 0.99 || result > 1.01) {
        printf("FAIL: sin(π/2) = %f, expected ~1.0\n", result);
        return 0;
    }
    printf("PASS: sin(π/2) = %f\n", result);
    
    result = cos(0.0);
    if (result < 0.99 || result > 1.01) {
        printf("FAIL: cos(0) = %f, expected 1.0\n", result);
        return 0;
    }
    printf("PASS: cos(0) = %f\n", result);
    
    result = sqrt(16.0);
    if (result < 3.99 || result > 4.01) {
        printf("FAIL: sqrt(16) = %f, expected 4.0\n", result);
        return 0;
    }
    printf("PASS: sqrt(16) = %f\n", result);
    
    return 1;
}

int main() {
    printf("=== Basic System Functions Test ===\n");
    
    int passed = 0;
    int total = 3;
    
    if (test_memory_functions()) passed++;
    if (test_string_functions()) passed++;
    if (test_math_functions()) passed++;
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d\n", passed, total);
    
    if (passed == total) {
        printf("SUCCESS: All basic system functions working!\n");
        return 0;
    } else {
        printf("FAILURE: Some tests failed!\n");
        return 1;
    }
}
