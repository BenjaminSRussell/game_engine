#pragma once

#include "include/core/types.h"
#include "math/vec3.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct WarmStartCache WarmStartCache;
typedef struct RigidBody RigidBody;

// Impulse cache entry
typedef struct {
    f32 normal_impulse;
    Vec3 friction_impulse;
    f32 bounce_impulse;
    u32 frame_id;
} ImpulseCache;

// Contact persistence data
typedef struct {
    u32 contact_id;
    f32 cached_normal_impulse;
    Vec3 cached_friction_impulse;
    u32 persistence_frames;
    bool is_valid;
} ContactPersistenceData;

// Warm starting cache
typedef struct WarmStartCache {
    ContactPersistenceData *contacts;
    u32 contact_count;
    u32 contact_capacity;
    u32 current_frame;
    bool enabled;
} WarmStartCache;

/**
 * WarmStarting: Reuse previous frame impulses for faster convergence
 *
 * Properties:
 *   - Impulse accumulation and reuse
 *   - Friction persistence tracking
 *   - Cache invalidation detection
 *   - Block solver support
 *   - Stacking test validation
 *
 * Benefits:
 *   - Faster constraint convergence
 *   - Better stacking stability
 *   - Reduced number of solver iterations
 *   - More stable contact behavior
 *
 * Use cases:
 *   - Physics simulation (warm starting)
 *   - Stacking and resting contacts
 *   - Iterative constraint solving
 */

// ============================================================================
// Cache Creation and Management
// ============================================================================

/**
 * Create warm starting cache
 *
 * Args:
 *   initial_capacity: Initial contact capacity
 *
 * Returns:
 *   New WarmStartCache (must be freed with warm_start_cache_free)
 */
WarmStartCache *warm_start_cache_create(u32 initial_capacity);

/**
 * Free warm starting cache
 *
 * Args:
 *   cache: WarmStartCache to free (NULL-safe)
 */
void warm_start_cache_free(WarmStartCache *cache);

/**
 * Enable/disable warm starting
 *
 * Args:
 *   cache: WarmStartCache
 *   enable: Whether to use warm starting
 */
void warm_start_cache_set_enabled(WarmStartCache *cache, bool enable);

/**
 * Check if warm starting is enabled
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   true if enabled
 */
bool warm_start_cache_is_enabled(WarmStartCache *cache);

/**
 * Clear all cached data
 *
 * Args:
 *   cache: WarmStartCache
 */
void warm_start_cache_clear(WarmStartCache *cache);

/**
 * Reset for new frame
 *
 * Args:
 *   cache: WarmStartCache
 */
void warm_start_cache_next_frame(WarmStartCache *cache);

// ============================================================================
// Impulse Storage and Retrieval
// ============================================================================

/**
 * Store impulse for contact
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Unique contact identifier
 *   normal_impulse: Normal impulse applied
 *   friction_impulse: Tangential impulse
 */
void warm_start_cache_store_impulse(WarmStartCache *cache, u32 contact_id, f32 normal_impulse, Vec3 friction_impulse);

/**
 * Retrieve cached impulse for contact
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to retrieve
 *   normal_impulse: Output normal impulse
 *   friction_impulse: Output friction impulse
 *
 * Returns:
 *   true if impulse found and valid
 */
bool warm_start_cache_get_impulse(WarmStartCache *cache, u32 contact_id, f32 *normal_impulse, Vec3 *friction_impulse);

/**
 * Get cached impulse with age information
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to retrieve
 *   data: Output contact persistence data
 *
 * Returns:
 *   true if found
 */
bool warm_start_cache_get_persistence_data(WarmStartCache *cache, u32 contact_id, ContactPersistenceData *data);

/**
 * Remove contact from cache
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to remove
 *
 * Returns:
 *   true if contact removed
 */
bool warm_start_cache_remove_contact(WarmStartCache *cache, u32 contact_id);

/**
 * Check if contact is in cache
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to check
 *
 * Returns:
 *   true if cached
 */
bool warm_start_cache_has_contact(WarmStartCache *cache, u32 contact_id);

// ============================================================================
// Friction Persistence
// ============================================================================

/**
 * Update friction persistence for contacts
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_ids: Array of active contact IDs
 *   count: Number of contacts
 *
 * Note: Marks old contacts as stale
 */
void warm_start_cache_update_friction_persistence(WarmStartCache *cache, u32 *contact_ids, u32 count);

/**
 * Get friction cache age (frames since last update)
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *
 * Returns:
 *   Frame age (0 = current frame)
 */
u32 warm_start_cache_get_friction_age(WarmStartCache *cache, u32 contact_id);

/**
 * Scale friction by age (decay old friction)
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *   decay_factor: Per-frame decay (e.g., 0.95)
 *
 * Returns:
 *   Scaled friction impulse
 */
Vec3 warm_start_cache_decay_friction(WarmStartCache *cache, u32 contact_id, f32 decay_factor);

/**
 * Invalidate friction for contact if moving apart
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *   relative_velocity: Relative velocity at contact
 *   threshold: Velocity threshold for invalidation
 *
 * Returns:
 *   true if friction was invalidated
 */
bool warm_start_cache_invalidate_friction(WarmStartCache *cache, u32 contact_id, f32 relative_velocity, f32 threshold);

// ============================================================================
// Cache Invalidation Detection
// ============================================================================

