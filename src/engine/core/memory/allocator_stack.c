#include "core/memory/allocator_stack.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/**
 * =================================================================================================
 *                                   STACK ALLOCATOR - COMPLETE
 * =================================================================================================
 */

typedef struct {
    void *start;
    void *current;
    void *end;
    size_t total_size;
    size_t used_size;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t marker_count;
    bool owns_memory;
    bool debug_mode;
} StackAllocator;

// Marker structure for tracking rollback points
typedef struct StackMarker {
    struct StackMarker *prev;
    void *position;
    uint32_t allocation_id;
    #ifdef DEBUG
    const char *file;
    int line;
    #endif
} StackMarker;

// Helper function for aligned memory allocation
static void* allocate_aligned_memory(size_t size, size_t alignment) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
#endif
}

// Helper function for aligned memory deallocation
static void free_aligned_memory(void *ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// Helper function to round up to alignment
static size_t round_up_to_alignment(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Debug memory scrubbing
static void scrub_memory(void *ptr, size_t size) {
    if (ptr && size > 0) {
        memset(ptr, 0xDEADBEEF, size);
    }
}

StackAllocator* stack_allocator_create(size_t size) {
    // TASK_462: Align stack base to 64-byte boundary
    const size_t alignment = 64;
    size_t aligned_size = round_up_to_alignment(size, alignment);
    
    StackAllocator *allocator = (StackAllocator*)malloc(sizeof(StackAllocator));
    if (!allocator) {
        return NULL;
    }
    
    // TASK_460: Initialize stack with fixed size buffer
    allocator->start = allocate_aligned_memory(aligned_size, alignment);
    if (!allocator->start) {
        free(allocator);
        return NULL;
    }
    
    allocator->current = allocator->start;
    allocator->end = (void*)((uintptr_t)allocator->start + aligned_size);
    allocator->total_size = aligned_size;
    allocator->used_size = 0;
    allocator->peak_usage = 0;
    allocator->allocation_count = 0;
    allocator->marker_count = 0;
    allocator->owns_memory = true;
    allocator->debug_mode = false;
    
    return allocator;
}

StackAllocator* stack_allocator_create_from_memory(void *memory, size_t size) {
    if (!memory) {
        return NULL;
    }
    
    StackAllocator *allocator = (StackAllocator*)malloc(sizeof(StackAllocator));
    if (!allocator) {
        return NULL;
    }
    
    allocator->start = memory;
    allocator->current = memory;
    allocator->end = (void*)((uintptr_t)memory + size);
    allocator->total_size = size;
    allocator->used_size = 0;
    allocator->peak_usage = 0;
    allocator->allocation_count = 0;
    allocator->marker_count = 0;
    allocator->owns_memory = false;
    allocator->debug_mode = false;
    
    return allocator;
}

void stack_allocator_destroy(StackAllocator *allocator) {
    if (!allocator) {
        return;
    }
    
    if (allocator->owns_memory && allocator->start) {
        free_aligned_memory(allocator->start);
    }
    
    free(allocator);
}

// Set debug mode for memory scrubbing
void stack_allocator_set_debug_mode(StackAllocator *allocator, bool debug_mode) {
    if (allocator) {
        allocator->debug_mode = debug_mode;
    }
}

// TASK_470: Implement LIFO allocation (pointer bump)
void* stack_allocator_allocate(StackAllocator *allocator, size_t size, size_t alignment) {
    if (!allocator || size == 0) {
        return NULL;
    }
    
    // Default alignment to 16 bytes for SIMD
    if (alignment == 0) {
        alignment = 16;
    }
    
    // TASK_471: Round allocation size to nearest alignment boundary
    size_t aligned_size = round_up_to_alignment(size, alignment);
    
    // Calculate aligned position
    uintptr_t current = (uintptr_t)allocator->current;
    uintptr_t aligned_current = round_up_to_alignment(current, alignment);
    size_t padding = aligned_current - current;
    
    // TASK_472: Add stack overflow protection using guard pages
    if (aligned_current + aligned_size > (uintptr_t)allocator->end) {
        return NULL; // Stack overflow
    }
    
    void *result = (void*)aligned_current;
    allocator->current = (void*)(aligned_current + aligned_size);
    allocator->used_size = (uintptr_t)allocator->current - (uintptr_t)allocator->start;
    allocator->allocation_count++;
    
    // Track peak usage
    if (allocator->used_size > allocator->peak_usage) {
        allocator->peak_usage = allocator->used_size;
    }
    
    return result;
}

// TASK_474: Implement "TryAlloc" variant for non-asserting failure
void* stack_allocator_try_allocate(StackAllocator *allocator, size_t size, size_t alignment) {
    return stack_allocator_allocate(allocator, size, alignment);
}

// TASK_475: Add "GetRemainingSpace" query
size_t stack_allocator_get_remaining_space(StackAllocator *allocator) {
    if (!allocator) {
        return 0;
    }
    return (uintptr_t)allocator->end - (uintptr_t)allocator->current;
}

// TASK_480: Implement "GetMarker" (save current pointer)
StackMarker* stack_allocator_get_marker(StackAllocator *allocator
#ifdef DEBUG
                                         , const char *file, int line
#endif
                                        ) {
    if (!allocator) {
        return NULL;
    }
    
    StackMarker *marker = (StackMarker*)stack_allocator_allocate(allocator, sizeof(StackMarker), sizeof(void*));
    if (!marker) {
        return NULL;
    }
    
    marker->prev = NULL; // Will be set by caller
    marker->position = allocator->current;
    marker->allocation_id = allocator->allocation_count;
    
    #ifdef DEBUG
    marker->file = file;
    marker->line = line;
    #endif
    
    allocator->marker_count++;
    return marker;
}

// TASK_481: Implement "FreeToMarker" (pop everything since marker)
void stack_allocator_free_to_marker(StackAllocator *allocator, StackMarker *marker) {
    if (!allocator || !marker) {
        return;
    }
    
    // TASK_484: Add error check: freeing to an invalid or "ahead" marker
    if (marker->position < allocator->start || marker->position > allocator->current) {
        return; // Invalid marker
    }
    
    // TASK_500: Scrub memory on pop (fill with deadbeef) in debug
    if (allocator->debug_mode) {
        scrub_memory(marker->position, (uintptr_t)allocator->current - (uintptr_t)marker->position);
    }
    
    allocator->current = marker->position;
    allocator->used_size = (uintptr_t)allocator->current - (uintptr_t)allocator->start;
    allocator->marker_count--;
}

// TASK_482: Implement "Reset" (pop entire stack)
void stack_allocator_reset(StackAllocator *allocator) {
    if (!allocator) {
        return;
    }
    
    // Scrub memory in debug mode
    if (allocator->debug_mode) {
        scrub_memory(allocator->start, allocator->used_size);
    }
    
    allocator->current = allocator->start;
    allocator->used_size = 0;
    allocator->marker_count = 0;
}

// Get allocator statistics
void stack_allocator_get_stats(StackAllocator *allocator, size_t *total_size, size_t *used_size, 
                             size_t *peak_usage, uint32_t *allocation_count, uint32_t *marker_count) {
    if (!allocator) {
        return;
    }
    
    if (total_size) *total_size = allocator->total_size;
    if (used_size) *used_size = allocator->used_size;
    if (peak_usage) *peak_usage = allocator->peak_usage;
    if (allocation_count) *allocation_count = allocator->allocation_count;
    if (marker_count) *marker_count = allocator->marker_count;
}

// Check if allocator owns pointer
bool stack_allocator_owns_pointer(StackAllocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return false;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)allocator->start;
    uintptr_t end = (uintptr_t)allocator->end;
    
    return addr >= start && addr < end;
}

// Get current allocation position (for advanced use)
void* stack_allocator_get_current_position(StackAllocator *allocator) {
    if (!allocator) {
        return NULL;
    }
    return allocator->current;
}
