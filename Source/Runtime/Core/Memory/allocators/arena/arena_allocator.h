#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

// Arena allocator for fast sequential allocations
// Perfect for frame-based allocations that can be reset in bulk
typedef struct ArenaAllocator {
  void *buffer;
  size_t size;
  size_t offset;
  size_t previous_offset;
  bool owns_memory;
} ArenaAllocator;

// Create arena with specified size
ArenaAllocator *arena_create(size_t size);

// Create arena from existing buffer (doesn't own memory)
ArenaAllocator *arena_create_from_buffer(void *buffer, size_t size);

// Allocate from arena
void *arena_alloc(ArenaAllocator *arena, size_t size);

// Allocate with alignment
void *arena_alloc_aligned(ArenaAllocator *arena, size_t size, size_t alignment);

// Reset arena to beginning (invalidates all previous allocations)
void arena_reset(ArenaAllocator *arena);

// Get current offset
size_t arena_get_offset(ArenaAllocator *arena);

// Get remaining space
size_t arena_get_remaining(ArenaAllocator *arena);

// Destroy arena
void arena_destroy(ArenaAllocator *arena);

#endif // ARENA_ALLOCATOR_H
