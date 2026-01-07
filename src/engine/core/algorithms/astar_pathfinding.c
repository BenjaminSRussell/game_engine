#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdint.h>

// ✅ COMPLETED: A* Pathfinding Implementation - AGENT_CORE_2
// Optimal pathfinding algorithm using heuristic-guided search
// Combines Dijkstra's completeness with greedy best-first efficiency

typedef struct {
    i32 x, y;              // Grid coordinates
} AStarNode;

typedef struct {
    AStarNode position;     // Node position
    f32 g_cost;           // Cost from start to this node
    f32 h_cost;           // Heuristic cost to goal
    f32 f_cost;           // Total cost (g + h)
    i32 parent_index;     // Index of parent node in path array
    bool is_open;         // Is node in open set
    bool is_closed;       // Is node in closed set
    bool is_walkable;     // Can this node be traversed
} AStarPathNode;

typedef struct {
    AStarPathNode* nodes;  // Grid of pathfinding nodes
    i32* open_set;         // Min-heap for open set
    size_t width;          // Grid width
    size_t height;         // Grid height
    size_t total_nodes;    // Total nodes in grid
    size_t open_count;     // Number of nodes in open set
    AStarNode start;       // Start position
    AStarNode goal;        // Goal position
    bool diagonal_movement; // Allow diagonal movement
} AStarContext;

// ✅ COMPLETED: Heuristic functions
static f32 astar_manhattan_distance(const AStarNode* a, const AStarNode* b) {
    return (f32)(abs(a->x - b->x) + abs(a->y - b->y));
}

static f32 astar_euclidean_distance(const AStarNode* a, const AStarNode* b) {
    i32 dx = a->x - b->x;
    i32 dy = a->y - b->y;
    return sqrtf((f32)(dx * dx + dy * dy));
}

static f32 astar_diagonal_distance(const AStarNode* a, const AStarNode* b) {
    i32 dx = abs(a->x - b->x);
    i32 dy = abs(a->y - b->y);
    return (f32)(dx + dy) + (sqrtf(2.0f) - 2.0f) * (f32)(dx < dy ? dx : dy);
}

// ✅ COMPLETED: Priority queue operations for open set
static void astar_heap_push(AStarContext* ctx, i32 node_index) {
    size_t i = ctx->open_count++;
    ctx->open_set[i] = node_index;
    
    // Heapify up
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (ctx->nodes[ctx->open_set[parent]].f_cost <= ctx->nodes[ctx->open_set[i]].f_cost) {
            break;
        }
        
        // Swap with parent
        i32 temp = ctx->open_set[parent];
        ctx->open_set[parent] = ctx->open_set[i];
        ctx->open_set[i] = temp;
        i = parent;
    }
}

static i32 astar_heap_pop(AStarContext* ctx) {
    if (ctx->open_count == 0) return -1;
    
    i32 result = ctx->open_set[0];
    ctx->open_count--;
    
    if (ctx->open_count > 0) {
        ctx->open_set[0] = ctx->open_set[ctx->open_count];
        
        // Heapify down
        size_t i = 0;
        while (true) {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            size_t smallest = i;
            
            if (left < ctx->open_count && 
                ctx->nodes[ctx->open_set[left]].f_cost < ctx->nodes[ctx->open_set[smallest]].f_cost) {
                smallest = left;
            }
            
            if (right < ctx->open_count && 
                ctx->nodes[ctx->open_set[right]].f_cost < ctx->nodes[ctx->open_set[smallest]].f_cost) {
                smallest = right;
            }
            
            if (smallest == i) break;
            
            // Swap with smallest child
            i32 temp = ctx->open_set[i];
            ctx->open_set[i] = ctx->open_set[smallest];
            ctx->open_set[smallest] = temp;
            i = smallest;
        }
    }
    
    return result;
}

// ✅ COMPLETED: Get node index from position
static size_t astar_get_node_index(const AStarContext* ctx, const AStarNode* pos) {
    if (pos->x < 0 || pos->x >= (i32)ctx->width || 
        pos->y < 0 || pos->y >= (i32)ctx->height) {
        return SIZE_MAX;
    }
    return (size_t)pos->y * ctx->width + (size_t)pos->x;
}

