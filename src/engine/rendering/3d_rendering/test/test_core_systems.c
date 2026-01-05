#include "../core/device/device_context.h"
#include "../core/memory/gpu_allocator.h"
#include "../core/device/device_memory.h"
#include <stdio.h>
#include <assert.h>

void test_device_context_init() {
    printf("Testing device context initialization...\n");
    int result = core_device_context_init();
    assert(result == 0);
    printf("Device context initialized.\n");
    core_device_context_shutdown();
}

void test_gpu_allocator() {
    printf("Testing GPU allocator...\n");
    core_gpu_allocator_init();
    core_device_memory_init();
    
    core_gpu_allocator_handle_t handle;
    core_gpu_allocator_desc_t desc = {0};
    int result = core_gpu_allocator_create(&handle, &desc);
    assert(result == 0);

    gpu_allocation_t alloc = {0};
    alloc.flags = GPU_ALLOCATION_FLAG_DEVICE_LOCAL;
    result = core_gpu_allocator_alloc(handle, 1024, 256, &alloc);
    assert(result == 0);
    assert(alloc.size == 1024);

    core_gpu_allocator_free(handle, &alloc);
    core_gpu_allocator_destroy(handle);
    core_device_memory_shutdown();
    core_gpu_allocator_shutdown();
    printf("GPU allocator tests passed.\n");
}

int main() {
    test_device_context_init();
    test_gpu_allocator();
    printf("All core system tests passed!\n");
    return 0;
}
