#ifndef CORE_MEMORY_ALLOCATOR_STACK_H
#define CORE_MEMORY_ALLOCATOR_STACK_H

#include <common.h>

typedef struct StackAllocator {
    void *start;
    void *current;
    void *end;
    size_t total_size;
    size_t used_size;
    size_t peak_usage;
    u32 allocation_count;
    bool owns_memory;
    void *last_marker;
} StackAllocator;

StackAllocator *stack_allocator_create(u64 size);
void stack_allocator_destroy(StackAllocator *alloc);
void *stack_allocator_alloc(StackAllocator *alloc, u64 size);
void stack_allocator_reset(StackAllocator *alloc);
void stack_allocator_end_frame(StackAllocator *alloc); // Maps to marker pop/reset logic if needed, or simple reset

#endif
