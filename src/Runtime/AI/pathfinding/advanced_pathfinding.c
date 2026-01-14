// Advanced AI Pathfinding Implementation
// Enhanced navigation with hierarchical pathfinding, dynamic obstacles, and crowd simulation

#include "advanced_pathfinding.h"
#include "include/core/logger.h"
#include "math/math_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal structures for pathfinding
typedef struct AStarNode {
  u32 id;
  f32 g_score;    // Cost from start
  f32 f_score;    // Estimated total cost
  u32 parent;
  bool in_open_set;
  bool in_closed_set;
} AStarNode;

typedef struct OpenSet {
  u32 *nodes;
  f32 *f_scores;
  u32 count;
  u32 capacity;
} OpenSet;

// Priority queue for A*
typedef struct PriorityQueue {
  u32 *indices;
  f32 *priorities;
  u32 *positions;  // Maps node ID to position in heap
  u32 count;
  u32 capacity;
} PriorityQueue;

// Helper functions
static f32 heuristic_manhattan(Vec3 a, Vec3 b);
static f32 heuristic_euclidean(Vec3 a, Vec3 b);
static f32 heuristic_chebyshev(Vec3 a, Vec3 b);
static f32 heuristic_octile(Vec3 a, Vec3 b);
static void priority_queue_init(PriorityQueue *pq, u32 capacity);
static void priority_queue_destroy(PriorityQueue *pq);
static void priority_queue_push(PriorityQueue *pq, u32 node_id, f32 priority);
static u32 priority_queue_pop(PriorityQueue *pq);
static bool priority_queue_contains(PriorityQueue *pq, u32 node_id);
static void priority_queue_update(PriorityQueue *pq, u32 node_id, f32 priority);
static void reconstruct_path(NavMesh *mesh, const AStarNode *nodes, u32 goal_id, NavPath *out_path);
static void smooth_path_simple(NavPath *path);
static void smooth_path_catmull_rom(NavPath *path);
static bool line_intersects_obstacle(Vec3 start, Vec3 end, const DynamicObstacle *obstacle);

AdvancedPathfindingSystem *advanced_pathfinding_create(NavMeshType type, u32 width, u32 height, u32 depth, f32 cell_size) {
  AdvancedPathfindingSystem *system = calloc(1, sizeof(AdvancedPathfindingSystem));
  if (!system) {
    LOG_ERROR("Failed to allocate advanced pathfinding system");
    return NULL;
  }
  
  system->mesh_type = type;
  system->algorithm = PATHFINDING_ASTAR;
  system->heuristic = HEURISTIC_EUCLIDEAN;
  system->smoothing = SMOOTHING_SIMPLE;
  
  // Create base navigation mesh
  Vec3 origin = {0.0f, 0.0f, 0.0f};
  system->base_mesh = nav_mesh_create(width, height, depth, cell_size, origin);
  if (!system->base_mesh) {
    LOG_ERROR("Failed to create base navigation mesh");
    free(system);
    return NULL;
  }
  
  // Initialize dynamic obstacles
  system->max_obstacles = 1024;
  system->obstacles = calloc(system->max_obstacles, sizeof(DynamicObstacle));
  
  // Initialize agents
  system->max_agents = 512;
  system->agents = calloc(system->max_agents, sizeof(PathfindingAgent));
  
  // Initialize path cache
  memset(system->path_cache, 0, sizeof(system->path_cache));
  
  // Set default performance limits
  system->max_pathfinding_time = 0.1f;  // 100ms max
  system->max_nodes_explored = 10000;
  system->enable_early_out = true;
  
  LOG_INFO("Advanced pathfinding system created: %dx%dx%d grid", width, height, depth);
  return system;
}

void advanced_pathfinding_destroy(AdvancedPathfindingSystem *system) {
  if (!system) return;
  
  if (system->base_mesh) {
    nav_mesh_destroy(system->base_mesh);
  }
  
  if (system->hierarchical_mesh) {
    free(system->hierarchical_mesh->cluster_map);
    free(system->hierarchical_mesh->entrance_nodes);
    free(system->hierarchical_mesh->high_level);
    free(system->hierarchical_mesh->low_level);
    free(system->hierarchical_mesh);
  }
  
  free(system->obstacles);
  free(system->agents);
  free(system->debug_nodes);
  
  free(system);
  LOG_INFO("Advanced pathfinding system destroyed");
}

