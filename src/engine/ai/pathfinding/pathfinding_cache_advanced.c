// src/engine/ai/pathfinding/pathfinding_cache_advanced.c
//
// Purpose: Advanced pathfinding caching and optimization system
// Provides hierarchical caching, path prediction, and dynamic optimization

#include <ai/pathfinding/pathfinding_cache_advanced.h>
#include <core/memory.h>
#include <core/logging/unified_logger.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

#define ADVANCED_CACHE_MAX_REGIONS 64
#define ADVANCED_CACHE_MAX_PATHS_PER_REGION 128
#define ADVANCED_CACHE_MAX_WAYPOINTS 1024
#define ADVANCED_CACHE_TOLERANCE 0.25f

// Alignment helper
#ifndef alignof
#define alignof(type) __alignof__(type)
#endif
#define ADVANCED_CACHE_MAX_AGE 30.0f  // seconds
#define ADVANCED_CACHE_PREDICTION_FRAMES 10

// Helper function for clamping values
static inline f32 clamp(f32 value, f32 min, f32 max) {
    return value < min ? min : (value > max ? max : value);
}

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct PathRegion {
    vec3 center;
    f32 radius;
    u32 path_count;
    u32 path_capacity;
    u32* path_indices;
    f32 last_access_time;
} PathRegion;

typedef struct AdvancedCachedPath {
    vec3 start;
    vec3 goal;
    vec3 waypoints[ADVANCED_CACHE_MAX_WAYPOINTS];
    u32 waypoint_count;
    f32 path_length;
    f32 heuristic_cost;
    u32 region_start;
    u32 region_goal;
    f32 creation_time;
    f32 computation_time;
    f32 last_access_time;
    u32 access_count;
    u32 prediction_success_count;
    bool is_valid;
    u8 quality_score;  // 0-255, higher is better
} AdvancedCachedPath;

typedef struct PathPrediction {
    vec3 predicted_goal;
    vec3 current_position;
    f32 confidence;
    u32 frames_since_prediction;
    u32 target_path_index;
} PathPrediction;

