#include "../../../src/engine/include/core/memory.h"
#include "../../../src/engine/include/core/vulkan_memory.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>

// Mock Vulkan structures if not building with full Vulkan
#ifndef VULKAN_BUILD
typedef struct VkAllocationCallbacks {
    void* pUserData;
    void* (*pfnAllocation)(void*, size_t, size_t, int);
    void* (*pfnReallocation)(void*, void*, size_t, size_t, int);
    void (*pfnFree)(void*, void*);
    void (*pfnInternalAllocation)(void*, size_t, int, int);
    void (*pfnInternalFree)(void*, size_t, int, int);
} VkAllocationCallbacks;
#endif

// Test aligned allocation callback
static TestResult test_vulkan_alloc_callback_alignment(void) {
    memory_tracker_init(1024);
    
    const VkAllocationCallbacks* callbacks = memory_get_vulkan_callbacks();
    TEST_ASSERT_NOT_NULL(callbacks, "Callbacks retrieved");
    TEST_ASSERT_NOT_NULL(callbacks->pfnAllocation, "Allocation callback present");
    
    // Test 16-byte alignment
    void* ptr16 = callbacks->pfnAllocation(NULL, 64, 16, 0);
    TEST_ASSERT_NOT_NULL(ptr16, "16-byte aligned alloc success");
    TEST_ASSERT_EQ((uintptr_t)ptr16 % 16, 0, "Pointer is 16-byte aligned");
    
    // Test 64-byte alignment
    void* ptr64 = callbacks->pfnAllocation(NULL, 128, 64, 0);
    TEST_ASSERT_NOT_NULL(ptr64, "64-byte aligned alloc success");
    TEST_ASSERT_EQ((uintptr_t)ptr64 % 64, 0, "Pointer is 64-byte aligned");
    
    // Cleanup
    callbacks->pfnFree(NULL, ptr16);
    callbacks->pfnFree(NULL, ptr64);
    
    memory_tracker_shutdown();
    return TEST_PASS;
}

// Test reallocation edge cases
static TestResult test_vulkan_realloc_callback_edge_cases(void) {
    memory_tracker_init(1024);
    
    const VkAllocationCallbacks* callbacks = memory_get_vulkan_callbacks();
    
    // Test NULL original -> Alloc
    void* ptr = callbacks->pfnReallocation(NULL, NULL, 64, 16, 0);
    TEST_ASSERT_NOT_NULL(ptr, "Realloc(NULL) behaves as Alloc");
    
    // Test Zero size -> Free
    // Note: implementation might return NULL or a specific marker, but standard convention for 0 size is free
    // Our implementation returns NULL for size 0
    void* result = callbacks->pfnReallocation(NULL, ptr, 0, 16, 0);
    TEST_ASSERT_NULL(result, "Realloc(0) behaves as Free");
    
    memory_tracker_shutdown();
    return TEST_PASS;
}

int main(void) {
    test_init();
    
    test_register("Core:VulkanMemory", "Alignment", test_vulkan_alloc_callback_alignment, NULL, NULL);
    test_register("Core:VulkanMemory", "Realloc Edge Cases", test_vulkan_realloc_callback_edge_cases, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
