/**
 * =================================================================================================
 *                          A* PATHFINDING IMPLEMENTATION
 *                          Phase 4: AI & Navigation
 * =================================================================================================
 *
 * PURPOSE: A* pathfinding for grid-based navigation
 * =================================================================================================
 */

#include <float.h>
#include <include/math/math_all.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ASTAR_MAX_NODES 16384
#define ASTAR_MAX_PATH 1024
#define ASTAR_INVALID UINT32_MAX

// Node state
typedef enum { NODE_UNVISITED, NODE_OPEN, NODE_CLOSED } NodeState;

// Navigation node
typedef struct {
  int32_t x, y, z; // Grid position
  float g_cost;    // Cost from start
  float h_cost;    // Heuristic cost to goal
  float f_cost;    // g + h
  uint32_t parent; // Parent node index
  NodeState state;
  bool walkable;
} NavNode;

// Navigation grid
typedef struct {
  NavNode *nodes;
  uint32_t width;
  uint32_t height;
  uint32_t depth; // For 3D grids
  float cell_size;
} NavGrid;

// Priority queue (min-heap by f_cost)
typedef struct {
  uint32_t *indices;
  uint32_t count;
  uint32_t capacity;
  NavGrid *grid;
} AStarOpenSet;

// Path result
typedef struct {
  float *waypoints; // x, y, z positions
  uint32_t waypoint_count;
  float total_cost;
  bool found;
} PathResult;

// Heuristic functions
typedef enum {
  HEURISTIC_MANHATTAN,
  HEURISTIC_EUCLIDEAN,
  HEURISTIC_CHEBYSHEV,
  HEURISTIC_OCTILE
} HeuristicType;

// -----------------------------------------------------------------------------
// Grid Creation
// -----------------------------------------------------------------------------

NavGrid *navgrid_create(uint32_t width, uint32_t height, uint32_t depth,
                        float cell_size) {
  NavGrid *grid = (NavGrid *)calloc(1, sizeof(NavGrid));
  if (!grid)
    return NULL;

  grid->width = width;
  grid->height = height;
  grid->depth = depth > 0 ? depth : 1;
  grid->cell_size = cell_size;

  uint32_t total = width * height * grid->depth;
  grid->nodes = (NavNode *)calloc(total, sizeof(NavNode));
  if (!grid->nodes) {
    free(grid);
    return NULL;
  }

  // Initialize all nodes as walkable
  for (uint32_t z = 0; z < grid->depth; z++) {
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
        uint32_t idx = z * (width * height) + y * width + x;
        grid->nodes[idx].x = (int32_t)x;
        grid->nodes[idx].y = (int32_t)y;
        grid->nodes[idx].z = (int32_t)z;
        grid->nodes[idx].walkable = true;
        grid->nodes[idx].state = NODE_UNVISITED;
        grid->nodes[idx].parent = ASTAR_INVALID;
      }
    }
  }

  return grid;
}

void navgrid_destroy(NavGrid *grid) {
  if (grid) {
    free(grid->nodes);
    free(grid);
  }
}

void navgrid_set_walkable(NavGrid *grid, int32_t x, int32_t y, int32_t z,
                          bool walkable) {
  if (!grid)
    return;
  if (x < 0 || x >= (int32_t)grid->width)
    return;
  if (y < 0 || y >= (int32_t)grid->height)
    return;
  if (z < 0 || z >= (int32_t)grid->depth)
    return;

  uint32_t idx = (uint32_t)z * (grid->width * grid->height) +
                 (uint32_t)y * grid->width + (uint32_t)x;
  grid->nodes[idx].walkable = walkable;
}

bool navgrid_is_walkable(NavGrid *grid, int32_t x, int32_t y, int32_t z) {
  if (!grid)
    return false;
  if (x < 0 || x >= (int32_t)grid->width)
    return false;
  if (y < 0 || y >= (int32_t)grid->height)
    return false;
  if (z < 0 || z >= (int32_t)grid->depth)
    return false;

  uint32_t idx = (uint32_t)z * (grid->width * grid->height) +
                 (uint32_t)y * grid->width + (uint32_t)x;
  return grid->nodes[idx].walkable;
}

static uint32_t navgrid_get_index(NavGrid *grid, int32_t x, int32_t y,
                                  int32_t z) {
  if (x < 0 || x >= (int32_t)grid->width)
    return ASTAR_INVALID;
  if (y < 0 || y >= (int32_t)grid->height)
    return ASTAR_INVALID;
  if (z < 0 || z >= (int32_t)grid->depth)
    return ASTAR_INVALID;

  return (uint32_t)z * (grid->width * grid->height) +
         (uint32_t)y * grid->width + (uint32_t)x;
}

// -----------------------------------------------------------------------------
// Priority Queue (Min-Heap)
// -----------------------------------------------------------------------------

