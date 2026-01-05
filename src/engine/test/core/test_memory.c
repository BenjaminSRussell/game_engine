#include "test_framework.h"
#include "core/memory.h"
#include <string.h>

// Mock linear allocator if not available yet, or include actual header
// For now, assuming core/memory.h has the definitions as seen in the file view

static LinearAllocator *g_allocator = NULL;

static TestResult setup_linear_allocator(void) {
    g_allocator = linear_allocator_create(1024, 2); // 1KB size, 2 buffers
    if (!g_allocator) return TEST_FAIL;
    return TEST_PASS;
}

static TestResult teardown_linear_allocator(void) {
    if (g_allocator) {
        linear_allocator_destroy(g_allocator);
        g_allocator = NULL;
    }
    return TEST_PASS;
}

static TestResult test_linear_allocator_basic(void) {
    TEST_ASSERT_NOT_NULL(g_allocator, "Allocator should be created");
    
    void *ptr1 = linear_allocator_alloc(g_allocator, 100, 4);
    TEST_ASSERT_NOT_NULL(ptr1, "Should allocate 100 bytes");
    
    void *ptr2 = linear_allocator_alloc(g_allocator, 200, 4);
    TEST_ASSERT_NOT_NULL(ptr2, "Should allocate 200 bytes");
    
    // Check they are distinct and correctly spaced
    TEST_ASSERT((char*)ptr2 >= (char*)ptr1 + 100, "Pointers should not overlap");
    
    return TEST_PASS;
}

static TestResult test_linear_allocator_reset(void) {
    void *ptr1 = linear_allocator_alloc(g_allocator, 100, 4);
    TEST_ASSERT_NOT_NULL(ptr1, "Should allocate 100 bytes");
    
    linear_allocator_reset(g_allocator);
    
    void *ptr2 = linear_allocator_alloc(g_allocator, 100, 4);
    // In a linear allocator, reset should point back to base (or start of buffer)
    // This assertion depends on implementation details, but generally true
    TEST_ASSERT_EQ(ptr1, ptr2, "Reset should reuse memory address");
    
    return TEST_PASS;
}

static TestResult test_linear_allocator_alignment(void) {
    void *ptr1 = linear_allocator_alloc(g_allocator, 1, 1); // 1 byte
    void *ptr2 = linear_allocator_alloc(g_allocator, 10, 16); // 16-byte aligned
    
    uintptr_t addr = (uintptr_t)ptr2;
    TEST_ASSERT_EQ(addr % 16, 0, "Pointer should be 16-byte aligned");
    
    return TEST_PASS;
}

void register_memory_tests(void) {
    TEST_REGISTER_WITH_SETUP("Memory", "LinearAllocator Basic", test_linear_allocator_basic, setup_linear_allocator, teardown_linear_allocator);
    TEST_REGISTER_WITH_SETUP("Memory", "LinearAllocator Reset", test_linear_allocator_reset, setup_linear_allocator, teardown_linear_allocator);
    TEST_REGISTER_WITH_SETUP("Memory", "LinearAllocator Alignment", test_linear_allocator_alignment, setup_linear_allocator, teardown_linear_allocator);
}