PathfindingResult advanced_pathfinding_find_path(AdvancedPathfindingSystem *system, 
                                           Vec3 start, Vec3 end, 
                                           PathfindingAlgorithm algorithm) {
  PathfindingResult result = {0};
  
  if (!system || !system->base_mesh) {
    result.error_message = "Invalid system or navigation mesh";
    return result;
  }
  
  // Check cache first
  if (advanced_pathfinding_get_cached_path(system, start, end, &result.path)) {
    result.success = true;
    result.path_cost = advanced_pathfinding_path_cost(&result.path);
    result.computation_time = 0.001f; // Cache lookup time
    return result;
  }
  
  // Convert world positions to grid coordinates
  u32 start_x = (u32)((start.x - system->base_mesh->origin.x) / system->base_mesh->cell_size);
  u32 start_y = (u32)((start.y - system->base_mesh->origin.y) / system->base_mesh->cell_size);
  u32 start_z = (u32)((start.z - system->base_mesh->origin.z) / system->base_mesh->cell_size);
  
  u32 end_x = (u32)((end.x - system->base_mesh->origin.x) / system->base_mesh->cell_size);
  u32 end_y = (u32)((end.y - system->base_mesh->origin.y) / system->base_mesh->cell_size);
  u32 end_z = (u32)((end.z - system->base_mesh->origin.z) / system->base_mesh->cell_size);
  
  // Validate coordinates
  if (start_x >= system->base_mesh->width || start_y >= system->base_mesh->height || 
      start_z >= system->base_mesh->depth || end_x >= system->base_mesh->width || 
      end_y >= system->base_mesh->height || end_z >= system->base_mesh->depth) {
    result.error_message = "Start or end position out of bounds";
    return result;
  }
  
  // Get node IDs
  u32 start_id = (start_z * system->base_mesh->width * system->base_mesh->height) + 
                 (start_y * system->base_mesh->width) + start_x;
  u32 end_id = (end_z * system->base_mesh->width * system->base_mesh->height) + 
               (end_y * system->base_mesh->width) + end_x;
  
  // Check if start and end are walkable
  if (!system->base_mesh->nodes[start_id].walkable || !system->base_mesh->nodes[end_id].walkable) {
    result.error_message = "Start or end position is not walkable";
    return result;
  }
  
  // Use hierarchical pathfinding if available
  if (system->hierarchical_mesh && algorithm == PATHFINDING_HPA) {
    return advanced_pathfinding_find_hierarchical_path(system, start, end);
  }
  
  // Perform pathfinding based on algorithm
  switch (algorithm) {
    case PATHFINDING_ASTAR:
      return advanced_pathfinding_astar(system, start_id, end_id);
    case PATHFINDING_DIJKSTRA:
      return advanced_pathfinding_dijkstra(system, start_id, end_id);
    case PATHFINDING_JPS:
      return advanced_pathfinding_jps(system, start_id, end_id);
    case PATHFINDING_THETA_STAR:
      return advanced_pathfinding_theta_star(system, start_id, end_id);
    default:
      return advanced_pathfinding_astar(system, start_id, end_id);
  }
}

