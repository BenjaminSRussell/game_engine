#ifndef MEMORY_ALLOCATORS_H
#define MEMORY_ALLOCATORS_H

#include <stddef.h>

typedef struct LinearAllocator LinearAllocator;
typedef struct PoolAllocator PoolAllocator;

// Linear Allocator API
LinearAllocator *linear_create(size_t size);
void *linear_alloc(LinearAllocator *l, size_t size, size_t alignment);
void linear_reset(LinearAllocator *l);

// Pool Allocator API
PoolAllocator *pool_create(size_t block_size, size_t block_count);
void *pool_alloc(PoolAllocator *p);
void pool_free(PoolAllocator *p, void *ptr);

#endif // MEMORY_ALLOCATORS_H