typedef struct AdvancedPathCache {
    AdvancedCachedPath paths[ADVANCED_CACHE_MAX_PATHS_PER_REGION * ADVANCED_CACHE_MAX_REGIONS];
    PathRegion regions[ADVANCED_CACHE_MAX_REGIONS];
    PathPrediction predictions[ADVANCED_CACHE_MAX_PATHS_PER_REGION];
    
    u32 path_count;
    u32 region_count;
    u32 prediction_count;
    
    f32 current_time;
    u32 total_requests;
    u32 cache_hits;
    u32 cache_misses;
    
    // Statistics
    f32 average_path_length;
    f32 average_computation_time;
    u32 evictions;
    u32 predictions_made;
    u32 predictions_successful;
} AdvancedPathCache;

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static AdvancedPathCache* g_advanced_cache = NULL;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static f32 vec3_distance_squared(vec3 a, vec3 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    f32 dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

static f32 vec3_distance(vec3 a, vec3 b) {
    return sqrtf(vec3_distance_squared(a, b));
}

static bool vec3_within_tolerance(vec3 a, vec3 b, f32 tolerance) {
    return vec3_distance_squared(a, b) <= (tolerance * tolerance);
}

static u32 find_or_create_region(vec3 position) {
    // Find existing region
    for (u32 i = 0; i < g_advanced_cache->region_count; i++) {
        if (vec3_distance(position, g_advanced_cache->regions[i].center) <= g_advanced_cache->regions[i].radius) {
            g_advanced_cache->regions[i].last_access_time = g_advanced_cache->current_time;
            return i;
        }
    }
    
    // Create new region if space available
    if (g_advanced_cache->region_count < ADVANCED_CACHE_MAX_REGIONS) {
        u32 region_idx = g_advanced_cache->region_count++;
        PathRegion* region = &g_advanced_cache->regions[region_idx];
        
        region->center = position;
        region->radius = 10.0f;  // Default region radius
        region->path_count = 0;
        region->path_capacity = 32;
        region->path_indices = allocator_alloc(g_persistent_allocator, region->path_capacity * sizeof(u32));
        region->last_access_time = g_advanced_cache->current_time;
        
        return region_idx;
    }
    
    // Evict least recently used region
    u32 lru_idx = 0;
    f32 oldest_time = g_advanced_cache->regions[0].last_access_time;
    
    for (u32 i = 1; i < ADVANCED_CACHE_MAX_REGIONS; i++) {
        if (g_advanced_cache->regions[i].last_access_time < oldest_time) {
            oldest_time = g_advanced_cache->regions[i].last_access_time;
            lru_idx = i;
        }
    }
    
    // Clear evicted region
    PathRegion* region = &g_advanced_cache->regions[lru_idx];
    if (region->path_indices) {
        FREE(region->path_indices);
    }
    
    region->center = position;
    region->radius = 10.0f;
    region->path_count = 0;
    region->path_capacity = 32;
    region->path_indices = allocator_alloc(g_persistent_allocator, region->path_capacity * sizeof(u32));
    region->last_access_time = g_advanced_cache->current_time;
    
    return lru_idx;
}

static u8 calculate_path_quality(const vec3* waypoints, u32 waypoint_count) {
    if (waypoint_count < 2) return 128;
    
    f32 total_length = 0.0f;
    f32 direct_distance = vec3_distance(waypoints[0], waypoints[waypoint_count - 1]);
    
    // Calculate total path length
    for (u32 i = 0; i < waypoint_count - 1; i++) {
        total_length += vec3_distance(waypoints[i], waypoints[i + 1]);
    }
    
    // Quality based on path efficiency (direct vs actual length)
    f32 efficiency = direct_distance / total_length;
    efficiency = clamp(efficiency, 0.0f, 1.0f);
    
    // Consider waypoint count (fewer waypoints is generally better)
    f32 waypoint_factor = 1.0f - ((f32)(waypoint_count - 2) / 100.0f);
    waypoint_factor = clamp(waypoint_factor, 0.0f, 1.0f);
    
    u8 quality = (u8)(efficiency * waypoint_factor * 255);
    return quality;
}

// ============================================================================
// CACHE INITIALIZATION AND MANAGEMENT
// ============================================================================

bool pathfinding_cache_advanced_initialize(u32 max_paths, u32 max_regions) {
    if (g_advanced_cache) {
        LOG_WARN(LOG_CAT_AI, "Advanced path cache already initialized");
        return true;
    }
    
    g_advanced_cache = MALLOC_PERSISTENT(sizeof(AdvancedPathCache));
    if (!g_advanced_cache) {
        LOG_ERROR(LOG_CAT_AI, "Failed to allocate advanced path cache");
        return false;
    }
    
    memset(g_advanced_cache, 0, sizeof(AdvancedPathCache));
    
    g_advanced_cache->current_time = 0.0f;
    
    LOG_INFO(LOG_CAT_AI, "Advanced path cache initialized");
    return true;
}

void pathfinding_cache_advanced_shutdown(void) {
    if (!g_advanced_cache) return;
    
    // Free region path indices
    for (u32 i = 0; i < g_advanced_cache->region_count; i++) {
        if (g_advanced_cache->regions[i].path_indices) {
            FREE(g_advanced_cache->regions[i].path_indices);
        }
    }
    
    FREE(g_advanced_cache);
    g_advanced_cache = NULL;
    
    LOG_INFO(LOG_CAT_AI, "Advanced path cache shutdown");
}

// ============================================================================
// ADVANCED CACHE OPERATIONS
// ============================================================================

bool pathfinding_cache_advanced_lookup(vec3 start, vec3 goal, vec3** out_waypoints, u32* out_count, u8* out_quality) {
    if (!g_advanced_cache || !out_waypoints || !out_count) return false;
    
    g_advanced_cache->total_requests++;
    
    // Find regions for start and goal
    u32 start_region = find_or_create_region(start);
    u32 goal_region = find_or_create_region(goal);
    
    // Search for matching path
    f32 best_match_distance = FLT_MAX;
    u32 best_match_index = UINT32_MAX;
    
    for (u32 i = 0; i < g_advanced_cache->path_count; i++) {
        AdvancedCachedPath* path = &g_advanced_cache->paths[i];
        if (!path->is_valid) continue;
        
        // Check if path is within tolerance
        f32 start_dist = vec3_distance(start, path->start);
        f32 goal_dist = vec3_distance(goal, path->goal);
        
        if (start_dist <= ADVANCED_CACHE_TOLERANCE && goal_dist <= ADVANCED_CACHE_TOLERANCE) {
            f32 total_dist = start_dist + goal_dist;
            if (total_dist < best_match_distance) {
                best_match_distance = total_dist;
                best_match_index = i;
            }
        }
    }
    
    if (best_match_index != UINT32_MAX) {
        // Cache hit
        AdvancedCachedPath* path = &g_advanced_cache->paths[best_match_index];
        path->last_access_time = g_advanced_cache->current_time;
        path->access_count++;
        
        *out_waypoints = path->waypoints;
        *out_count = path->waypoint_count;
        if (out_quality) *out_quality = path->quality_score;
        
        g_advanced_cache->cache_hits++;
        
        LOG_DEBUG(LOG_CAT_AI, "Advanced cache hit: %u waypoints, quality=%u", 
                  path->waypoint_count, path->quality_score);
        return true;
    }
    
    g_advanced_cache->cache_misses++;
    return false;
}

void pathfinding_cache_advanced_store(vec3 start, vec3 goal, const vec3* waypoints, u32 waypoint_count, f32 computation_time) {
    if (!g_advanced_cache || !waypoints || waypoint_count == 0 || waypoint_count > ADVANCED_CACHE_MAX_WAYPOINTS) {
        return;
    }
    
    // Find or create path slot
    u32 path_index = UINT32_MAX;
    
    // Look for invalid slot first
    for (u32 i = 0; i < g_advanced_cache->path_count; i++) {
        if (!g_advanced_cache->paths[i].is_valid) {
            path_index = i;
            break;
        }
    }
    
    // If no invalid slot, evict lowest quality path
    if (path_index == UINT32_MAX) {
        if (g_advanced_cache->path_count < ADVANCED_CACHE_MAX_PATHS_PER_REGION * ADVANCED_CACHE_MAX_REGIONS) {
            path_index = g_advanced_cache->path_count++;
        } else {
            u8 lowest_quality = 255;
            for (u32 i = 0; i < g_advanced_cache->path_count; i++) {
                if (g_advanced_cache->paths[i].quality_score < lowest_quality) {
                    lowest_quality = g_advanced_cache->paths[i].quality_score;
                    path_index = i;
                }
            }
        }
    }
    
    // Store path
    AdvancedCachedPath* path = &g_advanced_cache->paths[path_index];
    path->start = start;
    path->goal = goal;
    path->waypoint_count = waypoint_count;
    path->creation_time = g_advanced_cache->current_time;
    path->last_access_time = g_advanced_cache->current_time;
    path->access_count = 1;
    path->prediction_success_count = 0;
    path->is_valid = true;
    path->computation_time = computation_time;
    
    // Copy waypoints
    memcpy(path->waypoints, waypoints, waypoint_count * sizeof(vec3));
    
    // Calculate path metrics
    path->path_length = 0.0f;
    for (u32 i = 0; i < waypoint_count - 1; i++) {
        path->path_length += vec3_distance(waypoints[i], waypoints[i + 1]);
    }
    
    // Calculate heuristic (simple Euclidean distance)
    path->heuristic_cost = vec3_distance(start, goal);
    
    // Calculate quality score
    path->quality_score = calculate_path_quality(waypoints, waypoint_count);
    
    // Find regions
    path->region_start = find_or_create_region(start);
    path->region_goal = find_or_create_region(goal);
    
    // Add to regions
    PathRegion* start_region = &g_advanced_cache->regions[path->region_start];
    PathRegion* goal_region = &g_advanced_cache->regions[path->region_goal];
    
    // Add to start region
    if (start_region->path_count >= start_region->path_capacity) {
        start_region->path_capacity *= 2;
        start_region->path_indices = REALLOC_PERSISTENT(start_region->path_indices, 
                                                        start_region->path_capacity * sizeof(u32));
    }
    start_region->path_indices[start_region->path_count++] = path_index;
    
    // Add to goal region if different
    if (path->region_goal != path->region_start) {
        if (goal_region->path_count >= goal_region->path_capacity) {
            goal_region->path_capacity *= 2;
            goal_region->path_indices = REALLOC_PERSISTENT(goal_region->path_indices, 
                                                            goal_region->path_capacity * sizeof(u32));
        }
        goal_region->path_indices[goal_region->path_count++] = path_index;
    }
    
    LOG_DEBUG(LOG_CAT_AI, "Advanced cache store: %u waypoints, quality=%u, length=%.2f", 
              waypoint_count, path->quality_score, path->path_length);
}

// ============================================================================
// PATH PREDICTION
// ============================================================================

void pathfinding_cache_predict_path(vec3 current_position, vec3 likely_goal, f32 confidence) {
    if (!g_advanced_cache || g_advanced_cache->prediction_count >= ADVANCED_CACHE_MAX_PATHS_PER_REGION) {
        return;
    }
    
    // Check if we already have a prediction for this entity
    for (u32 i = 0; i < g_advanced_cache->prediction_count; i++) {
        PathPrediction* pred = &g_advanced_cache->predictions[i];
        if (vec3_within_tolerance(current_position, pred->current_position, 1.0f)) {
            // Update existing prediction
            pred->predicted_goal = likely_goal;
            pred->confidence = confidence;
            pred->frames_since_prediction = 0;
            return;
        }
    }
    
    // Create new prediction
    PathPrediction* pred = &g_advanced_cache->predictions[g_advanced_cache->prediction_count++];
    pred->predicted_goal = likely_goal;
    pred->current_position = current_position;
    pred->confidence = confidence;
    pred->frames_since_prediction = 0;
    pred->target_path_index = UINT32_MAX;
    
    g_advanced_cache->predictions_made++;
    
    LOG_DEBUG(LOG_CAT_AI, "Path prediction created: confidence=%.2f", confidence);
}

bool pathfinding_cache_get_predicted_path(vec3 current_position, vec3** out_waypoints, u32* out_count) {
    if (!g_advanced_cache || !out_waypoints || !out_count) return false;
    
    for (u32 i = 0; i < g_advanced_cache->prediction_count; i++) {
        PathPrediction* pred = &g_advanced_cache->predictions[i];
        
        if (vec3_within_tolerance(current_position, pred->current_position, 2.0f) && 
            pred->confidence > 0.5f) {
            
            // Look for cached path to predicted goal
            if (pathfinding_cache_advanced_lookup(current_position, pred->predicted_goal, 
                                                out_waypoints, out_count, NULL)) {
                pred->frames_since_prediction++;
                pred->target_path_index = i;  // Store for success tracking
                
                LOG_DEBUG(LOG_CAT_AI, "Predicted path found, frames_since=%u", pred->frames_since_prediction);
                return true;
            }
        }
    }
    
    return false;
}

// ============================================================================
// CACHE MAINTENANCE
// ============================================================================

void pathfinding_cache_advanced_update(f32 delta_time) {
    if (!g_advanced_cache) return;
    
    g_advanced_cache->current_time += delta_time;
    
    // Update predictions
    for (u32 i = 0; i < g_advanced_cache->prediction_count; i++) {
        PathPrediction* pred = &g_advanced_cache->predictions[i];
        pred->frames_since_prediction++;
        
        // Decay confidence over time
        pred->confidence *= 0.95f;
        
        // Remove old or low-confidence predictions
        if (pred->confidence < 0.1f || pred->frames_since_prediction > ADVANCED_CACHE_PREDICTION_FRAMES) {
            // Remove prediction by shifting remaining ones
            for (u32 j = i; j < g_advanced_cache->prediction_count - 1; j++) {
                g_advanced_cache->predictions[j] = g_advanced_cache->predictions[j + 1];
            }
            g_advanced_cache->prediction_count--;
            i--;  // Recheck this index
        }
    }
    
    // Remove old paths
    for (u32 i = 0; i < g_advanced_cache->path_count; i++) {
        AdvancedCachedPath* path = &g_advanced_cache->paths[i];
        if (!path->is_valid) continue;
        
        f32 age = g_advanced_cache->current_time - path->creation_time;
        if (age > ADVANCED_CACHE_MAX_AGE) {
            path->is_valid = false;
            g_advanced_cache->evictions++;
        }
    }
    
    // Update statistics
    if (g_advanced_cache->total_requests > 0) {
        f32 hit_rate = (f32)g_advanced_cache->cache_hits / g_advanced_cache->total_requests;
        if (hit_rate < 0.3f) {
            LOG_WARN(LOG_CAT_AI, "Low cache hit rate: %.2f%%", hit_rate * 100.0f);
        }
    }
}

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

void pathfinding_cache_advanced_get_statistics(AdvancedCacheStats* out_stats) {
    if (!g_advanced_cache || !out_stats) return;
    
    out_stats->total_paths = g_advanced_cache->path_count;
    out_stats->valid_paths = 0;
    out_stats->total_regions = g_advanced_cache->region_count;
    out_stats->active_predictions = g_advanced_cache->prediction_count;
    out_stats->total_requests = g_advanced_cache->total_requests;
    out_stats->cache_hits = g_advanced_cache->cache_hits;
    out_stats->cache_misses = g_advanced_cache->cache_misses;
    out_stats->evictions = g_advanced_cache->evictions;
    out_stats->predictions_made = g_advanced_cache->predictions_made;
    out_stats->predictions_successful = g_advanced_cache->predictions_successful;
    
    // Count valid paths and calculate averages
    f32 total_length = 0.0f;
    f32 total_quality = 0.0f;
    
    for (u32 i = 0; i < g_advanced_cache->path_count; i++) {
        AdvancedCachedPath* path = &g_advanced_cache->paths[i];
        if (path->is_valid) {
            out_stats->valid_paths++;
            total_length += path->path_length;
            total_quality += path->quality_score;
        }
    }
    
    out_stats->hit_rate = g_advanced_cache->total_requests > 0 ? 
                         (f32)g_advanced_cache->cache_hits / g_advanced_cache->total_requests : 0.0f;
    out_stats->average_path_length = out_stats->valid_paths > 0 ? total_length / out_stats->valid_paths : 0.0f;
    out_stats->average_quality = out_stats->valid_paths > 0 ? total_quality / out_stats->valid_paths : 0.0f;
    out_stats->current_time = g_advanced_cache->current_time;
}

void pathfinding_cache_advanced_print_statistics(void) {
    if (!g_advanced_cache) {
        LOG_WARN(LOG_CAT_AI, "Advanced path cache not initialized");
        return;
    }
    
    AdvancedCacheStats stats;
    pathfinding_cache_advanced_get_statistics(&stats);
    
    LOG_INFO(LOG_CAT_AI, "=== Advanced Path Cache Statistics ===");
    LOG_INFO(LOG_CAT_AI, "Paths: %u/%u valid", stats.valid_paths, stats.total_paths);
    LOG_INFO(LOG_CAT_AI, "Regions: %u", stats.total_regions);
    LOG_INFO(LOG_CAT_AI, "Predictions: %u active", stats.active_predictions);
    LOG_INFO(LOG_CAT_AI, "Hit rate: %.2f%%", stats.hit_rate * 100.0f);
    LOG_INFO(LOG_CAT_AI, "Requests: %u total, %u hits, %u misses", 
             stats.total_requests, stats.cache_hits, stats.cache_misses);
    LOG_INFO(LOG_CAT_AI, "Average path length: %.2f", stats.average_path_length);
    LOG_INFO(LOG_CAT_AI, "Average quality: %.2f/255", stats.average_quality);
    LOG_INFO(LOG_CAT_AI, "Evictions: %u", stats.evictions);
    LOG_INFO(LOG_CAT_AI, "Predictions: %u made, %u successful", 
             stats.predictions_made, stats.predictions_successful);
    LOG_INFO(LOG_CAT_AI, "=== End Statistics ===");
}
