// Advanced AI Pathfinding System
// Enhanced navigation meshes with hierarchical pathfinding, dynamic obstacles, and crowd simulation

#ifndef ADVANCED_PATHFINDING_H
#define ADVANCED_PATHFINDING_H

#include <common.h>
#include "math/vec3.h"
#include "math/vec2.h"
#include "ai/nav/nav_mesh.h"
#include <stdbool.h>

// Pathfinding algorithms
typedef enum {
  PATHFINDING_ASTAR,
  PATHFINDING_DIJKSTRA,
  PATHFINDING_JPS,        // Jump Point Search
  PATHFINDING_THETA_STAR,  // Theta* for any-angle pathfinding
  PATHFINDING_HPA,        // Hierarchical Pathfinding
  PATHFINDING_D_STAR       // D* Lite for dynamic environments
} PathfindingAlgorithm;

// Pathfinding heuristics
typedef enum {
  HEURISTIC_MANHATTAN,
  HEURISTIC_EUCLIDEAN,
  HEURISTIC_CHEBYSHEV,
  HEURISTIC_OCTILE,
  HEURISTIC_CUSTOM
} PathfindingHeuristic;

// Path smoothing types
typedef enum {
  SMOOTHING_NONE,
  SMOOTHING_SIMPLE,
  SMOOTHING_CATMULL_ROM,
  SMOOTHING_BEZIER,
  SMOOTHING_SPLINE
} PathSmoothingType;

// Navigation mesh types
typedef enum {
  NAVMESH_GRID,        // Regular grid-based
  NAVMESH_TRIANGLE,    // Triangle mesh
  NAVMESH_POLYGON,     // Convex polygons
  NAVMESH_VOXEL,      // Voxel-based
  NAVMESH_HIERARCHICAL // Multi-level hierarchy
} NavMeshType;

// Dynamic obstacle
typedef struct {
  Vec3 position;
  Vec3 size;
  Vec3 velocity;
  f32 radius;
  u32 id;
  bool active;
  f32 avoidance_weight;
} DynamicObstacle;

// Pathfinding agent
typedef struct {
  u32 id;
  Vec3 position;
  Vec3 velocity;
  Vec3 target;
  f32 radius;
  f32 max_speed;
  f32 max_acceleration;
  
  // Pathfinding state
  NavPath current_path;
  u32 current_waypoint;
  bool has_path;
  bool reached_target;
  
  // Agent properties
  f32 height;
  f32 climb_height;
  f32 slope_limit;
  bool can_fly;
  bool can_swim;
  
  // Avoidance
  f32 personal_space;
  f32 avoidance_force;
  DynamicObstacle *avoidance_obstacles;
  u32 avoidance_count;
  
} PathfindingAgent;

// Hierarchical pathfinding
typedef struct {
  NavMesh *high_level;     // Coarse graph
  NavMesh *low_level;      // Detailed graph
  u32 *cluster_map;        // Maps low-level nodes to high-level clusters
  u32 cluster_count;
  
  // Hierarchical connections
  u32 *entrance_nodes;
  u32 entrance_count;
  
} HierarchicalNavMesh;

// Pathfinding cache
typedef struct {
  Vec3 start;
  Vec3 end;
  NavPath cached_path;
  f32 cache_time;
  bool valid;
  u32 access_count;
} PathCacheEntry;

#define MAX_CACHE_ENTRIES 256

// Advanced pathfinding system
typedef struct {
  // Core navigation
  NavMesh *base_mesh;
  HierarchicalNavMesh *hierarchical_mesh;
  NavMeshType mesh_type;
  
  // Pathfinding configuration
  PathfindingAlgorithm algorithm;
  PathfindingHeuristic heuristic;
  PathSmoothingType smoothing;
  
  // Dynamic obstacles
  DynamicObstacle *obstacles;
  u32 obstacle_count;
  u32 max_obstacles;
  
  // Agents
  PathfindingAgent *agents;
  u32 agent_count;
  u32 max_agents;
  
  // Path caching
  PathCacheEntry path_cache[MAX_CACHE_ENTRIES];
  u32 cache_index;
  
  // Performance
  f32 max_pathfinding_time;
  u32 max_nodes_explored;
  bool enable_early_out;
  
  // Debugging
  bool debug_draw_enabled;
  Vec3 *debug_nodes;
  u32 debug_node_count;
  
} AdvancedPathfindingSystem;

