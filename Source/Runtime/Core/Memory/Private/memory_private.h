#ifndef MEMORY_PRIVATE_H
#define MEMORY_PRIVATE_H

#include "../Public/unified_memory.h"
#include <pthread.h>
#include <stdbool.h>

// Internal structures extracted from unified_memory.c

// Allocation metadata tracking
// Allocation metadata tracking
typedef struct unified_memory_allocation {
  void *ptr;
  size_t size;
  size_t actual_size;
  const char *file;
  int line;
  const char *function;
  const char *type;
  unified_memory_strategy_t strategy;
  unified_memory_flags_t flags;
  uint64_t timestamp;
  uint32_t allocation_id;
  struct unified_memory_allocation *next;
  struct unified_memory_allocation *prev;
} unified_memory_allocation_t;

// Profiling data - Defined in public header
// memory_profile_t

// Memory boundaries - Defined in public header
// memory_boundary_t

// Global state container
typedef struct {
  bool initialized;
  unified_memory_config_t config;
  unified_memory_stats_t stats;

  // Allocation tracking
  unified_memory_allocation_t *allocations;
  pthread_mutex_t allocations_mutex;
  uint32_t next_allocation_id;

  // Memory pools
  void *memory_pools;
  pthread_mutex_t pools_mutex;

  // Arena allocator
  void *arena_allocator;
  pthread_mutex_t arena_mutex;

  // Stack allocator
  void *stack_allocator;
  pthread_mutex_t stack_mutex;

  // Profiling
  memory_profile_t *profiles;
  pthread_mutex_t profiles_mutex;
  uint32_t profile_count;

  // Memory boundaries
  memory_boundary_t *active_boundary;
  pthread_mutex_t boundary_mutex;

  // Global mutex
  pthread_mutex_t global_mutex;
} unified_memory_state_t;

// Extern the global state so other private modules can access it
extern unified_memory_state_t g_unified_memory;

// Internal utility function decls
uint64_t get_timestamp_ms(void);

#endif // MEMORY_PRIVATE_H
