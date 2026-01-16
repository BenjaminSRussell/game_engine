#include "rendering/render_allocator.h"
#include "core/memory/memory_macros.h"
#include "engine/include/core/logger.h"
#include <string.h>

// Global render allocator instance
RenderAllocator *g_render_allocator = NULL;

// Draw call size (estimated)
#define DRAW_CALL_SIZE 128

// Mesh size (estimated)
#define MESH_SIZE 256

// Material size (estimated)
#define MATERIAL_SIZE 128

void render_allocator_init(void) {
  if (g_render_allocator) {
    LOG_WARNING("[Render] Allocator already initialized");
    return;
  }

  g_render_allocator = ALLOC_TYPE_ZERO(RenderAllocator);

  // Create frame arena (16MB, reset each frame)
  g_render_allocator->frame_arena =
      unified_memory_arena_create(16 * 1024 * 1024);
  if (!g_render_allocator->frame_arena) {
    LOG_ERROR("[Render] Failed to create frame arena");
    FREE(g_render_allocator);
    g_render_allocator = NULL;
    return;
  }

  // Create draw call pool (10K draw calls)
  MemoryPoolConfig draw_call_config = {
      .block_size = DRAW_CALL_SIZE, .block_count = 10000, .auto_expand = true};
  g_render_allocator->draw_call_pool =
      unified_memory_pool_create(&draw_call_config);
  if (!g_render_allocator->draw_call_pool) {
    LOG_ERROR("[Render] Failed to create draw call pool");
    unified_memory_arena_destroy(g_render_allocator->frame_arena);
    FREE(g_render_allocator);
    g_render_allocator = NULL;
    return;
  }

  // Create mesh pool (5K meshes)
  MemoryPoolConfig mesh_config = {
      .block_size = MESH_SIZE, .block_count = 5000, .auto_expand = true};
  g_render_allocator->mesh_pool = unified_memory_pool_create(&mesh_config);
  if (!g_render_allocator->mesh_pool) {
    LOG_ERROR("[Render] Failed to create mesh pool");
    unified_memory_pool_destroy(g_render_allocator->draw_call_pool);
    unified_memory_arena_destroy(g_render_allocator->frame_arena);
    FREE(g_render_allocator);
    g_render_allocator = NULL;
    return;
  }

  // Create material pool (2K materials)
  MemoryPoolConfig material_config = {
      .block_size = MATERIAL_SIZE, .block_count = 2000, .auto_expand = true};
  g_render_allocator->material_pool =
      unified_memory_pool_create(&material_config);
  if (!g_render_allocator->material_pool) {
    LOG_ERROR("[Render] Failed to create material pool");
    unified_memory_pool_destroy(g_render_allocator->mesh_pool);
    unified_memory_pool_destroy(g_render_allocator->draw_call_pool);
    unified_memory_arena_destroy(g_render_allocator->frame_arena);
    FREE(g_render_allocator);
    g_render_allocator = NULL;
    return;
  }

  // Initialize statistics
  g_render_allocator->frame_arena_peak_usage = 0;
  g_render_allocator->draw_calls_allocated = 0;
  g_render_allocator->meshes_allocated = 0;
  g_render_allocator->materials_allocated = 0;

  LOG_INFO("[Render] Allocator initialized");
  LOG_INFO("  Frame arena: 16 MB");
  LOG_INFO("  Draw call pool: 10,000 calls");
  LOG_INFO("  Mesh pool: 5,000 meshes");
  LOG_INFO("  Material pool: 2,000 materials");
}

void render_allocator_shutdown(void) {
  if (!g_render_allocator) {
    return;
  }

  // Print final statistics
  render_allocator_print_stats();

  // Destroy pools and arena
  if (g_render_allocator->material_pool) {
    unified_memory_pool_destroy(g_render_allocator->material_pool);
  }
  if (g_render_allocator->mesh_pool) {
    unified_memory_pool_destroy(g_render_allocator->mesh_pool);
  }
  if (g_render_allocator->draw_call_pool) {
    unified_memory_pool_destroy(g_render_allocator->draw_call_pool);
  }
  if (g_render_allocator->frame_arena) {
    unified_memory_arena_destroy(g_render_allocator->frame_arena);
  }

  FREE(g_render_allocator);
  g_render_allocator = NULL;

  LOG_INFO("[Render] Allocator shutdown");
}

void render_allocator_reset_frame(void) {
  if (!g_render_allocator || !g_render_allocator->frame_arena) {
    return;
  }

  // Track peak usage
  size_t current_usage =
      unified_memory_arena_get_usage(g_render_allocator->frame_arena);
  if (current_usage > g_render_allocator->frame_arena_peak_usage) {
    g_render_allocator->frame_arena_peak_usage = current_usage;
  }

  // Reset arena (fast!)
  unified_memory_arena_reset(g_render_allocator->frame_arena);
}

RenderAllocator *render_allocator_get(void) { return g_render_allocator; }

void *render_alloc_frame(size_t size) {
  if (!g_render_allocator || !g_render_allocator->frame_arena) {
    LOG_ERROR("[Render] Allocator not initialized");
    return NULL;
  }

  return unified_memory_arena_alloc(g_render_allocator->frame_arena, size);
}

void *render_alloc_draw_call(void) {
  if (!g_render_allocator || !g_render_allocator->draw_call_pool) {
    LOG_ERROR("[Render] Allocator not initialized");
    return NULL;
  }

  void *ptr = unified_memory_pool_alloc(g_render_allocator->draw_call_pool,
                                        DRAW_CALL_SIZE);
  if (ptr) {
    g_render_allocator->draw_calls_allocated++;
  }
  return ptr;
}

void render_free_draw_call(void *draw_call) {
  if (!g_render_allocator || !g_render_allocator->draw_call_pool ||
      !draw_call) {
    return;
  }

  unified_memory_pool_free(g_render_allocator->draw_call_pool, draw_call);
  g_render_allocator->draw_calls_allocated--;
}

void *render_alloc_mesh(void) {
  if (!g_render_allocator || !g_render_allocator->mesh_pool) {
    LOG_ERROR("[Render] Allocator not initialized");
    return NULL;
  }

  void *ptr =
      unified_memory_pool_alloc(g_render_allocator->mesh_pool, MESH_SIZE);
  if (ptr) {
    g_render_allocator->meshes_allocated++;
  }
  return ptr;
}

void render_free_mesh(void *mesh) {
  if (!g_render_allocator || !g_render_allocator->mesh_pool || !mesh) {
    return;
  }

  unified_memory_pool_free(g_render_allocator->mesh_pool, mesh);
  g_render_allocator->meshes_allocated--;
}

void render_allocator_print_stats(void) {
  if (!g_render_allocator) {
    printf("[Render] Allocator not initialized\n");
    return;
  }

  printf("\n=== Render Allocator Statistics ===\n");
  printf("Frame Arena:\n");
  printf("  Peak usage: %.2f MB / 16 MB\n",
         g_render_allocator->frame_arena_peak_usage / (1024.0 * 1024.0));
  printf(
      "  Utilization: %.1f%%\n",
      (g_render_allocator->frame_arena_peak_usage / (16.0 * 1024.0 * 1024.0)) *
          100.0);

  printf("\nPools:\n");
  printf("  Draw calls: %u allocated\n",
         g_render_allocator->draw_calls_allocated);
  printf("  Meshes: %u allocated\n", g_render_allocator->meshes_allocated);
  printf("  Materials: %u allocated\n",
         g_render_allocator->materials_allocated);
  printf("===================================\n\n");
}