/**
 * Check if cache entry is stale
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to check
 *   max_age: Maximum allowed age in frames
 *
 * Returns:
 *   true if entry is too old
 */
bool warm_start_cache_is_stale(WarmStartCache *cache, u32 contact_id, u32 max_age);

/**
 * Invalidate stale entries
 *
 * Args:
 *   cache: WarmStartCache
 *   max_age: Maximum allowed age
 *
 * Returns:
 *   Number of entries invalidated
 */
u32 warm_start_cache_invalidate_stale(WarmStartCache *cache, u32 max_age);

/**
 * Detect if contact configuration changed significantly
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to check
 *   new_normal: Current contact normal
 *   normal_dot_threshold: Dot product threshold (e.g., 0.95 for small changes)
 *
 * Returns:
 *   true if normal direction changed significantly
 */
bool warm_start_cache_detect_change(WarmStartCache *cache, u32 contact_id, Vec3 new_normal, f32 normal_dot_threshold);

/**
 * Invalidate contact if normal changed
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *   new_normal: Current normal
 *   threshold: Change threshold
 *
 * Returns:
 *   true if invalidated
 */
bool warm_start_cache_invalidate_if_changed(WarmStartCache *cache, u32 contact_id, Vec3 new_normal, f32 threshold);

// ============================================================================
// Apply Warm Starting
// ============================================================================

/**
 * Apply cached impulse to bodies
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact to apply
 *   body_a: First body
 *   body_b: Second body
 *   position_a: Contact point on body A
 *   position_b: Contact point on body B
 *
 * Returns:
 *   true if impulse applied
 */
bool warm_start_apply_impulse(WarmStartCache *cache, u32 contact_id, RigidBody *body_a, RigidBody *body_b, Vec3 position_a, Vec3 position_b);

/**
 * Apply normal impulse only
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *   body_a: First body
 *   body_b: Second body
 *   normal: Contact normal
 *   position_a: Contact point on body A
 *   position_b: Contact point on body B
 *
 * Returns:
 *   true if applied
 */
bool warm_start_apply_normal(WarmStartCache *cache, u32 contact_id, RigidBody *body_a, RigidBody *body_b, Vec3 normal, Vec3 position_a, Vec3 position_b);

/**
 * Apply friction impulse only
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_id: Contact ID
 *   body_a: First body
 *   body_b: Second body
 *   position_a: Contact point on body A
 *   position_b: Contact point on body B
 *
 * Returns:
 *   true if applied
 */
bool warm_start_apply_friction(WarmStartCache *cache, u32 contact_id, RigidBody *body_a, RigidBody *body_b, Vec3 position_a, Vec3 position_b);

/**
 * Scale all cached impulses
 *
 * Args:
 *   cache: WarmStartCache
 *   scale: Scaling factor
 */
void warm_start_scale_impulses(WarmStartCache *cache, f32 scale);

// ============================================================================
// Block Solver Support
// ============================================================================

/**
 * Get block of contacts for batch processing
 *
 * Args:
 *   cache: WarmStartCache
 *   start_index: Starting index
 *   block_size: Desired block size
 *   contact_ids: Output array for contact IDs
 *   max_contacts: Maximum contacts to return
 *
 * Returns:
 *   Number of contacts in block
 */
u32 warm_start_cache_get_contact_block(WarmStartCache *cache, u32 start_index, u32 block_size, u32 *contact_ids, u32 max_contacts);

/**
 * Apply block of impulses at once
 *
 * Args:
 *   cache: WarmStartCache
 *   contact_ids: Array of contact IDs
 *   count: Number of contacts
 *   bodies: Array of body pairs
 *   body_count: Number of body pairs
 *
 * Note: Optimized for cache efficiency
 */
void warm_start_apply_block(WarmStartCache *cache, u32 *contact_ids, u32 count, void **bodies, u32 body_count);

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

/**
 * Get cache size
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   Number of cached contacts
 */
u32 warm_start_cache_size(WarmStartCache *cache);

/**
 * Get cache capacity
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   Maximum cached contacts before expansion
 */
u32 warm_start_cache_capacity(WarmStartCache *cache);

/**
 * Get memory usage
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   Total bytes allocated
 */
u64 warm_start_cache_memory_usage(WarmStartCache *cache);

/**
 * Print cache statistics
 *
 * Args:
 *   cache: WarmStartCache
 */
void warm_start_cache_print_stats(WarmStartCache *cache);

/**
 * Get effectiveness metric
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   Percentage of contacts with valid warm start (0-100)
 */
f32 warm_start_cache_effectiveness(WarmStartCache *cache);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Validate cache integrity
 *
 * Args:
 *   cache: WarmStartCache
 *
 * Returns:
 *   true if cache is valid
 */
bool warm_start_cache_validate(WarmStartCache *cache);

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 warm_start_run_tests(void);

/**
 * Stacking test with warm starting
 *
 * Args:
 *   num_blocks: Number of blocks to stack
 *   use_warm_start: Whether to enable warm starting
 *   time_limit_ms: Test duration limit
 *
 * Returns:
 *   true if stack remains stable
 */
bool warm_start_test_stacking(u32 num_blocks, bool use_warm_start, u32 time_limit_ms);

#ifdef __cplusplus
}
#endif

#endif // WARM_STARTING_H