static PathfindingResult advanced_pathfinding_astar(AdvancedPathfindingSystem *system, u32 start_id, u32 end_id) {
  PathfindingResult result = {0};
  f64 start_time = 0.0; // Would use actual timer
  
  // Initialize A* nodes
  u32 node_count = system->base_mesh->node_count;
  AStarNode *nodes = calloc(node_count, sizeof(AStarNode));
  
  // Initialize start node
  nodes[start_id].g_score = 0.0f;
  nodes[start_id].f_score = advanced_pathfinding_heuristic(system->heuristic,
    system->base_mesh->nodes[start_id].position,
    system->base_mesh->nodes[end_id].position);
  nodes[start_id].parent = UINT32_MAX;
  
  // Priority queue for open set
  PriorityQueue open_set;
  priority_queue_init(&open_set, node_count);
  priority_queue_push(&open_set, start_id, nodes[start_id].f_score);
  nodes[start_id].in_open_set = true;
  
  u32 nodes_explored = 0;
  bool found_path = false;
  
  while (open_set.count > 0 && nodes_explored < system->max_nodes_explored) {
    // Check time limit
    if (system->enable_early_out) {
      f64 current_time = 0.0; // Would use actual timer
      if (current_time - start_time > system->max_pathfinding_time) {
        result.error_message = "Pathfinding time limit exceeded";
        break;
      }
    }
    
    // Get node with lowest f_score
    u32 current_id = priority_queue_pop(&open_set);
    nodes[current_id].in_open_set = false;
    nodes[current_id].in_closed_set = true;
    nodes_explored++;
    
    // Check if we reached the goal
    if (current_id == end_id) {
      found_path = true;
      break;
    }
    
    // Explore neighbors
    NavMeshNode *current_node = &system->base_mesh->nodes[current_id];
    for (u32 i = 0; i < current_node->neighbor_count; i++) {
      u32 neighbor_id = current_node->neighbors[i];
      
      if (nodes[neighbor_id].in_closed_set) continue;
      if (!system->base_mesh->nodes[neighbor_id].walkable) continue;
      
      // Calculate tentative g_score
      Vec3 current_pos = current_node->position;
      Vec3 neighbor_pos = system->base_mesh->nodes[neighbor_id].position;
      f32 distance = vec3_distance(current_pos, neighbor_pos);
      f32 tentative_g = nodes[current_id].g_score + distance * current_node->cost_multiplier;
      
      // Check if this path is better
      if (!nodes[neighbor_id].in_open_set || tentative_g < nodes[neighbor_id].g_score) {
        nodes[neighbor_id].parent = current_id;
        nodes[neighbor_id].g_score = tentative_g;
        
        // Calculate f_score
        Vec3 end_pos = system->base_mesh->nodes[end_id].position;
        f32 h_score = advanced_pathfinding_heuristic(system->heuristic, neighbor_pos, end_pos);
        nodes[neighbor_id].f_score = tentative_g + h_score;
        
        if (!nodes[neighbor_id].in_open_set) {
          priority_queue_push(&open_set, neighbor_id, nodes[neighbor_id].f_score);
          nodes[neighbor_id].in_open_set = true;
        } else {
          priority_queue_update(&open_set, neighbor_id, nodes[neighbor_id].f_score);
        }
      }
    }
  }
  
  if (found_path) {
    reconstruct_path(system->base_mesh, nodes, end_id, &result.path);
    
    // Apply path smoothing
    if (system->smoothing != SMOOTHING_NONE) {
      advanced_pathfinding_smooth_path(system, &result.path, system->smoothing);
    }
    
    result.success = true;
    result.path_cost = nodes[end_id].g_score;
    result.nodes_explored = nodes_explored;
    
    // Cache the result
    advanced_pathfinding_cache_path(system, 
      system->base_mesh->nodes[start_id].position,
      system->base_mesh->nodes[end_id].position,
      &result.path);
  } else {
    result.error_message = "No path found";
  }
  
  // Cleanup
  priority_queue_destroy(&open_set);
  free(nodes);
  
  return result;
}

static void reconstruct_path(NavMesh *mesh, const AStarNode *nodes, u32 goal_id, NavPath *out_path) {
  out_path->count = 0;
  
  u32 current_id = goal_id;
  while (current_id != UINT32_MAX && out_path->count < MAX_PATH_NODES) {
    out_path->points[out_path->count++] = mesh->nodes[current_id].position;
    current_id = nodes[current_id].parent;
  }
  
  // Reverse path to get start-to-end order
  for (u32 i = 0; i < out_path->count / 2; i++) {
    Vec3 temp = out_path->points[i];
    out_path->points[i] = out_path->points[out_path->count - 1 - i];
    out_path->points[out_path->count - 1 - i] = temp;
  }
}

void advanced_pathfinding_smooth_path(AdvancedPathfindingSystem *system, NavPath *path, PathSmoothingType type) {
  if (!system || !path || path->count < 3) return;
  
  switch (type) {
    case SMOOTHING_SIMPLE:
      smooth_path_simple(path);
      break;
    case SMOOTHING_CATMULL_ROM:
      smooth_path_catmull_rom(path);
      break;
    case SMOOTHING_BEZIER:
      // TODO: Implement Bezier smoothing
      smooth_path_simple(path);
      break;
    case SMOOTHING_SPLINE:
      // TODO: Implement spline smoothing
      smooth_path_simple(path);
      break;
    default:
      break;
  }
}

