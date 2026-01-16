// Pathfinding Optimizer - Path caching and smoothing for improved performance
// Reduces redundant pathfinding calculations with tolerance-based cache lookup
#include "pathfinding_optimizer.h"
#include "core/logger/unified_logger.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define PATHCACHE_MAX_ENTRIES 256
#define PATHCACHE_TOLERANCE 0.5f  // Same path if within 0.5 units

// Cached path entry
typedef struct {
    vec3 start;
    vec3 goal;
    vec3 *waypoints;
    u32 waypoint_count;
    f32 path_length;
    f32 last_used_time;
    u32 usage_count;
} CachedPath;

// Path cache
typedef struct {
    CachedPath entries[PATHCACHE_MAX_ENTRIES];
    u32 entry_count;
    f32 current_time;
} PathCache;

static PathCache g_path_cache = {0};

// Initialize path cache
void pathfinding_cache_init(void) {
    memset(&g_path_cache, 0, sizeof(PathCache));
    LOG_INFO(LOG_CAT_AI, "Path cache initialized with capacity %u", PATHCACHE_MAX_ENTRIES);
}

// Destroy path cache
void pathfinding_cache_shutdown(void) {
    for (u32 i = 0; i < g_path_cache.entry_count; i++) {
        if (g_path_cache.entries[i].waypoints) {
            free(g_path_cache.entries[i].waypoints);
        }
    }
    memset(&g_path_cache, 0, sizeof(PathCache));
    LOG_INFO(LOG_CAT_AI, "Path cache shutdown");
}

// Check if two points are within tolerance
static bool vec3_close_to(vec3 a, vec3 b, f32 tolerance) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    f32 dz = a.z - b.z;
    f32 dist_sq = dx*dx + dy*dy + dz*dz;
    return dist_sq <= (tolerance * tolerance);
}

// Lookup path in cache
bool pathfinding_cache_lookup(vec3 start, vec3 goal, vec3 **out_waypoints, u32 *out_count) {
    if (!out_waypoints || !out_count) return false;

    // Search cache for match within tolerance
    for (u32 i = 0; i < g_path_cache.entry_count; i++) {
        CachedPath *entry = &g_path_cache.entries[i];

        if (vec3_close_to(entry->start, start, PATHCACHE_TOLERANCE) &&
            vec3_close_to(entry->goal, goal, PATHCACHE_TOLERANCE)) {

            // Cache hit - update statistics
            entry->last_used_time = g_path_cache.current_time;
            entry->usage_count++;

            *out_waypoints = entry->waypoints;
            *out_count = entry->waypoint_count;

            LOG_DEBUG(LOG_CAT_AI, "Cache hit: %u waypoints, usage_count=%u",
                      entry->waypoint_count, entry->usage_count);
            return true;
        }
    }

    return false;  // Cache miss
}

// Store path in cache
void pathfinding_cache_store(vec3 start, vec3 goal, vec3 *waypoints, u32 waypoint_count) {
    if (!waypoints || waypoint_count == 0) return;

    // If cache is full, evict least-recently-used entry
    u32 evict_idx = 0;
    if (g_path_cache.entry_count >= PATHCACHE_MAX_ENTRIES) {
        f32 min_time = g_path_cache.entries[0].last_used_time;
        for (u32 i = 1; i < g_path_cache.entry_count; i++) {
            if (g_path_cache.entries[i].last_used_time < min_time) {
                min_time = g_path_cache.entries[i].last_used_time;
                evict_idx = i;
            }
        }

        // Free old waypoints
        if (g_path_cache.entries[evict_idx].waypoints) {
            free(g_path_cache.entries[evict_idx].waypoints);
        }
    } else {
        evict_idx = g_path_cache.entry_count;
        g_path_cache.entry_count++;
    }

    // Store new path
    CachedPath *entry = &g_path_cache.entries[evict_idx];
    entry->start = start;
    entry->goal = goal;
    entry->waypoint_count = waypoint_count;
    entry->usage_count = 1;
    entry->last_used_time = g_path_cache.current_time;

    // Copy waypoints
    entry->waypoints = malloc(waypoint_count * sizeof(vec3));
    if (entry->waypoints) {
        memcpy(entry->waypoints, waypoints, waypoint_count * sizeof(vec3));

        // Calculate path length
        entry->path_length = 0.0f;
        for (u32 i = 0; i < waypoint_count - 1; i++) {
            vec3 diff = {
                .x = waypoints[i+1].x - waypoints[i].x,
                .y = waypoints[i+1].y - waypoints[i].y,
                .z = waypoints[i+1].z - waypoints[i].z
            };
            f32 dx = diff.x, dy = diff.y, dz = diff.z;
            entry->path_length += sqrtf(dx*dx + dy*dy + dz*dz);
        }

        LOG_DEBUG(LOG_CAT_AI, "Cached path: %u waypoints, length=%.2f",
                  waypoint_count, entry->path_length);
    }
}

