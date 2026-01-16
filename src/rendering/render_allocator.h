#ifndef RENDER_ALLOCATOR_H
#define RENDER_ALLOCATOR_H

#include "core/memory/unified_memory_allocator.h"
#include "core/types.h"

/**
 * Render Allocator - Memory management for rendering subsystem
 *
 * Strategy:
 * - Frame arena: 16MB arena reset each frame (render commands, temp data)
 * - Draw call pool: Persistent pool for draw call structures
 * - Mesh pool: Pool for mesh data
 *
 * Performance:
 * - Zero allocations per frame after warmup
 * - Fast frame-to-frame reset
 * - Automatic leak detection
 */

typedef struct {
  ArenaAllocator *frame_arena; // Reset each frame
  MemoryPool *draw_call_pool;  // Persistent draw calls
  MemoryPool *mesh_pool;       // Persistent mesh data
  MemoryPool *material_pool;   // Persistent materials

  // Statistics
  u64 frame_arena_peak_usage;
  u32 draw_calls_allocated;
  u32 meshes_allocated;
  u32 materials_allocated;
} RenderAllocator;

// Global render allocator
extern RenderAllocator *g_render_allocator;

/**
 * Initialize render allocator
 * Call once at renderer startup
 */
void render_allocator_init(void);

/**
 * Shutdown render allocator
 * Call at renderer shutdown
 */
void render_allocator_shutdown(void);

/**
 * Reset frame arena
 * Call at the beginning of each frame
 */
void render_allocator_reset_frame(void);

/**
 * Get current render allocator
 */
RenderAllocator *render_allocator_get(void);

/**
 * Allocate from frame arena (temporary, reset each frame)
 */
void *render_alloc_frame(size_t size);

/**
 * Allocate array from frame arena
 */
#define RENDER_ALLOC_FRAME_ARRAY(type, count)                                  \
  ((type *)render_alloc_frame(sizeof(type) * (count)))

/**
 * Allocate draw call from pool
 */
void *render_alloc_draw_call(void);

/**
 * Free draw call to pool
 */
void render_free_draw_call(void *draw_call);

/**
 * Allocate mesh from pool
 */
void *render_alloc_mesh(void);

/**
 * Free mesh to pool
 */
void render_free_mesh(void *mesh);

/**
 * Get allocator statistics
 */
void render_allocator_print_stats(void);

#endif // RENDER_ALLOCATOR_H
