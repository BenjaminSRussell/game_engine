/**
 * =================================================================================================
 *                          HIGH-PERFORMANCE MEMORY ARENA
 *                          Phase 10: Optimization
 * =================================================================================================
 *
 * PURPOSE: Linear memory allocator for fast, cache-friendly allocations
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARENAS 16

typedef struct {
  uint8_t *memory;
  size_t size;
  size_t used;
  size_t peak;
  const char *name;
} MemoryArena;

typedef struct {
  MemoryArena *arena;
  size_t used_mark;
} ArenaTemp;

// Global tracker
static MemoryArena *g_arenas[MAX_ARENAS] = {0};
static int g_arena_count = 0;

// -----------------------------------------------------------------------------
// Arena Management
// -----------------------------------------------------------------------------

MemoryArena *arena_create(size_t size, const char *name) {
  MemoryArena *arena = (MemoryArena *)calloc(1, sizeof(MemoryArena));
  if (!arena)
    return NULL;

  // Align size to 4KB
  size = (size + 4095) & ~4095;

  arena->memory = (uint8_t *)malloc(size);
  if (!arena->memory) {
    free(arena);
    return NULL;
  }

  arena->size = size;
  arena->used = 0;
  arena->peak = 0;
  arena->name = name;

  if (g_arena_count < MAX_ARENAS) {
    g_arenas[g_arena_count++] = arena;
  }

  return arena;
}

void arena_destroy(MemoryArena *arena) {
  if (!arena)
    return;
  free(arena->memory);
  free(arena);

  // Remove from global list (linear scan, rare op)
  for (int i = 0; i < g_arena_count; i++) {
    if (g_arenas[i] == arena) {
      g_arenas[i] = g_arenas[--g_arena_count];
      break;
    }
  }
}

// -----------------------------------------------------------------------------
// Allocation
// -----------------------------------------------------------------------------

void *arena_alloc(MemoryArena *arena, size_t size, size_t alignment) {
  if (!arena)
    return NULL;

  // Default alignment
  if (alignment == 0)
    alignment = 8;

  // Calculate aligned pointer
  uintptr_t current_ptr = (uintptr_t)arena->memory + arena->used;
  uintptr_t aligned_ptr = (current_ptr + alignment - 1) & ~(alignment - 1);
  size_t padding = aligned_ptr - current_ptr;

  size_t total_size = size + padding;

  if (arena->used + total_size > arena->size) {
    // Out of memory
    return NULL;
  }

  arena->used += total_size;
  if (arena->used > arena->peak)
    arena->peak = arena->used;

  return (void *)aligned_ptr;
}

void *arena_alloc_zero(MemoryArena *arena, size_t size, size_t alignment) {
  void *ptr = arena_alloc(arena, size, alignment);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

void arena_reset(MemoryArena *arena) {
  if (arena)
    arena->used = 0;
}

// -----------------------------------------------------------------------------
// Temporary Memory (Scoped)
// -----------------------------------------------------------------------------

ArenaTemp arena_temp_begin(MemoryArena *arena) {
  ArenaTemp temp = {0};
  if (arena) {
    temp.arena = arena;
    temp.used_mark = arena->used;
  }
  return temp;
}

void arena_temp_end(ArenaTemp temp) {
  if (temp.arena) {
    if (temp.used_mark < temp.arena->used) {
      temp.arena->used = temp.used_mark;
    }
  }
}

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

size_t arena_get_used(MemoryArena *arena) { return arena ? arena->used : 0; }
size_t arena_get_remaining(MemoryArena *arena) {
  return arena ? arena->size - arena->used : 0;
}
float arena_get_usage_ratio(MemoryArena *arena) {
  return arena ? (float)arena->used / (float)arena->size : 0.0f;
}
