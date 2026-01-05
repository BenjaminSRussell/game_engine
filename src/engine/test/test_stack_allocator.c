#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "core/memory/stack_allocator.h"

// Test framework
#define TEST(name) printf("Test: %s... ", name)
#define PASS printf("PASSED\n")
#define FAIL printf("FAILED\n"); return 1

int test_create_destroy() {
    TEST("Create/Destroy");

    StackAllocator *alloc = stack_allocator_create(4096);
    assert(alloc != NULL);

    size_t total, used, peak;
    uint32_t count;
    stack_allocator_get_stats(alloc, &total, &used, &peak, &count);
    assert(total == 4096);
    assert(used == 0);
    assert(count == 0);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_allocate_basic() {
    TEST("Basic Allocation");

    StackAllocator *alloc = stack_allocator_create(1024);

    void *ptr1 = stack_allocator_alloc(alloc, 64, 0);
    assert(ptr1 != NULL);

    void *ptr2 = stack_allocator_alloc(alloc, 128, 0);
    assert(ptr2 != NULL);
    assert(ptr2 > ptr1);

    assert(stack_allocator_owns(alloc, ptr1));
    assert(stack_allocator_owns(alloc, ptr2));

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_alignment() {
    TEST("Alignment");

    StackAllocator *alloc = stack_allocator_create(2048);

    void *ptr1 = stack_allocator_alloc(alloc, 7, 32);  // Odd size, request alignment
    void *ptr2 = stack_allocator_alloc(alloc, 16, 0);  // Default alignment

    // Check ptr1 is 32-byte aligned
    assert(((uintptr_t)ptr1 & 31) == 0);
    // Check ptr2 is 16-byte aligned
    assert(((uintptr_t)ptr2 & 15) == 0);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_markers() {
    TEST("Markers and Rewind");

    StackAllocator *alloc = stack_allocator_create(1024);

    void *ptr1 = stack_allocator_alloc(alloc, 64, 0);
    StackMarker marker = stack_allocator_get_marker(alloc);

    void *ptr2 = stack_allocator_alloc(alloc, 128, 0);

    size_t used_before;
    stack_allocator_get_stats(alloc, NULL, &used_before, NULL, NULL);
    assert(used_before > 64);

    stack_allocator_rewind(alloc, marker);

    size_t used_after;
    stack_allocator_get_stats(alloc, NULL, &used_after, NULL, NULL);
    assert(used_after == 64);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_double_ended() {
    TEST("Double-Ended Stack");

    StackAllocator *alloc = stack_allocator_create(1024);

    // Allocate from front
    void *ptr_front = stack_allocator_alloc(alloc, 256, 0);
    assert(ptr_front != NULL);

    // Allocate from back
    void *ptr_back = stack_allocator_alloc_from_end(alloc, 128, 0);
    assert(ptr_back != NULL);
    assert(ptr_back > ptr_front);

    // Should still have room in middle
    void *ptr_mid = stack_allocator_alloc(alloc, 200, 0);
    assert(ptr_mid != NULL);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_pop() {
    TEST("Pop Operation");

    StackAllocator *alloc = stack_allocator_create(1024);

    stack_allocator_alloc(alloc, 64, 0);
    stack_allocator_alloc(alloc, 128, 0);

    uint32_t count_before;
    stack_allocator_get_stats(alloc, NULL, NULL, NULL, &count_before);
    assert(count_before == 2);

    // Pop assumes 16-byte aligned from default alloc
    bool result = stack_allocator_pop(alloc, 128);
    assert(result);

    uint32_t count_after;
    stack_allocator_get_stats(alloc, NULL, NULL, NULL, &count_after);
    assert(count_after == 1);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_reset() {
    TEST("Reset");

    StackAllocator *alloc = stack_allocator_create(1024);

    stack_allocator_alloc(alloc, 256, 0);
    stack_allocator_alloc(alloc, 128, 0);

    size_t used_before;
    stack_allocator_get_stats(alloc, NULL, &used_before, NULL, NULL);
    assert(used_before > 0);

    stack_allocator_reset(alloc);

    size_t used_after;
    uint32_t count;
    stack_allocator_get_stats(alloc, NULL, &used_after, NULL, &count);
    assert(used_after == 0);
    assert(count == 0);

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_out_of_memory() {
    TEST("Out of Memory Handling");

    StackAllocator *alloc = stack_allocator_create(256);

    void *ptr1 = stack_allocator_try_alloc(alloc, 200, 0);
    assert(ptr1 != NULL);

    void *ptr2 = stack_allocator_try_alloc(alloc, 100, 0);
    assert(ptr2 == NULL);  // Should fail

    stack_allocator_destroy(alloc);
    PASS;
    return 0;
}

int test_from_existing_memory() {
    TEST("Create from Existing Memory");

    void *memory = malloc(2048);
    StackAllocator *alloc = stack_allocator_create_from_memory(memory, 2048);

    assert(alloc != NULL);

    void *ptr = stack_allocator_alloc(alloc, 100, 0);
    assert(ptr != NULL);
    assert(ptr != memory);  // Not the allocator itself

    stack_allocator_destroy(alloc);
    free(memory);
    PASS;
    return 0;
}

int main() {
    printf("=== Stack Allocator Tests ===\n\n");

    int tests_passed = 0;
    int tests_failed = 0;

    if (test_create_destroy() == 0) tests_passed++; else tests_failed++;
    if (test_allocate_basic() == 0) tests_passed++; else tests_failed++;
    if (test_alignment() == 0) tests_passed++; else tests_failed++;
    if (test_markers() == 0) tests_passed++; else tests_failed++;
    if (test_double_ended() == 0) tests_passed++; else tests_failed++;
    if (test_pop() == 0) tests_passed++; else tests_failed++;
    if (test_reset() == 0) tests_passed++; else tests_failed++;
    if (test_out_of_memory() == 0) tests_passed++; else tests_failed++;
    if (test_from_existing_memory() == 0) tests_passed++; else tests_failed++;

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