static AStarOpenSet *openset_create(uint32_t capacity, NavGrid *grid) {
  AStarOpenSet *set = (AStarOpenSet *)calloc(1, sizeof(AStarOpenSet));
  if (!set)
    return NULL;

  set->indices = (uint32_t *)calloc(capacity, sizeof(uint32_t));
  if (!set->indices) {
    free(set);
    return NULL;
  }

  set->capacity = capacity;
  set->count = 0;
  set->grid = grid;
  return set;
}

static void openset_destroy(AStarOpenSet *set) {
  if (set) {
    free(set->indices);
    free(set);
  }
}

static void openset_sift_up(AStarOpenSet *set, uint32_t idx) {
  while (idx > 0) {
    uint32_t parent = (idx - 1) / 2;
    if (set->grid->nodes[set->indices[idx]].f_cost >=
        set->grid->nodes[set->indices[parent]].f_cost) {
      break;
    }
    // Swap
    uint32_t temp = set->indices[idx];
    set->indices[idx] = set->indices[parent];
    set->indices[parent] = temp;
    idx = parent;
  }
}

static void openset_sift_down(AStarOpenSet *set, uint32_t idx) {
  while (true) {
    uint32_t smallest = idx;
    uint32_t left = 2 * idx + 1;
    uint32_t right = 2 * idx + 2;

    if (left < set->count &&
        set->grid->nodes[set->indices[left]].f_cost <
            set->grid->nodes[set->indices[smallest]].f_cost) {
      smallest = left;
    }
    if (right < set->count &&
        set->grid->nodes[set->indices[right]].f_cost <
            set->grid->nodes[set->indices[smallest]].f_cost) {
      smallest = right;
    }

    if (smallest == idx)
      break;

    // Swap
    uint32_t temp = set->indices[idx];
    set->indices[idx] = set->indices[smallest];
    set->indices[smallest] = temp;
    idx = smallest;
  }
}

static void openset_push(AStarOpenSet *set, uint32_t node_idx) {
  if (set->count >= set->capacity)
    return;

  set->indices[set->count] = node_idx;
  openset_sift_up(set, set->count);
  set->count++;
}

static uint32_t openset_pop(AStarOpenSet *set) {
  if (set->count == 0)
    return ASTAR_INVALID;

  uint32_t result = set->indices[0];
  set->count--;

  if (set->count > 0) {
    set->indices[0] = set->indices[set->count];
    openset_sift_down(set, 0);
  }

  return result;
}

static bool openset_empty(AStarOpenSet *set) { return set->count == 0; }

// -----------------------------------------------------------------------------
// Heuristics
// -----------------------------------------------------------------------------

static float heuristic(int32_t x1, int32_t y1, int32_t z1, int32_t x2,
                       int32_t y2, int32_t z2, HeuristicType type) {
  int32_t dx = abs(x2 - x1);
  int32_t dy = abs(y2 - y1);
  int32_t dz = abs(z2 - z1);

  switch (type) {
  case HEURISTIC_MANHATTAN:
    return (float)(dx + dy + dz);

  case HEURISTIC_EUCLIDEAN:
    return sqrtf((float)(dx * dx + dy * dy + dz * dz));

  case HEURISTIC_CHEBYSHEV: {
    int32_t max = dx > dy ? dx : dy;
    max = max > dz ? max : dz;
    return (float)max;
  }

  case HEURISTIC_OCTILE: {
    // 2D octile with 3D extension
    int32_t min_xy = dx < dy ? dx : dy;
    int32_t max_xy = dx > dy ? dx : dy;
    return 1.414f * (float)min_xy + (float)(max_xy - min_xy) + (float)dz;
  }

  default:
    return (float)(dx + dy + dz);
  }
}

// -----------------------------------------------------------------------------
// A* Pathfinding
// -----------------------------------------------------------------------------

static void astar_reset(NavGrid *grid) {
  uint32_t total = grid->width * grid->height * grid->depth;
  for (uint32_t i = 0; i < total; i++) {
    grid->nodes[i].state = NODE_UNVISITED;
    grid->nodes[i].g_cost = FLT_MAX;
    grid->nodes[i].h_cost = 0.0f;
    grid->nodes[i].f_cost = FLT_MAX;
    grid->nodes[i].parent = ASTAR_INVALID;
  }
}

