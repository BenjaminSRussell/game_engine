/*
 * layout_invalidation.h
 * Layout Invalidation and Caching System
 * Efficient layout update tracking and multi-level caching
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LAYOUT_INVALIDATION_H
#define UI_LAYOUT_INVALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES AND ENUMERATIONS
 * ============================================================================ */

/* Invalidation flags indicating what changed */
typedef enum {
    INVALIDATE_LAYOUT = 1 << 0,      /* Layout values changed (position, size) */
    INVALIDATE_SIZE = 1 << 1,        /* Size constraints changed */
    INVALIDATE_POSITION = 1 << 2,    /* Position changed */
    INVALIDATE_VISIBILITY = 1 << 3,  /* Visibility changed */
    INVALIDATE_CONSTRAINTS = 1 << 4, /* Constraints changed */
    INVALIDATE_SPACING = 1 << 5,     /* Margin/padding changed */
    INVALIDATE_CHILDREN = 1 << 6,    /* Child hierarchy changed */
    INVALIDATE_ALL = 0xFF            /* All flags set */
} InvalidationFlag;

/* Cache levels for multi-level caching strategy */
typedef enum {
    CACHE_LEVEL_NONE = 0,           /* No caching */
    CACHE_LEVEL_ELEMENT = 1,        /* Cache individual element layout */
    CACHE_LEVEL_SUBTREE = 2,        /* Cache subtree layout */
    CACHE_LEVEL_FULL = 3            /* Full layout pipeline caching */
} CacheLevel;

/* Forward declarations */
typedef struct InvalidationEntry InvalidationEntry;
typedef struct InvalidationQueue InvalidationQueue;
typedef struct LayoutCacheEntry LayoutCacheEntry;
typedef struct LayoutCacheLevel LayoutCacheLevel;
typedef struct LayoutInvalidationSystem LayoutInvalidationSystem;

/* ============================================================================
 * INVALIDATION ENTRY
 * ============================================================================ */

/* Tracks what was invalidated and why */
typedef struct InvalidationEntry {
    uint32_t element_id;
    uint32_t timestamp;
    uint32_t invalidation_flags;
    const char* reason;              /* Debug information */
} InvalidationEntry;

/* ============================================================================
 * INVALIDATION QUEUE
 * ============================================================================ */

/* Queue of pending invalidations to process */
typedef struct InvalidationQueue {
    InvalidationEntry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
    uint32_t read_index;
} InvalidationQueue;

/* ============================================================================
 * LAYOUT CACHE ENTRY
 * ============================================================================ */

/* Single cached layout result */
typedef struct LayoutCacheEntry {
    uint32_t element_id;
    uint32_t cache_generation;       /* Generation when cached */
    uint32_t element_generation;     /* Element generation when cached */

    /* Cached values */
    float x;
    float y;
    float width;
    float height;

    /* Cache metadata */
    bool is_valid;
    uint32_t last_access_timestamp;
    uint32_t access_count;
} LayoutCacheEntry;

/* ============================================================================
 * CACHE LEVEL
 * ============================================================================ */

/* One level of the multi-level cache hierarchy */
typedef struct LayoutCacheLevel {
    CacheLevel level;
    LayoutCacheEntry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
    uint32_t hit_count;
    uint32_t miss_count;
    float hit_rate;                  /* Cache hit rate (0.0 - 1.0) */
} LayoutCacheLevel;

/* ============================================================================
 * INVALIDATION SYSTEM
 * ============================================================================ */

/* Main invalidation tracking and caching system */
typedef struct LayoutInvalidationSystem {
    /* Element tracking */
    uint32_t* element_ids;
    uint32_t* element_generations;
    uint32_t* element_last_layout_time;
    uint32_t element_count;
    uint32_t element_capacity;

    /* Invalidation queue */
    InvalidationQueue invalidation_queue;

    /* Multi-level caching */
    LayoutCacheLevel cache_levels[CACHE_LEVEL_FULL + 1];

    /* Global state */
    uint32_t current_generation;
    uint32_t current_timestamp;
    uint32_t frame_counter;

    /* Performance tracking */
    uint32_t total_invalidations;
    uint32_t total_layout_solves;
    float total_solve_time_ms;
    uint32_t cache_evictions;

    /* Configuration */
    bool enable_aggressive_caching;  /* Cache more aggressively */
    bool enable_generation_tracking; /* Track element generations */
    uint32_t max_queue_size;
    uint32_t max_cache_entries;
    float cache_eviction_threshold;  /* When to evict old entries (0.0-1.0) */
} LayoutInvalidationSystem;

/* ============================================================================
 * API - SYSTEM MANAGEMENT
 * ============================================================================ */

/* Create and destroy invalidation system */
LayoutInvalidationSystem* layout_invalidation_create(void);
void layout_invalidation_destroy(LayoutInvalidationSystem* system);

/* Configuration */
void layout_invalidation_set_cache_mode(LayoutInvalidationSystem* system,
                                       CacheLevel level);
void layout_invalidation_enable_aggressive_caching(LayoutInvalidationSystem* system,
                                                  bool enable);
void layout_invalidation_set_max_cache_entries(LayoutInvalidationSystem* system,
                                              uint32_t max_entries);

