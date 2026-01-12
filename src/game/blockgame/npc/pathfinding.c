// Optimized NPC pathfinding implementation with priority queue and path
// smoothing.
#include <block/block.h>
#include <chunk/chunk.h>
#include <ecs/ecs.h>
#include <math.h>
#include <math/vec3.h>
#include <npc/pathfinding.h>
#include <stdlib.h>
#include <string.h>
#include <world/world_data.h>

// Priority queue node for A*
typedef struct {
  i32 node_index;
  f32 f_cost;
} PriorityQueueNode;

// Min-heap priority queue for efficient pathfinding
typedef struct {
  PriorityQueueNode *nodes;
  int capacity;
  int count;
} PriorityQueue;

// Initialize priority queue
static void pq_init(PriorityQueue *pq, int capacity) {
  pq->nodes = malloc(sizeof(PriorityQueueNode) * capacity);
  pq->capacity = capacity;
  pq->count = 0;
}

// Free priority queue
static void pq_free(PriorityQueue *pq) {
  free(pq->nodes);
  pq->nodes = NULL;
  pq->count = 0;
}

// Swap two nodes
static void pq_swap(PriorityQueue *pq, int i, int j) {
  PriorityQueueNode temp = pq->nodes[i];
  pq->nodes[i] = pq->nodes[j];
  pq->nodes[j] = temp;
}

// Bubble up for min-heap
static void pq_bubble_up(PriorityQueue *pq, int index) {
  while (index > 0) {
    int parent = (index - 1) / 2;
    if (pq->nodes[parent].f_cost <= pq->nodes[index].f_cost)
      break;
    pq_swap(pq, parent, index);
    index = parent;
  }
}

// Bubble down for min-heap
static void pq_bubble_down(PriorityQueue *pq, int index) {
  while (1) {
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    int smallest = index;

    if (left < pq->count && pq->nodes[left].f_cost < pq->nodes[smallest].f_cost)
      smallest = left;
    if (right < pq->count &&
        pq->nodes[right].f_cost < pq->nodes[smallest].f_cost)
      smallest = right;

    if (smallest == index)
      break;
    pq_swap(pq, smallest, index);
    index = smallest;
  }
}

// Add to priority queue
static void pq_push(PriorityQueue *pq, int node_index, f32 f_cost) {
  if (pq->count >= pq->capacity)
    return;
  pq->nodes[pq->count].node_index = node_index;
  pq->nodes[pq->count].f_cost = f_cost;
  pq_bubble_up(pq, pq->count);
  pq->count++;
}

// Remove minimum from priority queue
static int pq_pop(PriorityQueue *pq) {
  if (pq->count == 0)
    return -1;
  int result = pq->nodes[0].node_index;
  pq->count--;
  if (pq->count > 0) {
    pq->nodes[0] = pq->nodes[pq->count];
    pq_bubble_down(pq, 0);
  }
  return result;
}

// Path node for A*
typedef struct {
  i32 x, y, z;
  f32 g_cost;
  f32 h_cost;
  f32 f_cost;
  int parent_index;
} PathNode;

// Simple hash function for spatial hashing
static inline u32 hash_position(i32 x, i32 y, i32 z) {
  return ((u32)x * 73856093u) ^ ((u32)y * 19349663u) ^ ((u32)z * 83492791u);
}

// Heuristic (Manhattan distance)
static f32 heuristic(int x1, int y1, int z1, int x2, int y2, int z2) {
  return (f32)(abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2));
}

// Check if position is walkable (basic version)
bool npc_is_walkable(ChunkManager *chunk_manager, BlockRegistry *block_registry,
                     i32 x, i32 y, i32 z) {
  if (!chunk_manager || !block_registry)
    return false;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return false;

  i32 local_x = x - cp.x * CHUNK_SIZE;
  i32 local_y = y - cp.y * CHUNK_SIZE;
  i32 local_z = z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, local_x, local_y, local_z);

  // Check if block is solid
  const BlockType *block_type = block_registry_get(block_registry, block);
  if (block_type && block_is_solid(block_type)) {
    return false;
  }

  // Check if block above is clear
  BlockID above = chunk_get_block(chunk, local_x, local_y + 1, local_z);
  const BlockType *above_type = block_registry_get(block_registry, above);
  if (above_type && block_is_solid(above_type)) {
    return false;
  }

  return true;
}

// Smooth path by removing unnecessary waypoints
static void smooth_path(Vec3 *path, u32 *path_length) {
  if (*path_length <= 2)
    return;

  Vec3 smoothed[32];
  u32 smoothed_count = 0;

  smoothed[smoothed_count++] = path[0];

  for (u32 i = 1; i < *path_length - 1; i++) {
    Vec3 prev = path[i - 1];
    Vec3 curr = path[i];
    Vec3 next = path[i + 1];

    // Check if current point is necessary (not collinear)
    Vec3 dir1 = vec3_normalize(vec3_sub(curr, prev));
    Vec3 dir2 = vec3_normalize(vec3_sub(next, curr));

    f32 dot = vec3_dot(dir1, dir2);
    // If not roughly the same direction, keep the waypoint
    if (dot < 0.95f) {
      smoothed[smoothed_count++] = curr;
    }
  }

  smoothed[smoothed_count++] = path[*path_length - 1];

  memcpy(path, smoothed, sizeof(Vec3) * smoothed_count);
  *path_length = smoothed_count;
}