// 6-connected neighbors (cardinal directions)
static const int32_t neighbors_6[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                          {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};

// 26-connected neighbors (all directions including diagonals)
static const int32_t neighbors_26[26][3] = {
    {1, 0, 0},   {-1, 0, 0},  {0, 1, 0},  {0, -1, 0},  {0, 0, 1},  {0, 0, -1},
    {1, 1, 0},   {1, -1, 0},  {-1, 1, 0}, {-1, -1, 0}, {1, 0, 1},  {1, 0, -1},
    {-1, 0, 1},  {-1, 0, -1}, {0, 1, 1},  {0, 1, -1},  {0, -1, 1}, {0, -1, -1},
    {1, 1, 1},   {1, 1, -1},  {1, -1, 1}, {1, -1, -1}, {-1, 1, 1}, {-1, 1, -1},
    {-1, -1, 1}, {-1, -1, -1}};

PathResult astar_find_path(NavGrid *grid, int32_t start_x, int32_t start_y,
                           int32_t start_z, int32_t goal_x, int32_t goal_y,
                           int32_t goal_z, bool allow_diagonals,
                           HeuristicType heuristic_type) {
  PathResult result = {NULL, 0, 0.0f, false};

  if (!grid)
    return result;

  uint32_t start_idx = navgrid_get_index(grid, start_x, start_y, start_z);
  uint32_t goal_idx = navgrid_get_index(grid, goal_x, goal_y, goal_z);

  if (start_idx == ASTAR_INVALID || goal_idx == ASTAR_INVALID)
    return result;
  if (!grid->nodes[start_idx].walkable || !grid->nodes[goal_idx].walkable)
    return result;

  // Reset grid
  astar_reset(grid);

  // Create open set
  AStarOpenSet *open = openset_create(ASTAR_MAX_NODES, grid);
  if (!open)
    return result;

  // Initialize start node
  grid->nodes[start_idx].g_cost = 0.0f;
  grid->nodes[start_idx].h_cost = heuristic(start_x, start_y, start_z, goal_x,
                                            goal_y, goal_z, heuristic_type);
  grid->nodes[start_idx].f_cost = grid->nodes[start_idx].h_cost;
  grid->nodes[start_idx].state = NODE_OPEN;
  openset_push(open, start_idx);

  // Neighbor configuration
  const int32_t(*neighbor_dirs)[3] =
      allow_diagonals ? neighbors_26 : neighbors_6;
  int neighbor_count = allow_diagonals ? 26 : 6;

  // A* loop
  while (!openset_empty(open)) {
    uint32_t current_idx = openset_pop(open);
    NavNode *current = &grid->nodes[current_idx];

    // Goal reached?
    if (current_idx == goal_idx) {
      result.found = true;
      result.total_cost = current->g_cost;
      break;
    }

    current->state = NODE_CLOSED;

    // Explore neighbors
    for (int n = 0; n < neighbor_count; n++) {
      int32_t nx = current->x + neighbor_dirs[n][0];
      int32_t ny = current->y + neighbor_dirs[n][1];
      int32_t nz = current->z + neighbor_dirs[n][2];

      uint32_t neighbor_idx = navgrid_get_index(grid, nx, ny, nz);
      if (neighbor_idx == ASTAR_INVALID)
        continue;

      NavNode *neighbor = &grid->nodes[neighbor_idx];
      if (!neighbor->walkable || neighbor->state == NODE_CLOSED)
        continue;

      // Movement cost (diagonal costs more)
      float move_cost = 1.0f;
      int32_t dx = abs(neighbor_dirs[n][0]);
      int32_t dy = abs(neighbor_dirs[n][1]);
      int32_t dz = abs(neighbor_dirs[n][2]);
      int steps = dx + dy + dz;
      if (steps == 2)
        move_cost = 1.414f;
      else if (steps == 3)
        move_cost = 1.732f;

      float tentative_g = current->g_cost + move_cost;

      if (tentative_g < neighbor->g_cost) {
        neighbor->parent = current_idx;
        neighbor->g_cost = tentative_g;
        neighbor->h_cost =
            heuristic(nx, ny, nz, goal_x, goal_y, goal_z, heuristic_type);
        neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;

        if (neighbor->state != NODE_OPEN) {
          neighbor->state = NODE_OPEN;
          openset_push(open, neighbor_idx);
        }
      }
    }
  }

  // Reconstruct path
  if (result.found) {
    // Count path length
    uint32_t path_length = 0;
    uint32_t trace = goal_idx;
    while (trace != ASTAR_INVALID) {
      path_length++;
      trace = grid->nodes[trace].parent;
    }

    // Allocate waypoints
    result.waypoints = (float *)malloc(path_length * 3 * sizeof(float));
    result.waypoint_count = path_length;

    // Fill waypoints (reverse order)
    trace = goal_idx;
    for (uint32_t i = path_length; i > 0; i--) {
      uint32_t idx = i - 1;
      NavNode *node = &grid->nodes[trace];
      result.waypoints[idx * 3 + 0] = ((float)node->x + 0.5f) * grid->cell_size;
      result.waypoints[idx * 3 + 1] = ((float)node->y + 0.5f) * grid->cell_size;
      result.waypoints[idx * 3 + 2] = ((float)node->z + 0.5f) * grid->cell_size;
      trace = node->parent;
    }
  }

  openset_destroy(open);
  return result;
}

void path_result_destroy(PathResult *result) {
  if (result && result->waypoints) {
    free(result->waypoints);
    result->waypoints = NULL;
    result->waypoint_count = 0;
  }
}
