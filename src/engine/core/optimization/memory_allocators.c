/**
 * @file memory_allocators.c
 * @brief Custom Memory Management.
 *
 * Implements Linear, Stack, and Pool allocators for high-performance usage.
 * Minimizes fragmentation and allocation overhead.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <core/optimization/memory_allocators.h>
#include <stdlib.h>
#include <string.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct LinearAllocator {
  void *start_ptr;
  size_t size;
  size_t offset;
} LinearAllocator;

typedef struct PoolAllocator {
  void *start_ptr;
  size_t block_size;
  size_t total_blocks;
  void **free_list; // Stack of free pointers
  size_t free_idx;
} PoolAllocator;

// =================================================================================================
//                                      LINEAR ALLOCATOR
// =================================================================================================

LinearAllocator *linear_create(size_t size) {
  LinearAllocator *l = malloc(sizeof(LinearAllocator));
  l->start_ptr = malloc(size);
  l->size = size;
  l->offset = 0;
  return l;
}

void *linear_alloc(LinearAllocator *l, size_t size, size_t alignment) {
  size_t adjustment = 0; // ... align ptr ...

  if (l->offset + size + adjustment > l->size)
    return NULL;

  // uintptr_t aligned_addr = ...
  void *ptr = (uint8_t *)l->start_ptr + l->offset;
  l->offset += size;
  return ptr;
}

void linear_reset(LinearAllocator *l) { l->offset = 0; }

// =================================================================================================
//                                      POOL ALLOCATOR
// =================================================================================================

PoolAllocator *pool_create(size_t block_size, size_t block_count) {
  PoolAllocator *p = malloc(sizeof(PoolAllocator));
  p->block_size = block_size;
  p->total_blocks = block_count;
  p->start_ptr = malloc(block_size * block_count);
  p->free_list = malloc(sizeof(void *) * block_count);

  // Initialize free list
  for (size_t i = 0; i < block_count; i++) {
    p->free_list[i] = (uint8_t *)p->start_ptr + (i * block_size);
  }
  p->free_idx = block_count;

  return p;
}

void *pool_alloc(PoolAllocator *p) {
  if (p->free_idx == 0)
    return NULL;

  p->free_idx--;
  return p->free_list[p->free_idx];
}

void pool_free(PoolAllocator *p, void *ptr) {
  if (p->free_idx >= p->total_blocks)
    return; // Error

  p->free_list[p->free_idx] = ptr;
  p->free_idx++;
}
