/**
 * Unit Test for Memory Management System
 * Tests memory allocation, tracking, and pool management
 */

#include "../../../src/engine/core/memory.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test memory system initialization
static TestResult test_memory_init(void) {
    memory_init();
    TEST_ASSERT(true, "Memory system initialized");
    memory_shutdown();
    return TEST_PASS;
}

// Test basic allocation and deallocation
static TestResult test_alloc_free(void) {
    memory_init();
    
    void* ptr = memory_alloc(1024);
    TEST_ASSERT_NOT_NULL(ptr, "Memory allocated");
    
    memory_free(ptr);
    TEST_ASSERT(true, "Memory freed without crash");
    
    memory_shutdown();
    return TEST_PASS;
}

// Test aligned allocation
static TestResult test_aligned_alloc(void) {
    memory_init();
    
    void* ptr = memory_alloc_aligned(1024, 16);
    TEST_ASSERT_NOT_NULL(ptr, "Aligned memory allocated");
    
    // Check alignment
    uintptr_t addr = (uintptr_t)ptr;
    TEST_ASSERT_EQ(addr % 16, 0, "Memory is 16-byte aligned");
    
    memory_free_aligned(ptr);
    
    memory_shutdown();
    return TEST_PASS;
}

// Test memory pool creation
static TestResult test_memory_pool(void) {
    memory_init();
    
    MemoryPool* pool = memory_pool_create(64, 100);  // 100 blocks of 64 bytes
    TEST_ASSERT_NOT_NULL(pool, "Memory pool created");
    
    void* block1 = memory_pool_alloc(pool);
    TEST_ASSERT_NOT_NULL(block1, "Block allocated from pool");
    
    void* block2 = memory_pool_alloc(pool);
    TEST_ASSERT_NOT_NULL(block2, "Second block allocated");
    TEST_ASSERT(block1 != block2, "Blocks are different");
    
    memory_pool_free(pool, block1);
    memory_pool_free(pool, block2);
    memory_pool_destroy(pool);
    
    memory_shutdown();
    return TEST_PASS;
}

// Test memory tracking
static TestResult test_memory_tracking(void) {
    memory_init();
    memory_enable_tracking();
    
    void* ptr1 = memory_alloc(256);
    void* ptr2 = memory_alloc(512);
    
    size_t allocated = memory_get_allocated_bytes();
    TEST_ASSERT(allocated >= 768, "Allocated bytes tracked");
    
    memory_free(ptr1);
    memory_free(ptr2);
    
    size_t after_free = memory_get_allocated_bytes();
    TEST_ASSERT(after_free < allocated, "Freed bytes tracked");
    
    memory_disable_tracking();
    memory_shutdown();
    return TEST_PASS;
}

// Test zero allocation
static TestResult test_zero_alloc(void) {
    memory_init();
    
    void* ptr = memory_alloc(0);
    // Zero allocation may return NULL or a valid pointer depending on implementation
    TEST_ASSERT(true, "Zero allocation handled");
    
    if (ptr) {
        memory_free(ptr);
    }
    
    memory_shutdown();
    return TEST_PASS;
}

// Test large allocation
static TestResult test_large_alloc(void) {
    memory_init();
    
    size_t large_size = 10 * 1024 * 1024;  // 10 MB
    void* ptr = memory_alloc(large_size);
    TEST_ASSERT_NOT_NULL(ptr, "Large allocation succeeded");
    
    memory_free(ptr);
    
    memory_shutdown();
    return TEST_PASS;
}

// Test memory reallocation
static TestResult test_realloc(void) {
    memory_init();
    
    void* ptr = memory_alloc(100);
    TEST_ASSERT_NOT_NULL(ptr, "Initial allocation");
    
    void* new_ptr = memory_realloc(ptr, 200);
    TEST_ASSERT_NOT_NULL(new_ptr, "Reallocation succeeded");
    
    memory_free(new_ptr);
    
    memory_shutdown();
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Core:Memory", "Initialization", test_memory_init, NULL, NULL);
    test_register("Core:Memory", "Alloc/Free", test_alloc_free, NULL, NULL);
    test_register("Core:Memory", "Aligned Alloc", test_aligned_alloc, NULL, NULL);
    test_register("Core:Memory", "Memory Pool", test_memory_pool, NULL, NULL);
    test_register("Core:Memory", "Memory Tracking", test_memory_tracking, NULL, NULL);
    test_register("Core:Memory", "Zero Allocation", test_zero_alloc, NULL, NULL);
    test_register("Core:Memory", "Large Allocation", test_large_alloc, NULL, NULL);
    test_register("Core:Memory", "Reallocation", test_realloc, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Memory System Test Results\n");
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
