#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// =================================================================================================
//                          LINEAR ALLOCATOR IMPLEMENTATION
// =================================================================================================

LinearAllocator *linear_allocator_create(u32 size, u32 buffer_count) {
    if (size == 0) return NULL;
    if (buffer_count == 0) buffer_count = 1;

    LinearAllocator *alloc = (LinearAllocator*)malloc(sizeof(LinearAllocator));
    if (!alloc) return NULL;

    alloc->size = size;
    alloc->offset = 0;
    alloc->peak_usage = 0;
    alloc->buffer_count = buffer_count;
    alloc->current_buffer = 0;
    
    alloc->buffers = (void**)malloc(sizeof(void*) * buffer_count);
    if (!alloc->buffers) {
        free(alloc);
        return NULL;
    }

    // Allocate buffers
    for (u32 i = 0; i < buffer_count; i++) {
        // Use calloc or malloc? Malloc is faster, reset handles zeroing if needed.
        // We'll aligned_alloc if available, or just malloc (system malloc is usually 16-byte aligned)
        alloc->buffers[i] = malloc(size);
        if (!alloc->buffers[i]) {
            // Cleanup partial
            for (u32 j = 0; j < i; j++) free(alloc->buffers[j]);
            free(alloc->buffers);
            free(alloc);
            return NULL;
        }
    }

    alloc->base = alloc->buffers[0];
    return alloc;
}

void linear_allocator_destroy(LinearAllocator *alloc) {
    if (!alloc) return;

    if (alloc->buffers) {
        for (u32 i = 0; i < alloc->buffer_count; i++) {
            if (alloc->buffers[i]) free(alloc->buffers[i]);
        }
        free(alloc->buffers);
    }
    free(alloc);
}

void *linear_allocator_alloc(LinearAllocator *alloc, u32 size, u32 alignment) {
    if (!alloc || size == 0) return NULL;

    // Default alignment if 0
    if (alignment == 0) alignment = 8;

    // Align request
    u32 current_offset = alloc->offset;
    u32 adjustment = 0;
    
    // Calculate adjustment for alignment
    // We assume base is aligned enough, or we calculate strictly relative to base+offset
    uintptr_t current_ptr = (uintptr_t)alloc->base + current_offset;
    uintptr_t aligned_ptr = (current_ptr + alignment - 1) & ~(uintptr_t)(alignment - 1);
    adjustment = (u32)(aligned_ptr - current_ptr);

    if (alloc->offset + adjustment + size > alloc->size) {
        // Out of memory in current buffer
        return NULL;
    }

    alloc->offset += adjustment;
    void *ptr = (void*)((uintptr_t)alloc->base + alloc->offset);
    alloc->offset += size;

    if (alloc->offset > alloc->peak_usage) {
        alloc->peak_usage = alloc->offset;
    }

    return ptr;
}

void linear_allocator_reset(LinearAllocator *alloc) {
    if (!alloc) return;
    // Reset ALL buffers? Or just current?
    // "reset" usually implies full reset.
    alloc->offset = 0;
    alloc->current_buffer = 0;
    alloc->base = alloc->buffers[0];
}

void linear_allocator_reset_current(LinearAllocator *alloc) {
    if (!alloc) return;
    alloc->offset = 0;
}

void linear_allocator_swap_buffers(LinearAllocator *alloc) {
    if (!alloc || alloc->buffer_count <= 1) return;

    alloc->current_buffer = (alloc->current_buffer + 1) % alloc->buffer_count;
    alloc->base = alloc->buffers[alloc->current_buffer];
    alloc->offset = 0; // Usually swap implies reset of the new buffer (double buffering frame allocator pattern)
}

// Helpers missing from .h but useful locally or if re-exposed
// Note: linear_allocator_get_stats was not in .h, skipping implementation to match .h API surface
