// src/engine/ai/nav/nav_mesh.c
#include "include/ai/nav/nav_mesh.h"
#include "include/core/logger.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

NavMesh *nav_mesh_create(u32 width, u32 height, u32 depth, f32 cell_size,
                         Vec3 origin) {
  NavMesh *mesh = (NavMesh *)calloc(1, sizeof(NavMesh));
  mesh->width = width;
  mesh->height = height;
  mesh->depth = depth;
  mesh->cell_size = cell_size;
  mesh->origin = origin;

  mesh->capacity = width * height * depth;
  mesh->nodes = (NavMeshNode *)calloc(mesh->capacity, sizeof(NavMeshNode));
  mesh->node_count = mesh->capacity;

  // Initialize nodes
  for (u32 x = 0; x < width; x++) {
    for (u32 y = 0; y < height; y++) {
      for (u32 z = 0; z < depth; z++) {
        u32 idx = (z * width * height) + (y * width) + x;
        mesh->nodes[idx].id = idx;
        mesh->nodes[idx].walkable = true; // Default
        mesh->nodes[idx].cost_multiplier = 1.0f;

        mesh->nodes[idx].position =
            (Vec3){origin.x + x * cell_size + cell_size * 0.5f,
                   origin.y + y * cell_size + cell_size * 0.5f,
                   origin.z + z * cell_size + cell_size * 0.5f};
      }
    }
  }

  return mesh;
}

void nav_mesh_destroy(NavMesh *mesh) {
  if (!mesh)
    return;
  if (mesh->nodes)
    free(mesh->nodes);
  free(mesh);
}

void nav_mesh_mark_walkable(NavMesh *mesh, u32 x, u32 y, u32 z, bool walkable) {
  if (!mesh)
    return;
  if (x >= mesh->width || y >= mesh->height || z >= mesh->depth)
    return;
  u32 idx = (z * mesh->width * mesh->height) + (y * mesh->width) + x;
  mesh->nodes[idx].walkable = walkable;
}