// ✅ COMPLETED: Get neighbors of a node
static void astar_get_neighbors(const AStarContext* ctx, const AStarNode* pos, 
                               AStarNode* neighbors, size_t* neighbor_count) {
    *neighbor_count = 0;
    
    // Cardinal directions
    const i32 directions[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    
    for (i32 i = 0; i < 4; i++) {
        AStarNode neighbor = {pos->x + directions[i][0], pos->y + directions[i][1]};
        size_t index = astar_get_node_index(ctx, &neighbor);
        
        if (index != SIZE_MAX && ctx->nodes[index].is_walkable) {
            neighbors[(*neighbor_count)++] = neighbor;
        }
    }
    
    // Diagonal directions (if enabled)
    if (ctx->diagonal_movement) {
        const i32 diagonals[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        
        for (i32 i = 0; i < 4; i++) {
            AStarNode neighbor = {pos->x + diagonals[i][0], pos->y + diagonals[i][1]};
            size_t index = astar_get_node_index(ctx, &neighbor);
            
            if (index != SIZE_MAX && ctx->nodes[index].is_walkable) {
                // Check if diagonal movement is valid (corner cutting)
                AStarNode horiz = {pos->x + diagonals[i][0], pos->y};
                AStarNode vert = {pos->x, pos->y + diagonals[i][1]};
                size_t horiz_index = astar_get_node_index(ctx, &horiz);
                size_t vert_index = astar_get_node_index(ctx, &vert);
                
                if ((horiz_index == SIZE_MAX || ctx->nodes[horiz_index].is_walkable) &&
                    (vert_index == SIZE_MAX || ctx->nodes[vert_index].is_walkable)) {
                    neighbors[(*neighbor_count)++] = neighbor;
                }
            }
        }
    }
}

// ✅ COMPLETED: Calculate movement cost
static f32 astar_movement_cost(const AStarNode* from, const AStarNode* to, bool diagonal) {
    if (diagonal) {
        i32 dx = to->x - from->x;
        i32 dy = to->y - from->y;
        return sqrtf((f32)(dx * dx + dy * dy));
    }
    return 1.0f;  // Cardinal movement cost
}

// ✅ COMPLETED: Reconstruct path from goal to start
static AStarNode* astar_reconstruct_path(const AStarContext* ctx, size_t* path_length) {
    // Count path length
    *path_length = 0;
    i32 current_index = astar_get_node_index(ctx, &ctx->goal);
    
    while (current_index != -1) {
        (*path_length)++;
        current_index = ctx->nodes[current_index].parent_index;
    }
    
    if (*path_length == 0) return NULL;
    
    // Allocate path array
    AStarNode* path = malloc(*path_length * sizeof(AStarNode));
    if (!path) return NULL;
    
    // Fill path (reverse order)
    current_index = astar_get_node_index(ctx, &ctx->goal);
    for (size_t i = *path_length; i > 0; i--) {
        path[i - 1] = ctx->nodes[current_index].position;
        current_index = ctx->nodes[current_index].parent_index;
    }
    
    return path;
}

// ✅ COMPLETED: A* Context Creation
AStarContext* astar_create_context(size_t width, size_t height, const bool* walkable_map) {
    if (width == 0 || height == 0) return NULL;
    
    AStarContext* ctx = malloc(sizeof(AStarContext));
    if (!ctx) return NULL;
    
    ctx->width = width;
    ctx->height = height;
    ctx->total_nodes = width * height;
    ctx->open_count = 0;
    ctx->diagonal_movement = true;
    
    // Allocate nodes
    ctx->nodes = calloc(ctx->total_nodes, sizeof(AStarPathNode));
    if (!ctx->nodes) {
        free(ctx);
        return NULL;
    }
    
    // Initialize nodes
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t index = y * width + x;
            ctx->nodes[index].position.x = (i32)x;
            ctx->nodes[index].position.y = (i32)y;
            ctx->nodes[index].is_walkable = walkable_map ? walkable_map[index] : true;
            ctx->nodes[index].parent_index = -1;
        }
    }
    
    // Allocate open set
    ctx->open_set = malloc(ctx->total_nodes * sizeof(i32));
    if (!ctx->open_set) {
        free(ctx->nodes);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

// ✅ COMPLETED: A* Pathfinding
AStarNode* astar_find_path(AStarContext* ctx, const AStarNode* start, const AStarNode* goal, 
                          size_t* path_length, f32 (*heuristic)(const AStarNode*, const AStarNode*)) {
    if (!ctx || !start || !goal || !path_length) return NULL;
    
    // Use default heuristic if none provided
    if (!heuristic) {
        heuristic = astar_manhattan_distance;
    }
    
    ctx->start = *start;
    ctx->goal = *goal;
    *path_length = 0;
    
    // Reset all nodes
    for (size_t i = 0; i < ctx->total_nodes; i++) {
        ctx->nodes[i].g_cost = FLT_MAX;
        ctx->nodes[i].h_cost = 0;
        ctx->nodes[i].f_cost = FLT_MAX;
        ctx->nodes[i].parent_index = -1;
        ctx->nodes[i].is_open = false;
        ctx->nodes[i].is_closed = false;
    }
    
    // Reset open set
    ctx->open_count = 0;
    
    // Initialize start node
    size_t start_index = astar_get_node_index(ctx, start);
    size_t goal_index = astar_get_node_index(ctx, goal);
    
    if (start_index == SIZE_MAX || goal_index == SIZE_MAX) return NULL;
    if (!ctx->nodes[start_index].is_walkable || !ctx->nodes[goal_index].is_walkable) return NULL;
    
    ctx->nodes[start_index].g_cost = 0;
    ctx->nodes[start_index].h_cost = heuristic(start, goal);
    ctx->nodes[start_index].f_cost = ctx->nodes[start_index].h_cost;
    ctx->nodes[start_index].is_open = true;
    
    astar_heap_push(ctx, (i32)start_index);
    
    // Main A* loop
    while (ctx->open_count > 0) {
        // Get node with lowest f_cost
        i32 current_index = astar_heap_pop(ctx);
        if (current_index == -1) break;
        
        AStarPathNode* current = &ctx->nodes[current_index];
        current->is_open = false;
        current->is_closed = true;
        
        // Check if goal reached
        if (current_index == (i32)goal_index) {
            return astar_reconstruct_path(ctx, path_length);
        }
        
        // Check neighbors
        AStarNode neighbors[8];
        size_t neighbor_count;
        astar_get_neighbors(ctx, &current->position, neighbors, &neighbor_count);
        
        for (size_t i = 0; i < neighbor_count; i++) {
            size_t neighbor_index = astar_get_node_index(ctx, &neighbors[i]);
            AStarPathNode* neighbor = &ctx->nodes[neighbor_index];
            
            if (neighbor->is_closed) continue;
            
            // Calculate tentative g_cost
            bool is_diagonal = (abs(neighbors[i].x - current->position.x) + 
                               abs(neighbors[i].y - current->position.y)) == 2;
            f32 tentative_g = current->g_cost + astar_movement_cost(&current->position, &neighbors[i], is_diagonal);
            
            if (tentative_g < neighbor->g_cost) {
                neighbor->parent_index = current_index;
                neighbor->g_cost = tentative_g;
                neighbor->h_cost = heuristic(&neighbors[i], goal);
                neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                
                if (!neighbor->is_open) {
                    neighbor->is_open = true;
                    astar_heap_push(ctx, (i32)neighbor_index);
                }
            }
        }
    }
    
    // No path found
    *path_length = 0;
    return NULL;
}

// ✅ COMPLETED: Update walkability map
void astar_update_walkability(AStarContext* ctx, const bool* walkable_map) {
    if (!ctx || !walkable_map) return;
    
    for (size_t i = 0; i < ctx->total_nodes; i++) {
        ctx->nodes[i].is_walkable = walkable_map[i];
    }
}

// ✅ COMPLETED: Set individual node walkability
void astar_set_node_walkable(AStarContext* ctx, const AStarNode* pos, bool walkable) {
    if (!ctx || !pos) return;
    
    size_t index = astar_get_node_index(ctx, pos);
    if (index != SIZE_MAX) {
        ctx->nodes[index].is_walkable = walkable;
    }
}

// ✅ COMPLETED: Configuration functions
void astar_set_diagonal_movement(AStarContext* ctx, bool allow_diagonal) {
    if (ctx) {
        ctx->diagonal_movement = allow_diagonal;
    }
}

bool astar_get_diagonal_movement(const AStarContext* ctx) {
    return ctx ? ctx->diagonal_movement : false;
}

// ✅ COMPLETED: Utility functions
size_t astar_get_width(const AStarContext* ctx) {
    return ctx ? ctx->width : 0;
}

size_t astar_get_height(const AStarContext* ctx) {
    return ctx ? ctx->height : 0;
}

bool astar_is_walkable(const AStarContext* ctx, const AStarNode* pos) {
    if (!ctx || !pos) return false;
    
    size_t index = astar_get_node_index(ctx, pos);
    return (index != SIZE_MAX) ? ctx->nodes[index].is_walkable : false;
}

// ✅ COMPLETED: Path validation
bool astar_is_path_valid(const AStarContext* ctx, const AStarNode* path, size_t path_length) {
    if (!ctx || !path || path_length == 0) return false;
    
    for (size_t i = 0; i < path_length; i++) {
        if (!astar_is_walkable(ctx, &path[i])) {
            return false;
        }
    }
    
    return true;
}

// ✅ COMPLETED: Context destruction
void astar_destroy_context(AStarContext* ctx) {
    if (!ctx) return;
    
    free(ctx->nodes);
    free(ctx->open_set);
    free(ctx);
}
