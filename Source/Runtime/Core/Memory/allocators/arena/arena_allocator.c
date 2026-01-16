// arena_allocator.c - Arena/bump allocator implementation
#include "arena_allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Align pointer to specified alignment
static inline void *align_pointer(void *ptr, size_t alignment) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void *)aligned;
}

ArenaAllocator *arena_create(size_t size) {
  ArenaAllocator *arena = (ArenaAllocator *)malloc(sizeof(ArenaAllocator));
  if (!arena)
    return NULL;

  arena->buffer = malloc(size);
  if (!arena->buffer) {
    free(arena);
    return NULL;
  }

  arena->size = size;
  arena->offset = 0;
  arena->previous_offset = 0;
  arena->owns_memory = true;

  return arena;
}

ArenaAllocator *arena_create_from_buffer(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return NULL;

  ArenaAllocator *arena = (ArenaAllocator *)malloc(sizeof(ArenaAllocator));
  if (!arena)
    return NULL;

  arena->buffer = buffer;
  arena->size = size;
  arena->offset = 0;
  arena->previous_offset = 0;
  arena->owns_memory = false;

  return arena;
}

void *arena_alloc(ArenaAllocator *arena, size_t size) {
  if (!arena || size == 0)
    return NULL;

  // Check if we have enough space
  if (arena->offset + size > arena->size) {
    return NULL; // Arena full
  }

  void *ptr = (char *)arena->buffer + arena->offset;
  arena->previous_offset = arena->offset;
  arena->offset += size;

  // Zero the memory
  memset(ptr, 0, size);

  return ptr;
}

void *arena_alloc_aligned(ArenaAllocator *arena, size_t size,
                          size_t alignment) {
  if (!arena || size == 0)
    return NULL;

  // Calculate aligned offset
  void *current_ptr = (char *)arena->buffer + arena->offset;
  void *aligned_ptr = align_pointer(current_ptr, alignment);
  size_t padding = (char *)aligned_ptr - (char *)current_ptr;

  // Check if we have enough space including padding
  if (arena->offset + padding + size > arena->size) {
    return NULL; // Arena full
  }

  arena->previous_offset = arena->offset;
  arena->offset += padding + size;

  // Zero the memory
  memset(aligned_ptr, 0, size);

  return aligned_ptr;
}

void arena_reset(ArenaAllocator *arena) {
  if (!arena)
    return;
  arena->offset = 0;
  arena->previous_offset = 0;
}

size_t arena_get_offset(ArenaAllocator *arena) {
  return arena ? arena->offset : 0;
}

size_t arena_get_remaining(ArenaAllocator *arena) {
  if (!arena)
    return 0;
  return arena->size - arena->offset;
}

void arena_destroy(ArenaAllocator *arena) {
  if (!arena)
    return;

  if (arena->owns_memory && arena->buffer) {
    free(arena->buffer);
  }

  free(arena);
}