static void smooth_path_simple(NavPath *path) {
  if (path->count < 3) return;
  
  NavPath smoothed;
  smoothed.count = 0;
  smoothed.points[0] = path->points[0]; // Keep start
  smoothed.count++;
  
  for (u32 i = 1; i < path->count - 1; i++) {
    Vec3 prev = path->points[i - 1];
    Vec3 current = path->points[i];
    Vec3 next = path->points[i + 1];
    
    // Check if we can skip this point (line of sight)
    if (!advanced_pathfinding_line_of_sight(NULL, prev, next)) {
      smoothed.points[smoothed.count++] = current;
    }
  }
  
  smoothed.points[smoothed.count++] = path->points[path->count - 1]; // Keep end
  
  // Copy smoothed path back
  *path = smoothed;
}

static void smooth_path_catmull_rom(NavPath *path) {
  if (path->count < 4) return;
  
  NavPath smoothed;
  smoothed.count = 0;
  
  // Add start point
  smoothed.points[smoothed.count++] = path->points[0];
  
  // Generate smooth points between each segment
  for (u32 i = 0; i < path->count - 1; i++) {
    Vec3 p0 = (i == 0) ? path->points[0] : path->points[i - 1];
    Vec3 p1 = path->points[i];
    Vec3 p2 = path->points[i + 1];
    Vec3 p3 = (i == path->count - 2) ? path->points[path->count - 1] : path->points[i + 2];
    
    // Generate 3 intermediate points
    for (f32 t = 0.25f; t < 1.0f; t += 0.25f) {
      Vec3 point = vec3_catmull_rom(p0, p1, p2, p3, t);
      if (smoothed.count < MAX_PATH_NODES) {
        smoothed.points[smoothed.count++] = point;
      }
    }
  }
  
  // Add end point
  if (smoothed.count < MAX_PATH_NODES) {
    smoothed.points[smoothed.count++] = path->points[path->count - 1];
  }
  
  *path = smoothed;
}

u32 advanced_pathfinding_add_agent(AdvancedPathfindingSystem *system, Vec3 position, f32 radius) {
  if (!system || system->agent_count >= system->max_agents) return UINT32_MAX;
  
  u32 agent_id = system->agent_count++;
  PathfindingAgent *agent = &system->agents[agent_id];
  
  agent->id = agent_id;
  agent->position = position;
  agent->velocity = (Vec3){0.0f, 0.0f, 0.0f};
  agent->radius = radius;
  agent->max_speed = 5.0f;
  agent->max_acceleration = 10.0f;
  agent->height = 2.0f;
  agent->climb_height = 0.5f;
  agent->slope_limit = 45.0f;
  agent->can_fly = false;
  agent->can_swim = false;
  agent->personal_space = radius * 2.0f;
  agent->avoidance_force = 5.0f;
  
  agent->has_path = false;
  agent->reached_target = false;
  agent->current_waypoint = 0;
  
  return agent_id;
}

void advanced_pathfinding_set_agent_target(AdvancedPathfindingSystem *system, u32 agent_id, Vec3 target) {
  if (!system || agent_id >= system->agent_count) return;
  
  PathfindingAgent *agent = &system->agents[agent_id];
  agent->target = target;
  
  // Find path to target
  PathfindingResult result = advanced_pathfinding_find_path(system, agent->position, target, system->algorithm);
  
  if (result.success) {
    agent->current_path = result.path;
    agent->current_waypoint = 0;
    agent->has_path = true;
    agent->reached_target = false;
  } else {
    agent->has_path = false;
    LOG_DEBUG("Failed to find path for agent %d: %s", agent_id, result.error_message);
  }
}

