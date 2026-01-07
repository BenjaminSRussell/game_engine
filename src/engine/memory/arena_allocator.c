#include "core/memory/arena_allocator.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ArenaBlock {
  uint8_t *memory;
  size_t size;
  size_t offset;
  struct ArenaBlock *next;
} ArenaBlock;

struct Arena {
  ArenaBlock *head;
  ArenaBlock *current;
  size_t default_size;
  bool growable;
};

static _Thread_local Arena *g_arena_tls = NULL;

static size_t align_up(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t mask = alignment - 1;
  return (value + mask) & ~mask;
}

static ArenaBlock *arena_block_create(size_t size) {
  ArenaBlock *block = (ArenaBlock *)calloc(1, sizeof(ArenaBlock));
  if (!block) {
    return NULL;
  }

  block->memory = (uint8_t *)malloc(size);
  if (!block->memory) {
    free(block);
    return NULL;
  }

  block->size = size;
  block->offset = 0;
  block->next = NULL;
  return block;
}

Arena *arena_init(size_t size, bool growable) {
  Arena *arena = (Arena *)calloc(1, sizeof(Arena));
  if (!arena) {
    return NULL;
  }

  if (size == 0) {
    size = 4096;
  }

  ArenaBlock *block = arena_block_create(size);
  if (!block) {
    free(arena);
    return NULL;
  }

  arena->head = block;
  arena->current = block;
  arena->default_size = size;
  arena->growable = growable;
  return arena;
}

void arena_destroy(Arena *arena) {
  if (!arena) {
    return;
  }

  ArenaBlock *block = arena->head;
  while (block) {
    ArenaBlock *next = block->next;
    free(block->memory);
    free(block);
    block = next;
  }

  free(arena);
}

void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
  if (!arena || size == 0) {
    return NULL;
  }

  if (alignment == 0) {
    alignment = 8u;
  }

  ArenaBlock *block = arena->current;
  if (!block) {
    return NULL;
  }

  size_t aligned_offset = align_up(block->offset, alignment);
  if (aligned_offset + size <= block->size) {
    void *ptr = block->memory + aligned_offset;
    block->offset = aligned_offset + size;
    return ptr;
  }

  if (!arena->growable) {
    return NULL;
  }

  size_t new_size = arena->default_size;
  if (new_size < size + alignment) {
    new_size = size + alignment;
  }

  ArenaBlock *new_block = arena_block_create(new_size);
  if (!new_block) {
    return NULL;
  }

  block->next = new_block;
  arena->current = new_block;

  aligned_offset = align_up(new_block->offset, alignment);
  void *ptr = new_block->memory + aligned_offset;
  new_block->offset = aligned_offset + size;
  return ptr;
}

void arena_reset(Arena *arena) {
  if (!arena || !arena->head) {
    return;
  }

  ArenaBlock *block = arena->head->next;
  while (block) {
    ArenaBlock *next = block->next;
    free(block->memory);
    free(block);
    block = next;
  }

  arena->head->next = NULL;
  arena->head->offset = 0;
  arena->current = arena->head;
}

ArenaCheckpoint arena_checkpoint(Arena *arena) {
  ArenaCheckpoint checkpoint;
  checkpoint.block = NULL;
  checkpoint.offset = 0;

  if (!arena || !arena->current) {
    return checkpoint;
  }

  checkpoint.block = arena->current;
  checkpoint.offset = arena->current->offset;
  return checkpoint;
}

void arena_rewind(Arena *arena, ArenaCheckpoint checkpoint) {
  if (!arena || !checkpoint.block) {
    return;
  }

  ArenaBlock *block = arena->head;
  ArenaBlock *prev = NULL;
  while (block && block != checkpoint.block) {
    prev = block;
    block = block->next;
  }

  if (!block) {
    return;
  }

  ArenaBlock *to_free = block->next;
  block->next = NULL;
  while (to_free) {
    ArenaBlock *next = to_free->next;
    free(to_free->memory);
    free(to_free);
    to_free = next;
  }

  block->offset = checkpoint.offset;
  arena->current = block;

  if (!prev && arena->head != block) {
    arena->head = block;
  }
}

Arena *arena_thread_local_get(void) { return g_arena_tls; }

void arena_thread_local_set(Arena *arena) { g_arena_tls = arena; }

double arena_benchmark(size_t iterations, size_t alloc_size) {
  if (iterations == 0 || alloc_size == 0) {
    return 0.0;
  }

  Arena *arena = arena_init(alloc_size * iterations, false);
  if (!arena) {
    return 0.0;
  }

  clock_t arena_start = clock();
  for (size_t i = 0; i < iterations; ++i) {
    if (!arena_alloc(arena, alloc_size, 8u)) {
      break;
    }
  }
  clock_t arena_end = clock();

  clock_t malloc_start = clock();
  for (size_t i = 0; i < iterations; ++i) {
    void *ptr = malloc(alloc_size);
    if (!ptr) {
      break;
    }
    memset(ptr, 0, alloc_size);
    free(ptr);
  }
  clock_t malloc_end = clock();

  arena_destroy(arena);

  double arena_time =
      (double)(arena_end - arena_start) / (double)CLOCKS_PER_SEC;
  double malloc_time =
      (double)(malloc_end - malloc_start) / (double)CLOCKS_PER_SEC;
  if (arena_time <= 0.0) {
    return 0.0;
  }
  return malloc_time / arena_time;
}
