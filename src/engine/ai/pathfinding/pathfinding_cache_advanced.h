// src/engine/ai/pathfinding/pathfinding_cache_advanced.h
//
// Purpose: Advanced pathfinding caching and optimization system header
// Provides hierarchical caching, path prediction, and dynamic optimization

#ifndef PATHFINDING_CACHE_ADVANCED_H
#define PATHFINDING_CACHE_ADVANCED_H

#include "core/math/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CACHE STATISTICS
// ============================================================================

typedef struct AdvancedCacheStats {
    u32 total_paths;
    u32 valid_paths;
    u32 total_regions;
    u32 active_predictions;
    u32 total_requests;
    u32 cache_hits;
    u32 cache_misses;
    u32 evictions;
    u32 predictions_made;
    u32 predictions_successful;
    
    f32 hit_rate;
    f32 average_path_length;
    f32 average_quality;
    f32 current_time;
} AdvancedCacheStats;

// ============================================================================
// ADVANCED CACHE MANAGEMENT
// ============================================================================

// Initialize advanced path cache system
bool pathfinding_cache_advanced_initialize(u32 max_paths, u32 max_regions);

// Shutdown advanced path cache system
void pathfinding_cache_advanced_shutdown(void);

// ============================================================================
// ADVANCED CACHE OPERATIONS
// ============================================================================

// Lookup path in advanced cache with quality scoring
// Returns true if found, outputs waypoints, count, and quality score (0-255)
bool pathfinding_cache_advanced_lookup(vec3 start, vec3 goal, vec3** out_waypoints, u32* out_count, u8* out_quality);

// Store path in advanced cache with computation time tracking
void pathfinding_cache_advanced_store(vec3 start, vec3 goal, const vec3* waypoints, u32 waypoint_count, f32 computation_time);

// ============================================================================
// PATH PREDICTION SYSTEM
// ============================================================================

// Predict likely path goal based on current position and behavior
// confidence: 0.0-1.0, higher means more confident in prediction
void pathfinding_cache_predict_path(vec3 current_position, vec3 likely_goal, f32 confidence);

// Get predicted path if available
// Returns true if prediction matches a cached path
bool pathfinding_cache_get_predicted_path(vec3 current_position, vec3** out_waypoints, u32* out_count);

// ============================================================================
// CACHE MAINTENANCE
// ============================================================================

// Update cache system (call once per frame)
void pathfinding_cache_advanced_update(f32 delta_time);

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

// Get comprehensive cache statistics
void pathfinding_cache_advanced_get_statistics(AdvancedCacheStats* out_stats);

// Print cache statistics to log
void pathfinding_cache_advanced_print_statistics(void);

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Optimize cache by removing low-quality paths
void pathfinding_cache_advanced_optimize(void);

// Precompute paths between high-traffic regions
void pathfinding_cache_advanced_precompute_regions(void);

// Export cache data for analysis
void pathfinding_cache_advanced_export_data(const char* filename);

// Import cache data from file
bool pathfinding_cache_advanced_import_data(const char* filename);

// ============================================================================
// PERFORMANCE TUNING
// ============================================================================

// Set cache tolerance for path matching
void pathfinding_cache_advanced_set_tolerance(f32 tolerance);

// Set maximum path age before eviction
void pathfinding_cache_advanced_set_max_age(f32 max_age_seconds);

// Set prediction confidence threshold
void pathfinding_cache_advanced_set_prediction_threshold(f32 threshold);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

// Quick cache lookup without quality
#define PATH_CACHE_ADVANCED_LOOKUP(start, goal, waypoints, count) \
    pathfinding_cache_advanced_lookup(start, goal, waypoints, count, NULL)

// Quick cache store without timing
#define PATH_CACHE_ADVANCED_STORE(start, goal, waypoints, count) \
    pathfinding_cache_advanced_store(start, goal, waypoints, count, 0.0f)

// Check if advanced cache is available
static inline bool pathfinding_cache_advanced_is_available(void) {
    extern void* g_advanced_cache;  // Forward declaration
    return g_advanced_cache != NULL;
}

#ifdef __cplusplus
}
#endif

#endif // PATHFINDING_CACHE_ADVANCED_H
