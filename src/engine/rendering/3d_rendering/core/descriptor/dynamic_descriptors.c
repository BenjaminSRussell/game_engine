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
    
    // TODO: Create actual GPU buffer here
    // allocator->buffer_handle = resource_manager_create_buffer(...);
    // allocator->mapped_ptr = resource_manager_map_buffer(...);
    
    // For now, mock
    allocator->buffer_handle = buffer_handle_invalid(); // Should be valid in real impl
    allocator->mapped_ptr = malloc(buffer_size); // Mock memory
    
    return allocator;
}

void dynamic_allocator_destroy(dynamic_allocator_t* allocator) {
    if (!allocator) return;
    
    if (allocator->mapped_ptr) {
        free(allocator->mapped_ptr); // Free mock memory
    }
    
    // TODO: Destroy GPU buffer
    
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
