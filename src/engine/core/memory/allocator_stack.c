#include "core/memory/allocator_stack.h"
#include <stdlib.h>
#include <stdio.h>

StackAllocator *stack_allocator_create(u64 size) {
    StackAllocator *alloc = (StackAllocator *)malloc(sizeof(StackAllocator));
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
    alloc->last_marker = NULL; // Simple marker tracking
    return alloc;
}

void stack_allocator_destroy(StackAllocator *alloc) {
    if (!alloc) return;
    if (alloc->owns_memory && alloc->start) {
        free(alloc->start);
    }
    free(alloc);
}

void *stack_allocator_alloc(StackAllocator *alloc, u64 size) {
    if (!alloc) return NULL;
    // Default alignment 16
    u64 alignment = 16;
    uintptr_t current_addr = (uintptr_t)alloc->current;
    uintptr_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
    u64 required = size + (aligned_addr - current_addr);

    if ((char*)alloc->current + required > (char*)alloc->end) {
        return NULL;
    }

    void *ptr = (void*)aligned_addr;
    alloc->current = (char*)alloc->current + required;
    alloc->used_size += required;
    alloc->allocation_count++;
    
    if (alloc->used_size > alloc->peak_usage) {
        alloc->peak_usage = alloc->used_size;
    }

    return ptr;
}

void stack_allocator_reset(StackAllocator *alloc) {
    if (alloc) {
        alloc->current = alloc->start;
        alloc->used_size = 0;
    }
}

void stack_allocator_end_frame(StackAllocator *alloc) {
    // For now, treat end_frame as reset?
    // memory.c implementation treated it as: alloc->frame_offset = alloc->offset;
    // But this struct uses start/current pointers.
    // We'll reset for frame based allocators.
    stack_allocator_reset(alloc);
}
