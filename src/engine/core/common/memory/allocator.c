/**
 * CUSTOM MEMORY ALLOCATOR
 * High-performance stack and pool allocators for game systems
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// --- Stack Allocator ---
// Good for per-frame temporary allocations
typedef struct {
  uint8_t *memory;
  size_t total_size;
  size_t offset;
} StackAllocator;

StackAllocator *stack_allocator_create(size_t size) {
  StackAllocator *allocator = malloc(sizeof(StackAllocator));
  allocator->memory = malloc(size);
  allocator->total_size = size;
  allocator->offset = 0;
  return allocator;
}

void *stack_alloc(StackAllocator *sa, size_t size) {
  size_t current_addr = (size_t)sa->memory + sa->offset;
  size_t padding = 0;

  // Calculate alignment padding
  if (current_addr % ALIGNMENT != 0) {
    padding = ALIGNMENT - (current_addr % ALIGNMENT);
  }

  if (sa->offset + padding + size > sa->total_size) {
    return NULL; // Out of memory
  }

  sa->offset += padding;
  void *ptr = sa->memory + sa->offset;
  sa->offset += size;

  return ptr;
}

void stack_reset(StackAllocator *sa) { sa->offset = 0; }

void stack_allocator_destroy(StackAllocator *sa) {
  free(sa->memory);
  free(sa);
}

// --- Pool Allocator ---
// Good for fixed-size objects (particles, entities, nodes)
typedef struct {
  uint8_t *memory;
  size_t block_size;
  size_t block_count;
  void **free_list; // Stack of free block pointers
  size_t free_count;
} PoolAllocator;

PoolAllocator *pool_allocator_create(size_t block_size, size_t block_count) {
  PoolAllocator *pool = malloc(sizeof(PoolAllocator));
  pool->block_size = ALIGN(block_size);
  pool->block_count = block_count;
  pool->memory = malloc(pool->block_size * block_count);
  pool->free_list = malloc(block_count * sizeof(void *));
  pool->free_count = block_count;

  // Initialize free list
  for (size_t i = 0; i < block_count; i++) {
    pool->free_list[i] = pool->memory + (i * pool->block_size);
  }

  return pool;
}

void *pool_alloc(PoolAllocator *pool) {
  if (pool->free_count == 0)
    return NULL;
  return pool->free_list[--pool->free_count];
}

void pool_free(PoolAllocator *pool, void *ptr) {
  if (pool->free_count >= pool->block_count)
    return; // Should not happen
  pool->free_list[pool->free_count++] = ptr;
}

void pool_allocator_destroy(PoolAllocator *pool) {
  free(pool->memory);
  free(pool->free_list);
  free(pool);
}

// --- Linear Allocator (Arena) ---
// Simple "pointer bump", can only be freed all at once
typedef struct {
  uint8_t *memory;
  size_t total_size;
  size_t offset;
} LinearAllocator;

LinearAllocator *linear_allocator_create(size_t size) {
  LinearAllocator *allocator = malloc(sizeof(LinearAllocator));
  allocator->memory = malloc(size);
  allocator->total_size = size;
  allocator->offset = 0;
  return allocator;
}

void *linear_alloc(LinearAllocator *la, size_t size) {
  size_t current_addr = (size_t)la->memory + la->offset;
  size_t padding = 0;

  if (current_addr % ALIGNMENT != 0) {
    padding = ALIGNMENT - (current_addr % ALIGNMENT);
  }

  if (la->offset + padding + size > la->total_size) {
    return NULL;
  }

  la->offset += padding;
  void *ptr = la->memory + la->offset;
  la->offset += size;

  return ptr;
}

void linear_reset(LinearAllocator *la) { la->offset = 0; }

void linear_allocator_destroy(LinearAllocator *la) {
  free(la->memory);
  free(la);
}
