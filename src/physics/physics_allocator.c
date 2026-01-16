#include "physics/physics_allocator.h"
#include "core/memory/memory_macros.h"
#include "engine/include/core/logger.h"
#include <string.h>

// Global physics allocator instance
PhysicsAllocator *g_physics_allocator = NULL;

// Object sizes (estimated, will be matched to actual structs in Phase 6.2)
#define RIGIDBODY_SIZE 256
#define COLLIDER_SIZE 128
#define CONSTRAINT_SIZE 192
#define MATERIAL_SIZE 64

void physics_allocator_init(void) {
  if (g_physics_allocator) {
    LOG_WARNING("[Physics] Allocator already initialized");
    return;
  }

  g_physics_allocator = ALLOC_TYPE_ZERO(PhysicsAllocator);

  // Create frame arena (8MB, reset each frame/step)
  // Smaller than render arena, but sufficient for contacts/manifold generation
  g_physics_allocator->frame_arena =
      unified_memory_arena_create(8 * 1024 * 1024);
  if (!g_physics_allocator->frame_arena) {
    LOG_ERROR("[Physics] Failed to create frame arena");
    FREE(g_physics_allocator);
    g_physics_allocator = NULL;
    return;
  }

  // Create rigidbody pool (5,000 bodies)
  MemoryPoolConfig rb_config = {
      .block_size = RIGIDBODY_SIZE, .block_count = 5000, .auto_expand = true};
  g_physics_allocator->rigidbody_pool = unified_memory_pool_create(&rb_config);
  if (!g_physics_allocator->rigidbody_pool) {
    LOG_ERROR("[Physics] Failed to create rigidbody pool");
    unified_memory_arena_destroy(g_physics_allocator->frame_arena);
    FREE(g_physics_allocator);
    g_physics_allocator = NULL;
    return;
  }

  // Create collider pool (10,000 colliders)
  MemoryPoolConfig col_config = {
      .block_size = COLLIDER_SIZE, .block_count = 10000, .auto_expand = true};
  g_physics_allocator->collider_pool = unified_memory_pool_create(&col_config);

  // Create constraint pool (2,000 constraints)
  MemoryPoolConfig joint_config = {
      .block_size = CONSTRAINT_SIZE, .block_count = 2000, .auto_expand = true};
  g_physics_allocator->constraint_pool =
      unified_memory_pool_create(&joint_config);

  // Create material pool (1,000 materials)
  MemoryPoolConfig mat_config = {
      .block_size = MATERIAL_SIZE, .block_count = 1000, .auto_expand = true};
  g_physics_allocator->material_pool = unified_memory_pool_create(&mat_config);

  // Initialize statistics
  g_physics_allocator->frame_arena_peak_usage = 0;
  g_physics_allocator->rigidbodies_allocated = 0;
  g_physics_allocator->colliders_allocated = 0;
  g_physics_allocator->constraints_allocated = 0;

  LOG_INFO("[Physics] Allocator initialized");
  LOG_INFO("  Frame arena: 8 MB");
  LOG_INFO("  Rigidbody pool: 5,000 bodies");
  LOG_INFO("  Collider pool: 10,000 colliders");
}

void physics_allocator_shutdown(void) {
  if (!g_physics_allocator) {
    return;
  }

  physics_allocator_print_stats();

  if (g_physics_allocator->material_pool)
    unified_memory_pool_destroy(g_physics_allocator->material_pool);
  if (g_physics_allocator->constraint_pool)
    unified_memory_pool_destroy(g_physics_allocator->constraint_pool);
  if (g_physics_allocator->collider_pool)
    unified_memory_pool_destroy(g_physics_allocator->collider_pool);
  if (g_physics_allocator->rigidbody_pool)
    unified_memory_pool_destroy(g_physics_allocator->rigidbody_pool);
  if (g_physics_allocator->frame_arena)
    unified_memory_arena_destroy(g_physics_allocator->frame_arena);

  FREE(g_physics_allocator);
  g_physics_allocator = NULL;

  LOG_INFO("[Physics] Allocator shutdown");
}

void physics_allocator_reset_frame(void) {
  if (!g_physics_allocator || !g_physics_allocator->frame_arena)
    return;

  size_t current =
      unified_memory_arena_get_usage(g_physics_allocator->frame_arena);
  if (current > g_physics_allocator->frame_arena_peak_usage) {
    g_physics_allocator->frame_arena_peak_usage = current;
  }

  unified_memory_arena_reset(g_physics_allocator->frame_arena);
}

PhysicsAllocator *physics_allocator_get(void) { return g_physics_allocator; }

void *physics_alloc_frame(size_t size) {
  if (!g_physics_allocator || !g_physics_allocator->frame_arena)
    return NULL;
  return unified_memory_arena_alloc(g_physics_allocator->frame_arena, size);
}

void *physics_alloc_rigidbody(void) {
  if (!g_physics_allocator || !g_physics_allocator->rigidbody_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_physics_allocator->rigidbody_pool,
                                        RIGIDBODY_SIZE);
  if (ptr)
    g_physics_allocator->rigidbodies_allocated++;
  return ptr;
}

void physics_free_rigidbody(void *body) {
  if (!g_physics_allocator || !g_physics_allocator->rigidbody_pool || !body)
    return;
  unified_memory_pool_free(g_physics_allocator->rigidbody_pool, body);
  g_physics_allocator->rigidbodies_allocated--;
}

void *physics_alloc_collider(void) {
  if (!g_physics_allocator || !g_physics_allocator->collider_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_physics_allocator->collider_pool,
                                        COLLIDER_SIZE);
  if (ptr)
    g_physics_allocator->colliders_allocated++;
  return ptr;
}

void physics_free_collider(void *collider) {
  if (!g_physics_allocator || !g_physics_allocator->collider_pool || !collider)
    return;
  unified_memory_pool_free(g_physics_allocator->collider_pool, collider);
  g_physics_allocator->colliders_allocated--;
}

void *physics_alloc_constraint(void) {
  if (!g_physics_allocator || !g_physics_allocator->constraint_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_physics_allocator->constraint_pool,
                                        CONSTRAINT_SIZE);
  if (ptr)
    g_physics_allocator->constraints_allocated++;
  return ptr;
}

void physics_free_constraint(void *constraint) {
  if (!g_physics_allocator || !g_physics_allocator->constraint_pool ||
      !constraint)
    return;
  unified_memory_pool_free(g_physics_allocator->constraint_pool, constraint);
  g_physics_allocator->constraints_allocated--;
}

void physics_allocator_print_stats(void) {
  if (!g_physics_allocator) {
    printf("[Physics] Allocator not initialized\n");
    return;
  }

  printf("\n=== Physics Allocator Statistics ===\n");
  printf("Frame Arena:\n");
  printf("  Peak usage: %.2f MB / 8 MB\n",
         g_physics_allocator->frame_arena_peak_usage / (1024.0 * 1024.0));

  printf("\nPools:\n");
  printf("  Rigidbodies: %u allocated\n",
         g_physics_allocator->rigidbodies_allocated);
  printf("  Colliders:   %u allocated\n",
         g_physics_allocator->colliders_allocated);
  printf("  Constraints: %u allocated\n",
         g_physics_allocator->constraints_allocated);
  printf("====================================\n\n");
}