// Path smoothing - remove unnecessary waypoints using line-of-sight tests
u32 pathfinding_smooth_path(vec3 *waypoints, u32 waypoint_count, vec3 *out_smoothed) {
    if (!waypoints || waypoint_count < 3 || !out_smoothed) {
        return 0;
    }

    u32 smooth_count = 0;
    out_smoothed[smooth_count++] = waypoints[0];  // Always keep start

    u32 current = 0;
    while (current < waypoint_count - 1) {
        // Find furthest waypoint we can reach directly from current
        u32 furthest = current + 1;
        for (u32 i = current + 2; i < waypoint_count; i++) {
            // TODO: Check line-of-sight between waypoints[current] and waypoints[i]
            // For now, assume all waypoints are visible (in real implementation, check obstacles)
            furthest = i;
        }

        if (furthest == current + 1) {
            // Can't skip next waypoint, add it
            out_smoothed[smooth_count++] = waypoints[current + 1];
            current++;
        } else {
            // Can skip to furthest, add that waypoint
            out_smoothed[smooth_count++] = waypoints[furthest];
            current = furthest;
        }

        if (smooth_count >= waypoint_count) break;  // Safety check
    }

    // Always add goal
    out_smoothed[smooth_count++] = waypoints[waypoint_count - 1];

    LOG_DEBUG(LOG_CAT_AI, "Path smoothed: %u waypoints -> %u waypoints",
              waypoint_count, smooth_count);
    return smooth_count;
}

// Catmull-Rom spline interpolation for smooth path following
vec3 pathfinding_interpolate_path(vec3 *waypoints, u32 waypoint_count, f32 t) {
    if (!waypoints || waypoint_count == 0) {
        return (vec3){0, 0, 0};
    }

    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Find which segment we're in
    f32 segment_length = 1.0f / (waypoint_count - 1);
    u32 segment = (u32)(t / segment_length);
    if (segment >= waypoint_count - 1) segment = waypoint_count - 2;

    // Local t within segment
    f32 local_t = (t - segment * segment_length) / segment_length;

    // Get control points for Catmull-Rom (use duplicated endpoints for boundaries)
    vec3 p0 = waypoints[segment == 0 ? 0 : segment - 1];
    vec3 p1 = waypoints[segment];
    vec3 p2 = waypoints[segment + 1];
    vec3 p3 = waypoints[segment + 1 >= waypoint_count - 1 ? waypoint_count - 1 : segment + 2];

    // Catmull-Rom basis
    f32 t2 = local_t * local_t;
    f32 t3 = t2 * local_t;

    f32 b0 = -0.5f * t3 + t2 - 0.5f * local_t;
    f32 b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
    f32 b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * local_t;
    f32 b3 = 0.5f * t3 - 0.5f * t2;

    // Interpolate position
    vec3 result = {
        .x = b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x,
        .y = b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y,
        .z = b0 * p0.z + b1 * p1.z + b2 * p2.z + b3 * p3.z
    };

    return result;
}

// Update cache timestamp (call once per frame)
void pathfinding_cache_update(f32 delta_time) {
    g_path_cache.current_time += delta_time;
}

// Get cache statistics
void pathfinding_cache_stats(u32 *out_entries, u32 *out_capacity, f32 *out_hit_rate) {
    if (out_entries) *out_entries = g_path_cache.entry_count;
    if (out_capacity) *out_capacity = PATHCACHE_MAX_ENTRIES;

    // Calculate hit rate from total usage
    if (out_hit_rate) {
        u32 total_hits = 0;
        for (u32 i = 0; i < g_path_cache.entry_count; i++) {
            total_hits += g_path_cache.entries[i].usage_count - 1;  // -1 for initial store
        }
        u32 total_uses = g_path_cache.entry_count + total_hits;
        *out_hit_rate = total_uses > 0 ? (f32)total_hits / total_uses : 0.0f;
    }
}