// Pathfinding request
typedef struct {
  u32 agent_id;
  Vec3 start;
  Vec3 end;
  PathfindingAlgorithm algorithm;
  f32 max_time;
  bool allow_partial_path;
  void *callback_data;
  void (*completion_callback)(NavPath *path, void *data);
} PathfindingRequest;

// Pathfinding result
typedef struct {
  bool success;
  NavPath path;
  f32 path_cost;
  f32 computation_time;
  u32 nodes_explored;
  const char *error_message;
} PathfindingResult;

// Crowd simulation
typedef struct {
  PathfindingAgent *agents;
  u32 agent_count;
  
  // Global forces
  Vec3 global_force;
  f32 separation_weight;
  f32 cohesion_weight;
  f32 alignment_weight;
  f32 goal_weight;
  
  // Collision avoidance
  f32 avoidance_radius;
  f32 prediction_time;
  
} CrowdSimulation;

#ifdef __cplusplus
extern "C" {
#endif

// System management
AdvancedPathfindingSystem *advanced_pathfinding_create(NavMeshType type, u32 width, u32 height, u32 depth, f32 cell_size);
void advanced_pathfinding_destroy(AdvancedPathfindingSystem *system);

// Navigation mesh management
void advanced_pathfinding_build_mesh(AdvancedPathfindingSystem *system, const void *world_data);
void advanced_pathfinding_update_mesh(AdvancedPathfindingSystem *system, u32 x, u32 y, u32 z, bool walkable);
void advanced_pathfinding_build_hierarchy(AdvancedPathfindingSystem *system, u32 levels);

// Pathfinding
PathfindingResult advanced_pathfinding_find_path(AdvancedPathfindingSystem *system, 
                                           Vec3 start, Vec3 end, 
                                           PathfindingAlgorithm algorithm);
PathfindingResult advanced_pathfinding_find_path_for_agent(AdvancedPathfindingSystem *system, 
                                                     u32 agent_id, Vec3 target);
bool advanced_pathfinding_request_path(AdvancedPathfindingSystem *system, PathfindingRequest *request);

// Dynamic obstacles
u32 advanced_pathfinding_add_obstacle(AdvancedPathfindingSystem *system, Vec3 position, f32 radius, Vec3 velocity);
void advanced_pathfinding_remove_obstacle(AdvancedPathfindingSystem *system, u32 obstacle_id);
void advanced_pathfinding_update_obstacle(AdvancedPathfindingSystem *system, u32 obstacle_id, Vec3 position, Vec3 velocity);
void advanced_pathfinding_clear_obstacles(AdvancedPathfindingSystem *system);

// Agent management
u32 advanced_pathfinding_add_agent(AdvancedPathfindingSystem *system, Vec3 position, f32 radius);
void advanced_pathfinding_remove_agent(AdvancedPathfindingSystem *system, u32 agent_id);
void advanced_pathfinding_update_agent(AdvancedPathfindingSystem *system, u32 agent_id, Vec3 position, Vec3 velocity);
void advanced_pathfinding_set_agent_target(AdvancedPathfindingSystem *system, u32 agent_id, Vec3 target);
PathfindingAgent *advanced_pathfinding_get_agent(AdvancedPathfindingSystem *system, u32 agent_id);

// Path smoothing and optimization
void advanced_pathfinding_smooth_path(AdvancedPathfindingSystem *system, NavPath *path, PathSmoothingType type);
void advanced_pathfinding_optimize_path(AdvancedPathfindingSystem *system, NavPath *path);
void advanced_pathfinding_string_pull(AdvancedPathfindingSystem *system, NavPath *path);

// Crowd simulation
void advanced_pathfinding_update_crowd(AdvancedPathfindingSystem *system, f32 delta_time);
void advanced_pathfinding_apply_crowd_forces(AdvancedPathfindingSystem *system);
void advanced_pathfinding_resolve_collisions(AdvancedPathfindingSystem *system);

// Hierarchical pathfinding
PathfindingResult advanced_pathfinding_find_hierarchical_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end);
void advanced_pathfinding_abstract_path(AdvancedPathfindingSystem *system, NavPath *detailed_path, NavPath *abstract_path);
void advanced_pathfinding_refine_path(AdvancedPathfindingSystem *system, NavPath *abstract_path, NavPath *refined_path);

