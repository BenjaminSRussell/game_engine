#include "core/memory/allocator_linear.h"
#include <stdlib.h>
#include <stdio.h>

LinearAllocator *linear_allocator_create(u64 size) {
    LinearAllocator *alloc = (LinearAllocator *)malloc(sizeof(LinearAllocator));
    if (!alloc) return NULL;

    alloc->start = malloc(size);
    if (!alloc->start) {
        free(alloc);
        return NULL;
    }

    alloc->current = alloc->start;
    alloc->end = (char*)alloc->start + size;
    alloc->total_size = size;
    alloc->used_size = 0;
    alloc->peak_usage = 0;
    alloc->allocation_count = 0;
    alloc->owns_memory = true;
    return alloc;
}

void linear_allocator_destroy(LinearAllocator *alloc) {
    if (!alloc) return;
    if (alloc->owns_memory && alloc->start) {
        free(alloc->start);
    }
    free(alloc);
}

void *linear_allocator_alloc(LinearAllocator *alloc, u64 size, u64 alignment) {
    if (!alloc) return NULL;

    uintptr_t current_addr = (uintptr_t)alloc->current;
    uintptr_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
    
    // Check overhead from alignment
    u64 adjustment = aligned_addr - current_addr;
    u64 required = size + adjustment;

    if ((char*)alloc->current + required > (char*)alloc->end) {
        return NULL;
    }

    void *aligned_ptr = (void*)aligned_addr;
    alloc->current = (char*)alloc->current + required;
    alloc->used_size += required;
    alloc->allocation_count++;

    if (alloc->used_size > alloc->peak_usage) {
        alloc->peak_usage = alloc->used_size;
    }

    return aligned_ptr;
}

void linear_allocator_reset(LinearAllocator *alloc) {
    if (!alloc) return;
    alloc->current = alloc->start;
    alloc->used_size = 0;
}

u64 linear_allocator_get_usage(LinearAllocator *alloc) {
    return alloc ? alloc->used_size : 0;
}
