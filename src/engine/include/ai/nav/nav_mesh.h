#ifndef NAV_MESH_H
#define NAV_MESH_H

#include "include/common.h"
#include "math/vec3.h"

// Max neighbors in a 3D grid (26) or simplified (6)
#define MAX_NAV_NEIGHBORS 6
#define MAX_PATH_NODES 512

typedef struct NavMeshNode {
  Vec3 position;
  u32 id;
  u32 neighbor_count;
  u32 neighbors[MAX_NAV_NEIGHBORS];
  f32 cost_multiplier;
  bool walkable;
} NavMeshNode;

typedef struct {
  NavMeshNode *nodes;
  u32 node_count;
  u32 capacity;

  // Grid settings
  Vec3 origin;
  f32 cell_size;
  u32 width, height, depth;
} NavMesh;

typedef struct {
  Vec3 points[MAX_PATH_NODES];
  u32 count;
} NavPath;

#ifdef __cplusplus
extern "C" {
#endif

NavMesh *nav_mesh_create(u32 width, u32 height, u32 depth, f32 cell_size,
                         Vec3 origin);
void nav_mesh_destroy(NavMesh *mesh);

// Build/Bake
void nav_mesh_mark_walkable(NavMesh *mesh, u32 x, u32 y, u32 z, bool walkable);
void nav_mesh_generate_links(NavMesh *mesh);

// Runtime
bool nav_mesh_find_path(NavMesh *mesh, Vec3 start, Vec3 end, NavPath *out_path);

#ifdef __cplusplus
}
#endif

#endif
