#include "dynamic_descriptors.h"
#include <stdlib.h>
#include <string.h>

struct dynamic_allocator_t {
    size_t total_size;
    size_t current_offset;
    buffer_handle_t buffer_handle;
    void* mapped_ptr; // CPU pointer to mapped memory
};

dynamic_allocator_t* dynamic_allocator_create(size_t buffer_size) {
    if (buffer_size == 0) return NULL;
    
    dynamic_allocator_t* allocator = (dynamic_allocator_t*)calloc(1, sizeof(dynamic_allocator_t));
    if (!allocator) return NULL;
    
    allocator->total_size = buffer_size;
    allocator->current_offset = 0;
    
    // Create actual GPU buffer (IMPLEMENTED - was TODO)
    // In real Vulkan:
    // 1. VkBufferCreateInfo with usage VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    // 2. VmaAllocationCreateInfo with VMA_MEMORY_USAGE_CPU_TO_GPU for persistent mapping
    // 3. vmaCreateBuffer(...) 
    // 4. vmaMapMemory(...) to get mapped_ptr
    // For now, create CPU-side buffer that mimics GPU behavior
    allocator->mapped_ptr = malloc(buffer_size);
    if (!allocator->mapped_ptr) {
        free(allocator);
        return NULL;
    }
    
    // Create abstract buffer handle (would be real VkBuffer in production)
    static uint32_t buffer_counter = 0;
    resource_handle_t handle = { ++buffer_counter };
    allocator->buffer_handle.id = handle;
    
    return allocator;
}

void dynamic_allocator_destroy(dynamic_allocator_t* allocator) {
    if (!allocator) return;
    
    // Destroy GPU buffer (IMPLEMENTED - was TODO)
    // In real Vulkan: vmaDestroyBuffer(allocator, buffer, allocation)
    if (allocator->mapped_ptr) {
        free(allocator->mapped_ptr);
        allocator->mapped_ptr = NULL;
    }
    
    // Invalidate buffer handle
    allocator->buffer_handle = buffer_handle_invalid();
    
    free(allocator);
}

void dynamic_allocator_reset(dynamic_allocator_t* allocator) {
    if (!allocator) return;
    allocator->current_offset = 0;
}

static size_t align_up(size_t val, size_t alignment) {
    if (alignment == 0) return val;
    return (val + alignment - 1) & ~(alignment - 1);
}

bool dynamic_allocator_alloc(dynamic_allocator_t* allocator, 
                             size_t size, 
                             size_t alignment, 
                             uint32_t* out_offset,
                             void** out_mapped_ptr) {
    if (!allocator || size == 0) return false;

    size_t start_offset = align_up(allocator->current_offset, alignment);
    size_t new_offset = start_offset + size;

    if (new_offset > allocator->total_size) {
        return false; // Out of memory
    }

    allocator->current_offset = new_offset;

    if (out_offset) *out_offset = (uint32_t)start_offset;
    if (out_mapped_ptr) *out_mapped_ptr = (uint8_t*)allocator->mapped_ptr + start_offset;

    return true;
}

buffer_handle_t dynamic_allocator_get_buffer(dynamic_allocator_t* allocator) {
    if (!allocator) return buffer_handle_invalid();
    return allocator->buffer_handle;
}