void advanced_pathfinding_update_crowd(AdvancedPathfindingSystem *system, f32 delta_time) {
  if (!system) return;
  
  // Update each agent
  for (u32 i = 0; i < system->agent_count; i++) {
    PathfindingAgent *agent = &system->agents[i];
    
    if (!agent->has_path) continue;
    
    // Check if reached current waypoint
    if (agent->current_waypoint < agent->current_path.count) {
      Vec3 waypoint = agent->current_path.points[agent->current_waypoint];
      f32 distance = vec3_distance(agent->position, waypoint);
      
      if (distance < agent->radius) {
        agent->current_waypoint++;
        if (agent->current_waypoint >= agent->current_path.count) {
          agent->reached_target = true;
          agent->has_path = false;
          continue;
        }
      }
      
      // Move towards waypoint
      if (agent->current_waypoint < agent->current_path.count) {
        Vec3 next_waypoint = agent->current_path.points[agent->current_waypoint];
        Vec3 direction = vec3_normalize(vec3_sub(next_waypoint, agent->position));
        
        // Apply avoidance forces
        Vec3 avoidance = {0.0f, 0.0f, 0.0f};
        for (u32 j = 0; j < system->agent_count; j++) {
          if (i == j) continue;
          
          PathfindingAgent *other = &system->agents[j];
          f32 separation = vec3_distance(agent->position, other->position);
          f32 min_separation = agent->personal_space + other->personal_space;
          
          if (separation < min_separation && separation > 0.0f) {
            Vec3 avoid_dir = vec3_normalize(vec3_sub(agent->position, other->position));
            f32 force = (min_separation - separation) / min_separation;
            avoidance = vec3_add(avoidance, vec3_scale(avoid_dir, force * agent->avoidance_force));
          }
        }
        
        // Combine movement and avoidance
        Vec3 desired_velocity = vec3_scale(direction, agent->max_speed);
        desired_velocity = vec3_add(desired_velocity, avoidance);
        
        // Update velocity with acceleration limits
        Vec3 acceleration = vec3_sub(desired_velocity, agent->velocity);
        f32 accel_mag = vec3_length(acceleration);
        if (accel_mag > agent->max_acceleration * delta_time) {
          acceleration = vec3_scale(vec3_normalize(acceleration), agent->max_acceleration * delta_time);
        }
        
        agent->velocity = vec3_add(agent->velocity, acceleration);
        
        // Limit to max speed
        f32 speed = vec3_length(agent->velocity);
        if (speed > agent->max_speed) {
          agent->velocity = vec3_scale(vec3_normalize(agent->velocity), agent->max_speed);
        }
        
        // Update position
        agent->position = vec3_add(agent->position, vec3_scale(agent->velocity, delta_time));
      }
    }
  }
}

bool advanced_pathfinding_line_of_sight(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end) {
  if (!system) return true; // Assume clear if no system
  
  // Check against dynamic obstacles
  for (u32 i = 0; i < system->obstacle_count; i++) {
    const DynamicObstacle *obstacle = &system->obstacles[i];
    if (!obstacle->active) continue;
    
    if (line_intersects_obstacle(start, end, obstacle)) {
      return false;
    }
  }
  
  // TODO: Check against static geometry
  return true;
}

static bool line_intersects_obstacle(Vec3 start, Vec3 end, const DynamicObstacle *obstacle) {
  // Simple sphere intersection test
  Vec3 closest_point = vec3_closest_point_on_line(start, end, obstacle->position);
  f32 distance = vec3_distance(closest_point, obstacle->position);
  return distance < obstacle->radius;
}

f32 advanced_pathfinding_heuristic(PathfindingHeuristic type, Vec3 a, Vec3 b) {
  switch (type) {
    case HEURISTIC_MANHATTAN:
      return heuristic_manhattan(a, b);
    case HEURISTIC_EUCLIDEAN:
      return heuristic_euclidean(a, b);
    case HEURISTIC_CHEBYSHEV:
      return heuristic_chebyshev(a, b);
    case HEURISTIC_OCTILE:
      return heuristic_octile(a, b);
    default:
      return heuristic_euclidean(a, b);
  }
}

static f32 heuristic_manhattan(Vec3 a, Vec3 b) {
  return fabsf(a.x - b.x) + fabsf(a.y - b.y) + fabsf(a.z - b.z);
}

static f32 heuristic_euclidean(Vec3 a, Vec3 b) {
  return vec3_distance(a, b);
}

static f32 heuristic_chebyshev(Vec3 a, Vec3 b) {
  f32 dx = fabsf(a.x - b.x);
  f32 dy = fabsf(a.y - b.y);
  f32 dz = fabsf(a.z - b.z);
  return fmaxf(fmaxf(dx, dy), dz);
}

static f32 heuristic_octile(Vec3 a, Vec3 b) {
  f32 dx = fabsf(a.x - b.x);
  f32 dy = fabsf(a.y - b.y);
  f32 dz = fabsf(a.z - b.z);
  f32 d1 = fabsf(dx - dy);
  f32 d2 = fabsf(dy - dz);
  f32 d3 = fabsf(dz - dx);
  return fminf(d1, fminf(d2, d3)) + sqrtf(2.0f) * fmaxf(d1, fmaxf(d2, d3));
}

