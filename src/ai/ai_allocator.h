#ifndef AI_ALLOCATOR_H
#define AI_ALLOCATOR_H

#include "core/memory/unified_memory_allocator.h"
#include "core/types.h"

/**
 * AI Allocator - Memory management for AI subsystem
 *
 * Strategy:
 * - Pathfinding Arena: Scratchpad for A* nodes (reset per request)
 * - Pools: Persistent agents, paths, states
 *
 * Threading:
 * - Pathfinding arena is thread-local or uses multiple arenas for parallel jobs
 * - Pools should be thread-safe
 */

typedef struct {
  ArenaAllocator *path_arena; // Temporary path nodes (A*)

  MemoryPool *agent_pool; // Agent logic data
  MemoryPool *path_pool;  // Stored waypoints
  MemoryPool *state_pool; // FSM/Behavior states

  // Statistics
  u64 path_arena_peak_usage;
  u32 agents_allocated;
  u32 paths_allocated;
} AIAllocator;

// Global AI allocator
extern AIAllocator *g_ai_allocator;

/**
 * Initialize AI allocator
 */
void ai_allocator_init(void);

/**
 * Shutdown AI allocator
 */
void ai_allocator_shutdown(void);

/**
 * Reset pathfinding arena
 * Call before starting a new pathfinding request
 */
void ai_allocator_reset_path_arena(void);

/**
 * Allocate temporary node for pathfinding
 */
void *ai_alloc_path_node(size_t size);

/**
 * Allocate agent
 */
void *ai_alloc_agent(void);

/**
 * Free agent
 */
void ai_free_agent(void *agent);

/**
 * Allocate persistent path (waypoints)
 */
void *ai_alloc_path(void);

/**
 * Free persistent path
 */
void ai_free_path(void *path);

/**
 * Get stats
 */
void ai_allocator_print_stats(void);

#endif // AI_ALLOCATOR_H
