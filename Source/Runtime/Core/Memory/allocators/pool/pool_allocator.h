#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

// Pool allocator for fixed-size objects
// Provides O(1) allocation and deallocation
typedef struct PoolAllocator {
  void *buffer;
  size_t object_size;
  size_t object_count;
  void *free_list;
  size_t allocated_count;
  bool owns_memory;
} PoolAllocator;

// Create pool with specified object size and count
PoolAllocator *pool_create(size_t object_size, size_t object_count);

// Create pool from existing buffer
PoolAllocator *pool_create_from_buffer(void *buffer, size_t object_size,
                                       size_t object_count);

// Allocate object from pool
void *pool_alloc(PoolAllocator *pool);

// Free object back to pool
void pool_free(PoolAllocator *pool, void *obj);

// Get allocation statistics
size_t pool_get_allocated_count(PoolAllocator *pool);
size_t pool_get_free_count(PoolAllocator *pool);
bool pool_is_full(PoolAllocator *pool);

// Destroy pool
void pool_destroy(PoolAllocator *pool);

#endif // POOL_ALLOCATOR_H
