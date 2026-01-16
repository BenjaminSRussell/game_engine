#include "core/memory/memory_macros.h"
#include "core/memory/unified_memory_allocator.h"
#include <stdio.h>

// ============================================================================
// COMMON MEMORY POOLS
// ============================================================================

// Pre-created pools for common allocation sizes
static MemoryPool *g_small_pool = NULL;  // 64 bytes
static MemoryPool *g_medium_pool = NULL; // 256 bytes
static MemoryPool *g_large_pool = NULL;  // 1024 bytes
static MemoryPool *g_entity_pool = NULL; // For ECS entities (when implemented)

bool common_pools_init(void) {
  // Small allocations (64 bytes) - for small structs, strings
  MemoryPoolConfig small_config = {.block_size = 64,
                                   .block_count = 2048, // 128KB total
                                   .auto_expand = true};
  g_small_pool = unified_memory_pool_create(&small_config);
  if (!g_small_pool) {
    fprintf(stderr, "Failed to create small memory pool\n");
    return false;
  }

  // Medium allocations (256 bytes) - for medium structs
  MemoryPoolConfig medium_config = {.block_size = 256,
                                    .block_count = 1024, // 256KB total
                                    .auto_expand = true};
  g_medium_pool = unified_memory_pool_create(&medium_config);
  if (!g_medium_pool) {
    fprintf(stderr, "Failed to create medium memory pool\n");
    return false;
  }

  // Large allocations (1024 bytes) - for large structs, buffers
  MemoryPoolConfig large_config = {.block_size = 1024,
                                   .block_count = 512, // 512KB total
                                   .auto_expand = true};
  g_large_pool = unified_memory_pool_create(&large_config);
  if (!g_large_pool) {
    fprintf(stderr, "Failed to create large memory pool\n");
    return false;
  }

  printf("Common memory pools initialized:\n");
  printf("  Small pool: 64B x 2048 blocks\n");
  printf("  Medium pool: 256B x 1024 blocks\n");
  printf("  Large pool: 1024B x 512 blocks\n");

  return true;
}

void common_pools_shutdown(void) {
  if (g_small_pool) {
    unified_memory_pool_destroy(g_small_pool);
    g_small_pool = NULL;
  }
  if (g_medium_pool) {
    unified_memory_pool_destroy(g_medium_pool);
    g_medium_pool = NULL;
  }
  if (g_large_pool) {
    unified_memory_pool_destroy(g_large_pool);
    g_large_pool = NULL;
  }
  if (g_entity_pool) {
    unified_memory_pool_destroy(g_entity_pool);
    g_entity_pool = NULL;
  }
}

// Smart allocation that chooses the right pool based on size
void *common_pool_alloc(size_t size) {
  if (size <= 64 && g_small_pool) {
    return unified_memory_pool_alloc(g_small_pool, size, 0);
  }
  if (size <= 256 && g_medium_pool) {
    return unified_memory_pool_alloc(g_medium_pool, size, 0);
  }
  if (size <= 1024 && g_large_pool) {
    return unified_memory_pool_alloc(g_large_pool, size, 0);
  }

  // Fall back to general allocator for larger sizes
  return ALLOC(size);
}

void common_pool_free(void *ptr, size_t size) {
  if (!ptr)
    return;

  if (size <= 64 && g_small_pool) {
    unified_memory_pool_free(g_small_pool, ptr);
  } else if (size <= 256 && g_medium_pool) {
    unified_memory_pool_free(g_medium_pool, ptr);
  } else if (size <= 1024 && g_large_pool) {
    unified_memory_pool_free(g_large_pool, ptr);
  } else {
    FREE(ptr);
  }
}

// Get pool statistics
void common_pools_print_stats(void) {
  printf("\n=== Common Memory Pools Statistics ===\n");

  if (g_small_pool) {
    printf("Small Pool (64B):\n");
    printf("  Free blocks: %u / %u\n", g_small_pool->free_blocks,
           g_small_pool->total_blocks);
  }

  if (g_medium_pool) {
    printf("Medium Pool (256B):\n");
    printf("  Free blocks: %u / %u\n", g_medium_pool->free_blocks,
           g_medium_pool->total_blocks);
  }

  if (g_large_pool) {
    printf("Large Pool (1024B):\n");
    printf("  Free blocks: %u / %u\n", g_large_pool->free_blocks,
           g_large_pool->total_blocks);
  }

  printf("======================================\n\n");
}
