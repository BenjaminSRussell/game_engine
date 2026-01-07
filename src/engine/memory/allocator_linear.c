#include "allocator_linear.h"
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
 *                                   LINEAR ALLOCATOR - COMPLETE
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
    bool owns_memory;
} LinearAllocator;

// Global allocator tracking
static LinearAllocator *g_allocators[32];
static uint32_t g_allocator_count = 0;

// Helper function to get page size
static size_t get_page_size() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

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

LinearAllocator* linear_allocator_create(size_t size) {
    // TASK_400: Allocate backing memory using VirtualAlloc (Windows) or mmap (Unix)
    // TASK_401: Ensure 64-byte alignment for cache line optimization
    const size_t alignment = 64;
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    LinearAllocator *allocator = (LinearAllocator*)malloc(sizeof(LinearAllocator));
    if (!allocator) {
        return NULL;
    }
    
    allocator->start = allocate_aligned_memory(aligned_size, alignment);
    if (!allocator->start) {
        free(allocator);
        return NULL;
    }
    
    // TASK_402: Initialize allocator metadata structure
    allocator->current = allocator->start;
    allocator->end = (void*)((uintptr_t)allocator->start + aligned_size);
    allocator->total_size = aligned_size;
    allocator->used_size = 0;
    allocator->peak_usage = 0;
    allocator->allocation_count = 0;
    allocator->owns_memory = true;
    
    // TASK_404: Register allocator with global allocator tracking system
    if (g_allocator_count < 32) {
        g_allocators[g_allocator_count++] = allocator;
    }
    
    return allocator;
}

LinearAllocator* linear_allocator_create_from_memory(void *memory, size_t size) {
    if (!memory) {
        return NULL;
    }
    
    LinearAllocator *allocator = (LinearAllocator*)malloc(sizeof(LinearAllocator));
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
    allocator->owns_memory = false;
    
    return allocator;
}

void linear_allocator_destroy(LinearAllocator *allocator) {
    if (!allocator) {
        return;
    }
    
    // Remove from global tracking
    for (uint32_t i = 0; i < g_allocator_count; i++) {
        if (g_allocators[i] == allocator) {
            g_allocators[i] = g_allocators[--g_allocator_count];
            break;
        }
    }
    
    if (allocator->owns_memory && allocator->start) {
        free_aligned_memory(allocator->start);
    }
    
    free(allocator);
}

// TASK_410: Implement aligned allocation with configurable alignment
void* linear_allocator_allocate(LinearAllocator *allocator, size_t size, size_t alignment) {
    if (!allocator || size == 0) {
        return NULL;
    }
    
    // Default alignment to 16 bytes for SIMD
    if (alignment == 0) {
        alignment = 16;
    }
    
    // TASK_411: Calculate aligned pointer: (ptr + align - 1) & ~(align - 1)
    uintptr_t current = (uintptr_t)allocator->current;
    uintptr_t aligned_current = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned_current - current;
    
    // TASK_412: Check for overflow before bumping pointer
    uintptr_t end = (uintptr_t)allocator->end;
    if (aligned_current + size > end) {
        // TASK_413: Handle allocation failure gracefully (return NULL or assert)
        return NULL;
    }
    
    void *result = (void*)aligned_current;
    allocator->current = (void*)(aligned_current + size);
    allocator->used_size = (uintptr_t)allocator->current - (uintptr_t)allocator->start;
    allocator->allocation_count++;
    
    // TASK_414: Track allocation high-water mark for profiling
    if (allocator->used_size > allocator->peak_usage) {
        allocator->peak_usage = allocator->used_size;
    }
    
    return result;
}

// TASK_420: Implement fast reset by resetting pointer to start
void linear_allocator_reset(LinearAllocator *allocator) {
    if (!allocator) {
        return;
    }
    
    allocator->current = allocator->start;
    allocator->used_size = 0;
    allocator->allocation_count = 0;
}

// TASK_422: Implement scoped markers for partial resets
LinearAllocatorMarker linear_allocator_get_marker(LinearAllocator *allocator) {
    if (!allocator) {
        return 0;
    }
    return (LinearAllocatorMarker)((uintptr_t)allocator->current - (uintptr_t)allocator->start);
}

void linear_allocator_reset_to_marker(LinearAllocator *allocator, LinearAllocatorMarker marker) {
    if (!allocator || marker > allocator->total_size) {
        return;
    }
    
    allocator->current = (void*)((uintptr_t)allocator->start + marker);
    allocator->used_size = (uintptr_t)allocator->current - (uintptr_t)allocator->start;
}

// Get allocator statistics
void linear_allocator_get_stats(LinearAllocator *allocator, size_t *total_size, size_t *used_size, 
                               size_t *peak_usage, uint32_t *allocation_count) {
    if (!allocator) {
        return;
    }
    
    if (total_size) *total_size = allocator->total_size;
    if (used_size) *used_size = allocator->used_size;
    if (peak_usage) *peak_usage = allocator->peak_usage;
    if (allocation_count) *allocation_count = allocator->allocation_count;
}

// Get remaining space
size_t linear_allocator_get_remaining_space(LinearAllocator *allocator) {
    if (!allocator) {
        return 0;
    }
    return (uintptr_t)allocator->end - (uintptr_t)allocator->current;
}

// Check if allocator owns pointer
bool linear_allocator_owns_pointer(LinearAllocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return false;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)allocator->start;
    uintptr_t end = (uintptr_t)allocator->end;
    
    return addr >= start && addr < end;
}