// Priority queue implementation
static void priority_queue_init(PriorityQueue *pq, u32 capacity) {
  pq->indices = calloc(capacity, sizeof(u32));
  pq->priorities = calloc(capacity, sizeof(f32));
  pq->positions = calloc(capacity, sizeof(u32));
  pq->count = 0;
  pq->capacity = capacity;
  
  // Initialize positions to invalid
  for (u32 i = 0; i < capacity; i++) {
    pq->positions[i] = UINT32_MAX;
  }
}

static void priority_queue_destroy(PriorityQueue *pq) {
  free(pq->indices);
  free(pq->priorities);
  free(pq->positions);
}

static void priority_queue_push(PriorityQueue *pq, u32 node_id, f32 priority) {
  if (pq->count >= pq->capacity) return;
  
  // Add to end
  u32 index = pq->count++;
  pq->indices[index] = node_id;
  pq->priorities[index] = priority;
  pq->positions[node_id] = index;
  
  // Bubble up
  while (index > 0) {
    u32 parent = (index - 1) / 2;
    if (pq->priorities[index] < pq->priorities[parent]) {
      // Swap
      u32 temp_idx = pq->indices[index];
      f32 temp_prio = pq->priorities[index];
      
      pq->indices[index] = pq->indices[parent];
      pq->priorities[index] = pq->priorities[parent];
      pq->positions[pq->indices[parent]] = index;
      
      pq->indices[parent] = temp_idx;
      pq->priorities[parent] = temp_prio;
      pq->positions[node_id] = parent;
      
      index = parent;
    } else {
      break;
    }
  }
}

static u32 priority_queue_pop(PriorityQueue *pq) {
  if (pq->count == 0) return UINT32_MAX;
  
  u32 result = pq->indices[0];
  pq->positions[result] = UINT32_MAX;
  
  // Move last element to root
  pq->count--;
  if (pq->count > 0) {
    u32 node_id = pq->indices[pq->count];
    f32 priority = pq->priorities[pq->count];
    
    pq->indices[0] = node_id;
    pq->priorities[0] = priority;
    pq->positions[node_id] = 0;
    
    // Bubble down
    u32 index = 0;
    while (true) {
      u32 left = index * 2 + 1;
      u32 right = index * 2 + 2;
      u32 smallest = index;
      
      if (left < pq->count && pq->priorities[left] < pq->priorities[smallest]) {
        smallest = left;
      }
      if (right < pq->count && pq->priorities[right] < pq->priorities[smallest]) {
        smallest = right;
      }
      
      if (smallest != index) {
        // Swap
        u32 temp_idx = pq->indices[index];
        f32 temp_prio = pq->priorities[index];
        
        pq->indices[index] = pq->indices[smallest];
        pq->priorities[index] = pq->priorities[smallest];
        pq->positions[pq->indices[smallest]] = index;
        
        pq->indices[smallest] = temp_idx;
        pq->priorities[smallest] = temp_prio;
        pq->positions[temp_idx] = smallest;
        
        index = smallest;
      } else {
        break;
      }
    }
  }
  
  return result;
}

void advanced_pathfinding_cache_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end, NavPath *path) {
  if (!system || !path) return;
  
  u32 index = system->cache_index;
  PathCacheEntry *entry = &system->path_cache[index];
  
  entry->start = start;
  entry->end = end;
  entry->cached_path = *path;
  entry->cache_time = 0.0; // Would use actual time
  entry->valid = true;
  entry->access_count = 0;
  
  system->cache_index = (system->cache_index + 1) % MAX_CACHE_ENTRIES;
}

bool advanced_pathfinding_get_cached_path(AdvancedPathfindingSystem *system, Vec3 start, Vec3 end, NavPath *path) {
  if (!system || !path) return false;
  
  for (u32 i = 0; i < MAX_CACHE_ENTRIES; i++) {
    PathCacheEntry *entry = &system->path_cache[i];
    if (!entry->valid) continue;
    
    // Check if cached path matches (with some tolerance)
    f32 start_dist = vec3_distance(start, entry->start);
    f32 end_dist = vec3_distance(end, entry->end);
    
    if (start_dist < 1.0f && end_dist < 1.0f) {
      *path = entry->cached_path;
      entry->access_count++;
      return true;
    }
  }
  
  return false;
}

f32 advanced_pathfinding_path_cost(const NavPath *path) {
  if (!path || path->count < 2) return 0.0f;
  
  f32 total_cost = 0.0f;
  for (u32 i = 1; i < path->count; i++) {
    total_cost += vec3_distance(path->points[i - 1], path->points[i]);
  }
  
  return total_cost;
}
