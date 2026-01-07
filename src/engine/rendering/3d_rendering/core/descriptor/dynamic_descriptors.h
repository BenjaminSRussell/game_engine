/*
 * dynamic_descriptors.h
 * Dynamic uniform/storage buffer management
 */

#ifndef CORE_DYNAMIC_DESCRIPTORS_H
#define CORE_DYNAMIC_DESCRIPTORS_H

#include "../../resource_management/resource_handle.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Allocator for dynamic UBOs (linear ring buffer strategy)
typedef struct dynamic_allocator_t dynamic_allocator_t;

// Initialize a new allocator
// buffer_size: Total size of the underlying GPU buffer (e.g., 64MB)
dynamic_allocator_t* dynamic_allocator_create(size_t buffer_size);

void dynamic_allocator_destroy(dynamic_allocator_t* allocator);

// Reset allocator at start of frame (or when wrapping)
void dynamic_allocator_reset(dynamic_allocator_t* allocator);

// Allocate a slice of memory
// Returns true on success, populating out_offset
// Returns false if out of memory (caller should flush or error)
bool dynamic_allocator_alloc(dynamic_allocator_t* allocator, 
                             size_t size, 
                             size_t alignment, 
                             uint32_t* out_offset,
                             void** out_mapped_ptr);

// Get the underlying buffer handle
buffer_handle_t dynamic_allocator_get_buffer(dynamic_allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DYNAMIC_DESCRIPTORS_H */
