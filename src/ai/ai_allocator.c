#include "ai/ai_allocator.h"
#include "core/memory/memory_macros.h"
#include "engine/include/core/logger.h"
#include <string.h>

AIAllocator *g_ai_allocator = NULL;

// Allocation sizes
#define AGENT_SIZE 256
#define PATH_SIZE 1024 // Stores array of waypoints
#define STATE_SIZE 64

void ai_allocator_init(void) {
  if (g_ai_allocator)
    return;

  g_ai_allocator = ALLOC_TYPE_ZERO(AIAllocator);

  // Pathfinding Arena (4MB - sufficient for large A* expansions)
  g_ai_allocator->path_arena = unified_memory_arena_create(4 * 1024 * 1024);

  // Agent Pool (1000 max agents)
  MemoryPoolConfig agent_config = {
      .block_size = AGENT_SIZE, .block_count = 1000, .auto_expand = true};
  g_ai_allocator->agent_pool = unified_memory_pool_create(&agent_config);

  // Path Pool (500 paths)
  MemoryPoolConfig path_config = {
      .block_size = PATH_SIZE, .block_count = 500, .auto_expand = true};
  g_ai_allocator->path_pool = unified_memory_pool_create(&path_config);

  // State Pool (2000 states)
  MemoryPoolConfig state_config = {
      .block_size = STATE_SIZE, .block_count = 2000, .auto_expand = true};
  g_ai_allocator->state_pool = unified_memory_pool_create(&state_config);

  LOG_INFO("[AI] Allocator initialized");
}

void ai_allocator_shutdown(void) {
  if (!g_ai_allocator)
    return;

  ai_allocator_print_stats();

  if (g_ai_allocator->state_pool)
    unified_memory_pool_destroy(g_ai_allocator->state_pool);
  if (g_ai_allocator->path_pool)
    unified_memory_pool_destroy(g_ai_allocator->path_pool);
  if (g_ai_allocator->agent_pool)
    unified_memory_pool_destroy(g_ai_allocator->agent_pool);
  if (g_ai_allocator->path_arena)
    unified_memory_arena_destroy(g_ai_allocator->path_arena);

  FREE(g_ai_allocator);
  g_ai_allocator = NULL;

  LOG_INFO("[AI] Allocator shutdown");
}

void ai_allocator_reset_path_arena(void) {
  if (!g_ai_allocator || !g_ai_allocator->path_arena)
    return;

  size_t current = unified_memory_arena_get_usage(g_ai_allocator->path_arena);
  if (current > g_ai_allocator->path_arena_peak_usage) {
    g_ai_allocator->path_arena_peak_usage = current;
  }

  unified_memory_arena_reset(g_ai_allocator->path_arena);
}

void *ai_alloc_path_node(size_t size) {
  if (!g_ai_allocator || !g_ai_allocator->path_arena)
    return NULL;
  return unified_memory_arena_alloc(g_ai_allocator->path_arena, size);
}

void *ai_alloc_agent(void) {
  if (!g_ai_allocator || !g_ai_allocator->agent_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_ai_allocator->agent_pool, AGENT_SIZE);
  if (ptr)
    g_ai_allocator->agents_allocated++;
  return ptr;
}

void ai_free_agent(void *agent) {
  if (!g_ai_allocator || !g_ai_allocator->agent_pool || !agent)
    return;
  unified_memory_pool_free(g_ai_allocator->agent_pool, agent);
  g_ai_allocator->agents_allocated--;
}

void *ai_alloc_path(void) {
  if (!g_ai_allocator || !g_ai_allocator->path_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_ai_allocator->path_pool, PATH_SIZE);
  if (ptr)
    g_ai_allocator->paths_allocated++;
  return ptr;
}

void ai_free_path(void *path) {
  if (!g_ai_allocator || !g_ai_allocator->path_pool || !path)
    return;
  unified_memory_pool_free(g_ai_allocator->path_pool, path);
  g_ai_allocator->paths_allocated--;
}

void ai_allocator_print_stats(void) {
  if (!g_ai_allocator)
    return;

  printf("\n=== AI Allocator Statistics ===\n");
  printf("Path Arena Peak: %.2f MB\n",
         g_ai_allocator->path_arena_peak_usage / (1024.0f * 1024.0f));
  printf("Agents: %u active\n", g_ai_allocator->agents_allocated);
  printf("Paths: %u allocated\n", g_ai_allocator->paths_allocated);
  printf("===============================\n\n");
}
