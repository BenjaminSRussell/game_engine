// Pathfinding Optimizer - Path caching and smoothing utilities
#ifndef PATHFINDING_OPTIMIZER_H
#define PATHFINDING_OPTIMIZER_H

#include "core/math/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize path caching system
void pathfinding_cache_init(void);

// Shutdown path caching system
void pathfinding_cache_shutdown(void);

// Lookup cached path - returns true if found
// start, goal: Query endpoints
// out_waypoints, out_count: Output path data
bool pathfinding_cache_lookup(vec3 start, vec3 goal, vec3 **out_waypoints, u32 *out_count);

// Store path in cache
// waypoints: Array of waypoint positions
// waypoint_count: Number of waypoints
void pathfinding_cache_store(vec3 start, vec3 goal, vec3 *waypoints, u32 waypoint_count);

// Smooth path by removing unnecessary waypoints
// Uses line-of-sight tests to connect non-adjacent waypoints
// Returns number of smoothed waypoints
u32 pathfinding_smooth_path(vec3 *waypoints, u32 waypoint_count, vec3 *out_smoothed);

// Catmull-Rom spline interpolation for smooth path following
// t: Parameter in [0,1] representing position along full path
// Returns interpolated position
vec3 pathfinding_interpolate_path(vec3 *waypoints, u32 waypoint_count, f32 t);

// Update cache timestamp (call once per frame)
void pathfinding_cache_update(f32 delta_time);

// Get cache statistics
// out_hit_rate: Cache hit rate [0,1]
void pathfinding_cache_stats(u32 *out_entries, u32 *out_capacity, f32 *out_hit_rate);

#ifdef __cplusplus
}
#endif

#endif // PATHFINDING_OPTIMIZER_H