// Optimized A* pathfinding with priority queue
bool npc_find_path(struct World *ecs, EntityID entity,
                   ChunkManager *chunk_manager, BlockRegistry *block_registry,
                   Vec3 start_pos, Vec3 goal_pos, Vec3 *path, u32 *path_length,
                   u32 max_path_length) {
  i32 start_x = (i32)floor(start_pos.x);
  i32 start_y = (i32)floor(start_pos.y);
  i32 start_z = (i32)floor(start_pos.z);
  i32 goal_x = (i32)floor(goal_pos.x);
  i32 goal_y = (i32)floor(goal_pos.y);
  i32 goal_z = (i32)floor(goal_pos.z);

  // Early termination if goal is too far
  f32 straight_distance =
      heuristic(start_x, start_y, start_z, goal_x, goal_y, goal_z);
  if (straight_distance > 50.0f) {
    *path_length = 0;
    return false;
  }

  int max_nodes = 4096;
  PathNode *nodes = malloc(sizeof(PathNode) * max_nodes);
  int node_count = 0;

  PriorityQueue open_set;
  pq_init(&open_set, max_nodes);

  bool *closed_set = calloc(max_nodes, sizeof(bool));

  // Start node
  nodes[node_count++] = (PathNode){
      .x = start_x,
      .y = start_y,
      .z = start_z,
      .g_cost = 0,
      .h_cost = heuristic(start_x, start_y, start_z, goal_x, goal_y, goal_z),
      .f_cost = heuristic(start_x, start_y, start_z, goal_x, goal_y, goal_z),
      .parent_index = -1,
  };
  pq_push(&open_set, 0, nodes[0].f_cost);

  int goal_node_index = -1;

  while (open_set.count > 0) {
    int current_node_index = pq_pop(&open_set);
    PathNode *current_node = &nodes[current_node_index];

    // Check if we reached the goal (or close enough)
    f32 dist_to_goal = heuristic(current_node->x, current_node->y,
                                 current_node->z, goal_x, goal_y, goal_z);
    if (dist_to_goal < 2.0f) {
      goal_node_index = current_node_index;
      break;
    }

    closed_set[current_node_index] = true;

    // Explore neighbors (6-directional for simplicity - can jump is handled
    // separately)
    i32 directions[][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 0, 1},
                           {0, 0, -1}, {0, 1, 0},  {0, -1, 0}};

    for (int d = 0; d < 6; d++) {
      i32 neighbor_x = current_node->x + directions[d][0];
      i32 neighbor_y = current_node->y + directions[d][1];
      i32 neighbor_z = current_node->z + directions[d][2];

      if (!npc_is_walkable(chunk_manager, block_registry, neighbor_x,
                           neighbor_y, neighbor_z)) {
        continue;
      }

      // Find or create neighbor node
      int neighbor_node_index = -1;
      bool in_closed = false;

      for (int i = 0; i < node_count; ++i) {
        if (nodes[i].x == neighbor_x && nodes[i].y == neighbor_y &&
            nodes[i].z == neighbor_z) {
          neighbor_node_index = i;
          if (closed_set[i]) {
            in_closed = true;
          }
          break;
        }
      }

      if (in_closed)
        continue;

      float new_g_cost = current_node->g_cost + 1.0f;

      if (neighbor_node_index == -1) {
        if (node_count >= max_nodes)
          continue;
        neighbor_node_index = node_count++;
        nodes[neighbor_node_index] = (PathNode){
            .x = neighbor_x,
            .y = neighbor_y,
            .z = neighbor_z,
            .g_cost = new_g_cost,
            .h_cost = heuristic(neighbor_x, neighbor_y, neighbor_z, goal_x,
                                goal_y, goal_z),
        };
        nodes[neighbor_node_index].f_cost = nodes[neighbor_node_index].g_cost +
                                            nodes[neighbor_node_index].h_cost;
        nodes[neighbor_node_index].parent_index = current_node_index;
        pq_push(&open_set, neighbor_node_index,
                nodes[neighbor_node_index].f_cost);
      } else if (new_g_cost < nodes[neighbor_node_index].g_cost) {
        nodes[neighbor_node_index].g_cost = new_g_cost;
        nodes[neighbor_node_index].f_cost =
            new_g_cost + nodes[neighbor_node_index].h_cost;
        nodes[neighbor_node_index].parent_index = current_node_index;
        pq_push(&open_set, neighbor_node_index,
                nodes[neighbor_node_index].f_cost);
      }
    }
  }

  bool path_found = false;
  if (goal_node_index != -1) {
    // Reconstruct path
    int current_index = goal_node_index;
    int count = 0;
    while (current_index != -1 && count < (int)max_path_length) {
      count++;
      current_index = nodes[current_index].parent_index;
    }

    *path_length = count;
    current_index = goal_node_index;
    for (int i = count - 1; i >= 0; i--) {
      path[i] = (Vec3){(f32)nodes[current_index].x + 0.5f,
                       (f32)nodes[current_index].y,
                       (f32)nodes[current_index].z + 0.5f};
      current_index = nodes[current_index].parent_index;
    }

    // Smooth the path
    smooth_path(path, path_length);

    path_found = true;
  }

  free(nodes);
  pq_free(&open_set);
  free(closed_set);

  return path_found;
}