// Dynamic pathfinding (D* Lite)
void advanced_pathfinding_init_dynamic(AdvancedPathfindingSystem *system);
PathfindingResult advanced_pathfinding_update_dynamic_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end);

// Path caching
void advanced_pathfinding_cache_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end, NavPath *path);
bool advanced_pathfinding_get_cached_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end, NavPath *path);
void advanced_pathfinding_clear_cache(AdvancedPathfindingSystem *system);

// Raycasting and line-of-sight
bool advanced_pathfinding_line_of_sight(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end);
bool advanced_pathfinding_raycast(AdvancedPathfindingSystem *system, Vec3 start, Vec3 direction, f32 max_distance, Vec3 *hit_point);
f32 advanced_pathfinding_distance_to_wall(AdvancedPathfindingSystem *system, Vec3 position, Vec3 direction);

// Utility functions
f32 advanced_pathfinding_heuristic(PathfindingHeuristic type, Vec3 a, Vec3 b);
f32 advanced_pathfinding_path_cost(const NavPath *path);
bool advanced_pathfinding_is_valid_position(AdvancedPathfindingSystem *system, Vec3 position);
Vec3 advanced_pathfinding_closest_valid_position(AdvancedPathfindingSystem *system, Vec3 position);

// Performance and debugging
void advanced_pathfinding_enable_debug_draw(AdvancedPathfindingSystem *system, bool enable);
void advanced_pathdrawing_render_debug(AdvancedPathfindingSystem *system);
void advanced_pathfinding_get_stats(AdvancedPathfindingSystem *system, u32 *total_agents, u32 *active_paths, f32 *avg_computation_time);

// Configuration
void advanced_pathfinding_set_algorithm(AdvancedPathfindingSystem *system, PathfindingAlgorithm algorithm);
void advanced_pathfinding_set_heuristic(AdvancedPathfindingSystem *system, PathfindingHeuristic heuristic);
void advanced_pathfinding_set_smoothing(AdvancedPathfindingSystem *system, PathSmoothingType smoothing);
void advanced_pathfinding_set_performance_limits(AdvancedPathfindingSystem *system, f32 max_time, u32 max_nodes);

// Specialized pathfinding
PathfindingResult advanced_pathfinding_find_flight_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end);
PathfindingResult advanced_pathfinding_find_swim_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end);
PathfindingResult advanced_pathfinding_find_climb_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end, f32 max_climb_height);

// Multi-agent coordination
void advanced_pathfinding_coordinate_agents(AdvancedPathfindingSystem *system);
void advanced_pathfinding_avoid_agents(AdvancedPathfindingSystem *system, u32 agent_id);
void advanced_pathfinding_assign_priorities(AdvancedPathfindingSystem *system);

// Integration with world
void advanced_pathfinding_update_from_world(AdvancedPathfindingSystem *system, const void *world_changes);
void advanced_pathfinding_export_navmesh(AdvancedPathfindingSystem *system, const char *filename);
void advanced_pathfinding_import_navmesh(AdvancedPathfindingSystem *system, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_PATHFINDING_H
