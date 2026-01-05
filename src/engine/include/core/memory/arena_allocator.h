#ifndef CORE_MEMORY_ARENA_ALLOCATOR_H
#define CORE_MEMORY_ARENA_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Arena Arena;

typedef struct ArenaCheckpoint {
  void *block;
  size_t offset;
} ArenaCheckpoint;

Arena *arena_init(size_t size, bool growable);
void arena_destroy(Arena *arena);

void *arena_alloc(Arena *arena, size_t size, size_t alignment);
void arena_reset(Arena *arena);

ArenaCheckpoint arena_checkpoint(Arena *arena);
void arena_rewind(Arena *arena, ArenaCheckpoint checkpoint);

Arena *arena_thread_local_get(void);
void arena_thread_local_set(Arena *arena);

double arena_benchmark(size_t iterations, size_t alloc_size);

#ifdef __cplusplus
}
#endif

#endif
