#include "animation/animation_allocator.h"
#include "core/memory/memory_macros.h"
#include "engine/include/core/logger.h"
#include <string.h>

AnimationAllocator *g_animation_allocator = NULL;

// Allocation sizes
#define SKELETON_SIZE                                                          \
  4096 // Large enough for bones + matrices (e.g., 64 bones * 64 bytes)
#define POSE_SIZE 2048 // Local transforms for 64 bones
#define CLIP_SIZE 1024 // Metadata for clip

void animation_allocator_init(void) {
  if (g_animation_allocator)
    return;

  g_animation_allocator = ALLOC_TYPE_ZERO(AnimationAllocator);

  // Blend Arena (8MB - sufficient for many concurrent blends)
  g_animation_allocator->blend_arena =
      unified_memory_arena_create(8 * 1024 * 1024);

  // Skeleton Pool (1000 max skeletons)
  MemoryPoolConfig skel_config = {
      .block_size = SKELETON_SIZE, .block_count = 1000, .auto_expand = true};
  g_animation_allocator->skeleton_pool =
      unified_memory_pool_create(&skel_config);

  // Pose Pool (2000 poses - double buffering)
  MemoryPoolConfig pose_config = {
      .block_size = POSE_SIZE, .block_count = 2000, .auto_expand = true};
  g_animation_allocator->pose_pool = unified_memory_pool_create(&pose_config);

  // Clip Pool (500 clips)
  MemoryPoolConfig clip_config = {
      .block_size = CLIP_SIZE, .block_count = 500, .auto_expand = true};
  g_animation_allocator->clip_pool = unified_memory_pool_create(&clip_config);

  LOG_INFO("[Animation] Allocator initialized");
}

void animation_allocator_shutdown(void) {
  if (!g_animation_allocator)
    return;

  animation_allocator_print_stats();

  if (g_animation_allocator->clip_pool)
    unified_memory_pool_destroy(g_animation_allocator->clip_pool);
  if (g_animation_allocator->pose_pool)
    unified_memory_pool_destroy(g_animation_allocator->pose_pool);
  if (g_animation_allocator->skeleton_pool)
    unified_memory_pool_destroy(g_animation_allocator->skeleton_pool);
  if (g_animation_allocator->blend_arena)
    unified_memory_arena_destroy(g_animation_allocator->blend_arena);

  FREE(g_animation_allocator);
  g_animation_allocator = NULL;

  LOG_INFO("[Animation] Allocator shutdown");
}

void animation_allocator_reset_blend_arena(void) {
  if (!g_animation_allocator || !g_animation_allocator->blend_arena)
    return;

  size_t current =
      unified_memory_arena_get_usage(g_animation_allocator->blend_arena);
  if (current > g_animation_allocator->blend_arena_peak_usage) {
    g_animation_allocator->blend_arena_peak_usage = current;
  }

  unified_memory_arena_reset(g_animation_allocator->blend_arena);
}

void *anim_alloc_blend_buffer(size_t size) {
  if (!g_animation_allocator || !g_animation_allocator->blend_arena)
    return NULL;
  return unified_memory_arena_alloc(g_animation_allocator->blend_arena, size);
}

void *anim_alloc_skeleton(void) {
  if (!g_animation_allocator || !g_animation_allocator->skeleton_pool)
    return NULL;
  void *ptr = unified_memory_pool_alloc(g_animation_allocator->skeleton_pool,
                                        SKELETON_SIZE);
  if (ptr)
    g_animation_allocator->skeletons_allocated++;
  return ptr;
}

void anim_free_skeleton(void *skeleton) {
  if (!g_animation_allocator || !g_animation_allocator->skeleton_pool ||
      !skeleton)
    return;
  unified_memory_pool_free(g_animation_allocator->skeleton_pool, skeleton);
  g_animation_allocator->skeletons_allocated--;
}

void *anim_alloc_pose(void) {
  if (!g_animation_allocator || !g_animation_allocator->pose_pool)
    return NULL;
  void *ptr =
      unified_memory_pool_alloc(g_animation_allocator->pose_pool, POSE_SIZE);
  if (ptr)
    g_animation_allocator->poses_allocated++;
  return ptr;
}

void anim_free_pose(void *pose) {
  if (!g_animation_allocator || !g_animation_allocator->pose_pool || !pose)
    return;
  unified_memory_pool_free(g_animation_allocator->pose_pool, pose);
  g_animation_allocator->poses_allocated--;
}

void animation_allocator_print_stats(void) {
  if (!g_animation_allocator)
    return;

  printf("\n=== Animation Allocator Statistics ===\n");
  printf("Blend Arena Peak: %.2f MB\n",
         g_animation_allocator->blend_arena_peak_usage / (1024.0f * 1024.0f));
  printf("Skeletons: %u active\n", g_animation_allocator->skeletons_allocated);
  printf("Poses: %u allocated\n", g_animation_allocator->poses_allocated);
  printf("======================================\n\n");
}
