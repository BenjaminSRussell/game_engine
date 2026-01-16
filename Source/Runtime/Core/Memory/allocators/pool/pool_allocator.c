// pool_allocator.c - Fixed-size object pool implementation
#include "pool_allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

PoolAllocator *pool_create(size_t object_size, size_t object_count) {
  if (object_size == 0 || object_count == 0)
    return NULL;

  // Ensure object size is at least pointer size for free list
  if (object_size < sizeof(void *)) {
    object_size = sizeof(void *);
  }

  PoolAllocator *pool = (PoolAllocator *)malloc(sizeof(PoolAllocator));
  if (!pool)
    return NULL;

  // Allocate buffer for all objects
  pool->buffer = malloc(object_size * object_count);
  if (!pool->buffer) {
    free(pool);
    return NULL;
  }

  pool->object_size = object_size;
  pool->object_count = object_count;
  pool->allocated_count = 0;
  pool->owns_memory = true;
  pool->free_list = NULL;

  // Build free list - link all objects together
  for (size_t i = 0; i < object_count; i++) {
    void *obj = (char *)pool->buffer + (i * object_size);
    *(void **)obj = pool->free_list;
    pool->free_list = obj;
  }

  return pool;
}

PoolAllocator *pool_create_from_buffer(void *buffer, size_t object_size,
                                       size_t object_count) {
  if (!buffer || object_size == 0 || object_count == 0)
    return NULL;

  // Ensure object size is at least pointer size for free list
  if (object_size < sizeof(void *)) {
    object_size = sizeof(void *);
  }

  PoolAllocator *pool = (PoolAllocator *)malloc(sizeof(PoolAllocator));
  if (!pool)
    return NULL;

  pool->buffer = buffer;
  pool->object_size = object_size;
  pool->object_count = object_count;
  pool->allocated_count = 0;
  pool->owns_memory = false;
  pool->free_list = NULL;

  // Build free list
  for (size_t i = 0; i < object_count; i++) {
    void *obj = (char *)pool->buffer + (i * object_size);
    *(void **)obj = pool->free_list;
    pool->free_list = obj;
  }

  return pool;
}

void *pool_alloc(PoolAllocator *pool) {
  if (!pool || !pool->free_list)
    return NULL;

  // Pop from free list
  void *obj = pool->free_list;
  pool->free_list = *(void **)obj;
  pool->allocated_count++;

  // Zero the memory
  memset(obj, 0, pool->object_size);

  return obj;
}

void pool_free(PoolAllocator *pool, void *obj) {
  if (!pool || !obj)
    return;

  // Verify object is within pool bounds
  char *obj_ptr = (char *)obj;
  char *buffer_start = (char *)pool->buffer;
  char *buffer_end = buffer_start + (pool->object_size * pool->object_count);

  if (obj_ptr < buffer_start || obj_ptr >= buffer_end) {
    // Object not from this pool
    return;
  }

  // Verify object is properly aligned
  size_t offset = obj_ptr - buffer_start;
  if (offset % pool->object_size != 0) {
    // Object not properly aligned
    return;
  }

  // Push back to free list
  *(void **)obj = pool->free_list;
  pool->free_list = obj;
  pool->allocated_count--;
}

size_t pool_get_allocated_count(PoolAllocator *pool) {
  return pool ? pool->allocated_count : 0;
}

size_t pool_get_free_count(PoolAllocator *pool) {
  if (!pool)
    return 0;
  return pool->object_count - pool->allocated_count;
}

bool pool_is_full(PoolAllocator *pool) {
  if (!pool)
    return true;
  return pool->free_list == NULL;
}

void pool_destroy(PoolAllocator *pool) {
  if (!pool)
    return;

  if (pool->owns_memory && pool->buffer) {
    free(pool->buffer);
  }

  free(pool);
}
