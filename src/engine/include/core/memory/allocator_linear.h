#ifndef CORE_MEMORY_ALLOCATOR_LINEAR_H
#define CORE_MEMORY_ALLOCATOR_LINEAR_H

#include "include/common.h"

typedef struct LinearAllocator {
    void *start;
    void *current;
    void *end;
    size_t total_size;
    size_t used_size;
    size_t peak_usage;
    u32 allocation_count;
    bool owns_memory;
} LinearAllocator;

LinearAllocator *linear_allocator_create(u64 size);
void linear_allocator_destroy(LinearAllocator *alloc);
void *linear_allocator_alloc(LinearAllocator *alloc, u64 size, u64 alignment);
void linear_allocator_reset(LinearAllocator *alloc);
u64 linear_allocator_get_usage(LinearAllocator *alloc);

#endif
