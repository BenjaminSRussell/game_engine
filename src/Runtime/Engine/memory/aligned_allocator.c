#include "core/memory/aligned_allocator.h"
#include <stdlib.h>
#include <stdio.h>

struct AlignedAllocator {
    size_t total_size;
    size_t alignment;
    // Simple implementation wrapping malloc/posix_memalign
};

AlignedAllocator* aligned_allocator_create(size_t size, size_t alignment) {
    AlignedAllocator* allocator = (AlignedAllocator*)malloc(sizeof(AlignedAllocator));
    if (allocator) {
        allocator->total_size = size;
        allocator->alignment = alignment;
    }
    return allocator;
}

void aligned_allocator_destroy(AlignedAllocator* allocator) {
    free(allocator);
}

void* aligned_allocator_allocate(AlignedAllocator* allocator, size_t size, size_t alignment) {
    void* ptr = NULL;
    // Use posix_memalign for aligned allocation
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

void aligned_allocator_deallocate(AlignedAllocator* allocator, void* ptr) {
    free(ptr);
}
