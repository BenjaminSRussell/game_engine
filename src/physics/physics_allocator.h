#ifndef PHYSICS_ALLOCATOR_H
#define PHYSICS_ALLOCATOR_H

#include "core/memory/unified_memory_allocator.h"
#include "core/types.h"

/**
 * Physics Allocator - Memory management for physics subsystem
 *
 * Strategy:
 * - Frame arena: Reset each frame (collision contacts, temp solver data)
 * - Pools: Persistent storage for bodies, colliders, constraints
 *
 * Performance:
 * - O(1) allocation for physics objects
 * - Cache-friendly pool layout
 * - Automatic leak detection
 */

typedef struct {
  ArenaAllocator *frame_arena; // Reset each frame (contacts, islands)
  MemoryPool *rigidbody_pool;  // Persistent rigidbodies
  MemoryPool *collider_pool;   // Persistent colliders
  MemoryPool *constraint_pool; // Persistent constraints/joints
  MemoryPool *material_pool;   // Physics materials

  // Statistics
  u64 frame_arena_peak_usage;
  u32 rigidbodies_allocated;
  u32 colliders_allocated;
  u32 constraints_allocated;
} PhysicsAllocator;

// Global physics allocator
extern PhysicsAllocator *g_physics_allocator;

/**
 * Initialize physics allocator
 * Call once at physics startup
 */
void physics_allocator_init(void);

/**
 * Shutdown physics allocator
 * Call at physics shutdown
 */
void physics_allocator_shutdown(void);

/**
 * Reset frame arena
 * Call at the start of physics simulation step
 */
void physics_allocator_reset_frame(void);

/**
 * Get current physics allocator
 */
PhysicsAllocator *physics_allocator_get(void);

/**
 * Allocate from frame arena (temporary, reset each frame)
 */
void *physics_alloc_frame(size_t size);

/**
 * Allocate array from frame arena
 */
#define PHYSICS_ALLOC_FRAME_ARRAY(type, count)                                 \
  ((type *)physics_alloc_frame(sizeof(type) * (count)))

/**
 * Allocate rigidbody from pool
 */
void *physics_alloc_rigidbody(void);

/**
 * Free rigidbody to pool
 */
void physics_free_rigidbody(void *body);

/**
 * Allocate collider from pool
 */
void *physics_alloc_collider(void);

/**
 * Free collider to pool
 */
void physics_free_collider(void *collider);

/**
 * Allocate constraint from pool
 */
void *physics_alloc_constraint(void);

/**
 * Free constraint to pool
 */
void physics_free_constraint(void *constraint);

/**
 * Get allocator statistics
 */
void physics_allocator_print_stats(void);

#endif // PHYSICS_ALLOCATOR_H
