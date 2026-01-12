#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>

// Test pointer arithmetic issues
int test_pointer_arithmetic() {
    printf("Testing pointer arithmetic...\n");
    
    int array[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int *ptr = array;
    
    // Test valid pointer arithmetic
    ptr += 5;
    if (*ptr != 5) {
        printf("FAIL: Pointer arithmetic failed\n");
        return 0;
    }
    printf("PASS: Valid pointer arithmetic\n");
    
    // Test pointer difference
    ptrdiff_t diff = ptr - array;
    if (diff != 5) {
        printf("FAIL: Pointer difference failed\n");
        return 0;
    }
    printf("PASS: Pointer difference calculation\n");
    
    return 1;
}

// Test integer overflow
int test_integer_overflow() {
    printf("\nTesting integer overflow...\n");
    
    int max_int = INT_MAX;
    int result = max_int + 1;
    
    if (result > max_int) {
        printf("FAIL: Integer overflow not detected\n");
        return 0;
    }
    printf("PASS: Integer overflow wrapped around\n");
    
    // Test unsigned overflow
    unsigned int max_uint = UINT_MAX;
    unsigned int uint_result = max_uint + 1;
    
    if (uint_result != 0) {
        printf("FAIL: Unsigned overflow failed\n");
        return 0;
    }
    printf("PASS: Unsigned overflow wrapped to zero\n");
    
    return 1;
}

// Test floating point precision
int test_floating_point_precision() {
    printf("\nTesting floating point precision...\n");
    
    float f1 = 0.1f;
    float f2 = 0.2f;
    float sum = f1 + f2;
    
    // Due to floating point precision, 0.1 + 0.2 != 0.3 exactly
    if (sum == 0.3f) {
    } else {
        printf("PASS: Floating point precision issue detected\n");
    }
    
    // Test with epsilon comparison
    float epsilon = 0.0001f;
    if (fabs(sum - 0.3f) < epsilon) {
        printf("PASS: Epsilon comparison works\n");
    } else {
        printf("FAIL: Epsilon comparison failed\n");
        return 0;
    }
    
    return 1;
}

// Test null pointer dereference protection
int test_null_pointer() {
    printf("\nTesting null pointer handling...\n");
    
    int *null_ptr = NULL;
    
    // This should crash if we actually dereference
    // Instead, we'll test the detection
    if (null_ptr == NULL) {
        printf("PASS: NULL pointer detected\n");
    } else {
        printf("FAIL: NULL pointer not detected\n");
        return 0;
    }
    
    // Test function pointer
    void (*func_ptr)(void) = NULL;
    if (func_ptr == NULL) {
        printf("PASS: NULL function pointer detected\n");
    } else {
        printf("FAIL: NULL function pointer not detected\n");
        return 0;
    }
    
    return 1;
}

// Test array bounds
int test_array_bounds() {
    printf("\nTesting array bounds...\n");
    
    int array[5] = {1, 2, 3, 4, 5};
    
    // Valid access
    int val = array[2];
    if (val != 3) {
        printf("FAIL: Valid array access failed\n");
        return 0;
    }
    printf("PASS: Valid array access\n");
    
    // Calculate array size
    size_t size = sizeof(array) / sizeof(array[0]);
    if (size != 5) {
        printf("FAIL: Array size calculation failed\n");
        return 0;
    }
    printf("PASS: Array size calculation\n");
    
    return 1;
}

int main() {
    printf("=== Advanced Failure Detection Tests ===\n");
    
    int passed = 0;
    int total = 5;
    
    if (test_pointer_arithmetic()) passed++;
    if (test_integer_overflow()) passed++;
    if (test_floating_point_precision()) passed++;
    if (test_null_pointer()) passed++;
    if (test_array_bounds()) passed++;
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d\n", passed, total);
    
    if (passed == total) {
        printf("SUCCESS: All advanced tests passed!\n");
        return 0;
    } else {
        printf("FAILURE: Some tests failed!\n");
        return 1;
    }
}