/* ============================================================================
 * API - ELEMENT TRACKING
 * ============================================================================ */

/* Register element for tracking */
void layout_invalidation_register_element(LayoutInvalidationSystem* system,
                                         uint32_t element_id);

/* Unregister element */
void layout_invalidation_unregister_element(LayoutInvalidationSystem* system,
                                           uint32_t element_id);

/* Get element generation */
uint32_t layout_invalidation_get_element_generation(const LayoutInvalidationSystem* system,
                                                   uint32_t element_id);

/* Increment element generation when it changes */
void layout_invalidation_increment_generation(LayoutInvalidationSystem* system,
                                             uint32_t element_id);

/* ============================================================================
 * API - INVALIDATION
 * ============================================================================ */

/* Invalidate element with specific flags */
void layout_invalidation_invalidate(LayoutInvalidationSystem* system,
                                   uint32_t element_id,
                                   uint32_t invalidation_flags,
                                   const char* reason);

/* Invalidate element and all children */
void layout_invalidation_invalidate_subtree(LayoutInvalidationSystem* system,
                                           uint32_t element_id,
                                           uint32_t invalidation_flags);

/* Invalidate all elements */
void layout_invalidation_invalidate_all(LayoutInvalidationSystem* system);

/* Check if element is marked as dirty */
bool layout_invalidation_is_dirty(const LayoutInvalidationSystem* system,
                                 uint32_t element_id);

/* Check specific invalidation flag */
bool layout_invalidation_has_flag(const LayoutInvalidationSystem* system,
                                 uint32_t element_id,
                                 InvalidationFlag flag);

/* ============================================================================
 * API - QUEUE PROCESSING
 * ============================================================================ */

/* Get next pending invalidation from queue */
bool layout_invalidation_dequeue(LayoutInvalidationSystem* system,
                                InvalidationEntry* out_entry);

/* Peek at next invalidation without removing it */
bool layout_invalidation_peek_queue(const LayoutInvalidationSystem* system,
                                   InvalidationEntry* out_entry);

/* Get queue size */
uint32_t layout_invalidation_get_queue_size(const LayoutInvalidationSystem* system);

/* Clear invalidation queue */
void layout_invalidation_queue_clear(LayoutInvalidationSystem* system);

/* ============================================================================
 * API - CACHING
 * ============================================================================ */

/* Store layout in cache */
void layout_invalidation_cache_layout(LayoutInvalidationSystem* system,
                                     uint32_t element_id,
                                     float x, float y,
                                     float width, float height);

/* Retrieve cached layout */
bool layout_invalidation_get_cached_layout(const LayoutInvalidationSystem* system,
                                          uint32_t element_id,
                                          float* out_x, float* out_y,
                                          float* out_width, float* out_height);

/* Check if layout is in cache */
bool layout_invalidation_is_cached(const LayoutInvalidationSystem* system,
                                  uint32_t element_id);

/* Invalidate cache for element */
void layout_invalidation_cache_invalidate(LayoutInvalidationSystem* system,
                                         uint32_t element_id);

/* Invalidate entire cache */
void layout_invalidation_cache_clear_all(LayoutInvalidationSystem* system);

/* Clear specific cache level */
void layout_invalidation_cache_clear_level(LayoutInvalidationSystem* system,
                                          CacheLevel level);

/* ============================================================================
 * API - BATCH OPERATIONS
 * ============================================================================ */

/* Begin batch invalidation (suppress immediate updates) */
void layout_invalidation_begin_batch(LayoutInvalidationSystem* system);

/* End batch invalidation (process all queued invalidations) */
void layout_invalidation_end_batch(LayoutInvalidationSystem* system);

/* Batch add multiple elements */
void layout_invalidation_invalidate_batch(LayoutInvalidationSystem* system,
                                         const uint32_t* element_ids,
                                         uint32_t count,
                                         uint32_t invalidation_flags);

/* ============================================================================
 * API - PERFORMANCE MONITORING
 * ============================================================================ */

/* Get cache statistics */
void layout_invalidation_get_cache_stats(const LayoutInvalidationSystem* system,
                                        CacheLevel level,
                                        uint32_t* out_entries,
                                        uint32_t* out_hits,
                                        uint32_t* out_misses,
                                        float* out_hit_rate);

/* Get system performance metrics */
void layout_invalidation_get_performance_stats(const LayoutInvalidationSystem* system,
                                              uint32_t* out_total_invalidations,
                                              uint32_t* out_total_solves,
                                              float* out_avg_solve_time_ms,
                                              uint32_t* out_cache_evictions);

/* Get cache size */
uint32_t layout_invalidation_get_total_cache_entries(const LayoutInvalidationSystem* system);

/* ============================================================================
 * API - DEBUGGING
 * ============================================================================ */

/* Print system state */
void layout_invalidation_print_state(const LayoutInvalidationSystem* system);

/* Print cache statistics */
void layout_invalidation_print_cache_stats(const LayoutInvalidationSystem* system);

/* Print invalidation history (last N entries) */
void layout_invalidation_print_history(const LayoutInvalidationSystem* system,
                                      uint32_t last_n_entries);

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_INVALIDATION_H */
