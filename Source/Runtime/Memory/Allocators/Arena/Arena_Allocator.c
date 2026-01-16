#include "../../Public/Memory.h"

// Define the struct internally or in a private header.
// Since we forward declared it in Memory.h, we define it here.
struct Arena_Allocator {
  void *buffer;
  u64 size;
  u64 offset;
};

Arena_Allocator *Arena_Create(u64 size) {
  Arena_Allocator *arena = (Arena_Allocator *)Memory_Allocate(
      sizeof(Arena_Allocator), MEMORY_TAG_ARENA);
  arena->buffer = Memory_Allocate(size, MEMORY_TAG_ARENA);
  arena->size = size;
  arena->offset = 0;
  return arena;
}

void *Arena_Allocate(Arena_Allocator *arena, u64 size) {
  if (!arena) {
    return 0;
  }

  // Check for overflow
  if (arena->offset + size > arena->size) {
    // Out of memory in arena
    return 0;
  }

  void *block = (u8 *)arena->buffer + arena->offset;
  arena->offset += size;
  return block;
}

void Arena_Reset(Arena_Allocator *arena) {
  if (arena) {
    arena->offset = 0;
  }
}

void Arena_Destroy(Arena_Allocator *arena) {
  if (arena) {
    Memory_Free(arena->buffer, arena->size, MEMORY_TAG_ARENA);
    Memory_Free(arena, sizeof(Arena_Allocator), MEMORY_TAG_ARENA);
  }
}
