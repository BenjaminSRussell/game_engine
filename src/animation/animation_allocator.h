#ifndef ANIMATION_ALLOCATOR_H
#define ANIMATION_ALLOCATOR_H

#include "core/memory/unified_memory_allocator.h"
#include "core/types.h"

/**
 * Animation Allocator - Memory management for animation subsystem
 *
 * Strategy:
 * - Blend Arena: Thread-local or frame-based scratchpad for blending poses
 * - Pools: Persistent storage for Skeletons (instances) and Clips (assets)
 *
 * Performance:
 * - O(1) allocation for skeleton instances
 * - Fast reset for blending buffers
 */

typedef struct {
  ArenaAllocator *blend_arena; // Temporary pose storage (reset per frame)
  MemoryPool *skeleton_pool;   // Skeleton instances (bones + matrices)
  MemoryPool *clip_pool;       // Animation Clips
  MemoryPool *pose_pool;       // Reusable poses

  // Statistics
  u64 blend_arena_peak_usage;
  u32 skeletons_allocated;
  u32 poses_allocated;
} AnimationAllocator;

// Global animation allocator
extern AnimationAllocator *g_animation_allocator;

/**
 * Initialize animation allocator
 */
void animation_allocator_init(void);

/**
 * Shutdown animation allocator
 */
void animation_allocator_shutdown(void);

/**
 * Reset blend arena
 * Call at start of animation update
 */
void animation_allocator_reset_blend_arena(void);

/**
 * Allocate temporary buffer for blending
 */
void *anim_alloc_blend_buffer(size_t size);

/**
 * Allocate skeleton instance
 */
void *anim_alloc_skeleton(void);

/**
 * Free skeleton instance
 */
void anim_free_skeleton(void *skeleton);

/**
 * Allocate reusable pose
 */
void *anim_alloc_pose(void);

/**
 * Free reusable pose
 */
void anim_free_pose(void *pose);

/**
 * Get stats
 */
void animation_allocator_print_stats(void);

#endif // ANIMATION_ALLOCATOR_H