void nav_mesh_generate_links(NavMesh *mesh) {
  if (!mesh)
    return;

  // Simple 6-neighbor grid logic
  i32 offsets[6][3] = {{-1, 0, 0}, {1, 0, 0},  {0, -1, 0},
                       {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};

  for (u32 x = 0; x < mesh->width; x++) {
    for (u32 y = 0; y < mesh->height; y++) {
      for (u32 z = 0; z < mesh->depth; z++) {
        u32 idx = (z * mesh->width * mesh->height) + (y * mesh->width) + x;
        NavMeshNode *node = &mesh->nodes[idx];

        if (!node->walkable)
          continue;

        node->neighbor_count = 0;
        for (int i = 0; i < 6; i++) {
          i32 nx = (i32)x + offsets[i][0];
          i32 ny = (i32)y + offsets[i][1];
          i32 nz = (i32)z + offsets[i][2];

          if (nx >= 0 && nx < (i32)mesh->width && ny >= 0 &&
              ny < (i32)mesh->height && nz >= 0 && nz < (i32)mesh->depth) {

            u32 target_idx =
                (nz * mesh->width * mesh->height) + (ny * mesh->width) + nx;
            if (mesh->nodes[target_idx].walkable) {
              node->neighbors[node->neighbor_count++] = target_idx;
            }
          }
        }
      }
    }
  }
}

// Simple A* Priority Queue entry
typedef struct {
  u32 node_idx;
  f32 f_score;
} PQEntry;

// Very basic A* implementation for brevity
bool nav_mesh_find_path(NavMesh *mesh, Vec3 start, Vec3 end,
                        NavPath *out_path) {
  if (!mesh || !out_path)
    return false;

  out_path->count = 0;

  // Quantize start/end to grid
  i32 sx = (i32)((start.x - mesh->origin.x) / mesh->cell_size);
  i32 sy = (i32)((start.y - mesh->origin.y) / mesh->cell_size);
  i32 sz = (i32)((start.z - mesh->origin.z) / mesh->cell_size);

  i32 ex = (i32)((end.x - mesh->origin.x) / mesh->cell_size);
  i32 ey = (i32)((end.y - mesh->origin.y) / mesh->cell_size);
  i32 ez = (i32)((end.z - mesh->origin.z) / mesh->cell_size);

  if (sx < 0 || sx >= mesh->width || sy < 0 || sy >= mesh->height || sz < 0 ||
      sz >= mesh->depth)
    return false;
  if (ex < 0 || ex >= mesh->width || ey < 0 || ey >= mesh->height || ez < 0 ||
      ez >= mesh->depth)
    return false;

  u32 start_node = (sz * mesh->width * mesh->height) + (sy * mesh->width) + sx;
  u32 end_node = (ez * mesh->width * mesh->height) + (ey * mesh->width) + ex;

  if (!mesh->nodes[start_node].walkable || !mesh->nodes[end_node].walkable)
    return false;

  // Setup A* structures
  u32 *came_from = (u32 *)malloc(mesh->capacity * sizeof(u32));
  f32 *g_score = (f32 *)malloc(mesh->capacity * sizeof(f32));
  f32 *f_score = (f32 *)malloc(mesh->capacity * sizeof(f32));

  if (!came_from || !g_score || !f_score) {
    free(came_from);
    free(g_score);
    free(f_score);
    return false;
  }

  for (u32 i = 0; i < mesh->capacity; i++) {
    g_score[i] = FLT_MAX;
    f_score[i] = FLT_MAX;
  }

  g_score[start_node] = 0;
  f_score[start_node] = fabsf(end.x - start.x) + fabsf(end.y - start.y) +
                        fabsf(end.z - start.z); // Heuristic

  // Open set (simple array for now, O(n) search)
  // For production, use a heap
  u32 open_set[1024];
  u32 open_count = 0;
  open_set[open_count++] = start_node;

  bool found = false;

  while (open_count > 0) {
    // Find lowest F
    u32 current = open_set[0];
    u32 current_idx = 0;
    for (u32 i = 1; i < open_count; i++) {
      if (f_score[open_set[i]] < f_score[current]) {
        current = open_set[i];
        current_idx = i;
      }
    }

    if (current == end_node) {
      found = true;
      break;
    }

    // Remove current
    open_set[current_idx] = open_set[--open_count];

    // Neighbors
    NavMeshNode *node = &mesh->nodes[current];
    for (u32 i = 0; i < node->neighbor_count; i++) {
      u32 neighbor = node->neighbors[i];

      f32 tent_g = g_score[current] +
                   mesh->cell_size * mesh->nodes[neighbor].cost_multiplier;

      if (tent_g < g_score[neighbor]) {
        came_from[neighbor] = current;
        g_score[neighbor] = tent_g;

        NavMeshNode *n_ptr = &mesh->nodes[neighbor];
        f32 dist = fabsf((f32)ex - (i32)((n_ptr->position.x - mesh->origin.x) /
                                         mesh->cell_size)) +
                   fabsf((f32)ey - (i32)((n_ptr->position.y - mesh->origin.y) /
                                         mesh->cell_size)) +
                   fabsf((f32)ez - (i32)((n_ptr->position.z - mesh->origin.z) /
                                         mesh->cell_size));

        f_score[neighbor] = g_score[neighbor] + dist * mesh->cell_size;

        bool in_open = false;
        for (u32 k = 0; k < open_count; k++)
          if (open_set[k] == neighbor)
            in_open = true;
        if (!in_open && open_count < 1024) {
          open_set[open_count++] = neighbor;
        }
      }
    }
  }

  if (found) {
    // Reconstruct path
    u32 current = end_node;
    u32 path_nodes[MAX_PATH_NODES];
    u32 p_len = 0;

    while (current != start_node) {
      if (p_len < MAX_PATH_NODES)
        path_nodes[p_len++] = current;
      current = came_from[current];
    }
    // path_nodes[p_len++] = start_node; // optional

    out_path->count = p_len;
    for (u32 i = 0; i < p_len; i++) {
      out_path->points[i] = mesh->nodes[path_nodes[p_len - 1 - i]].position;
    }
  }

  free(came_from);
  free(g_score);
  free(f_score);
  return found;
}
