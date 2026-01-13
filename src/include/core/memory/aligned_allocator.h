#ifndef ALIGNED_ALLOCATOR_H
#define ALIGNED_ALLOCATOR_H

#include <stddef.h>

typedef struct AlignedAllocator AlignedAllocator;

AlignedAllocator* aligned_allocator_create(size_t size, size_t alignment);
void aligned_allocator_destroy(AlignedAllocator* allocator);
void* aligned_allocator_allocate(AlignedAllocator* allocator, size_t size, size_t alignment);
void aligned_allocator_deallocate(AlignedAllocator* allocator, void* ptr);

#endif // ALIGNED_ALLOCATOR_H
