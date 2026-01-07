/**
 * @file terrain_lod_manager.c
 * @brief Quadtree-based Terrain LOD.
 *
 * Manages streaming and level-of-detail for infinite terrain.
 * Splits/merges nodes based on camera distance.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <terrain/core/terrain_lod_manager.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct TerrainNode {
  struct TerrainNode *children[4]; // NW, NE, SW, SE
  struct TerrainNode *parent;
  AABB bounds;
  int lod_level;
  bool is_leaf;
  // Mesh* mesh_instance;
} TerrainNode;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

void update_node_lod(TerrainNode *node, vec3 cam_pos) {
  float size = node->bounds.max.x - node->bounds.min.x;
  float dist = vec3_distance(cam_pos, node->bounds.center);

  // Split criteria: Distance < Size * Multiplier
  bool should_split = dist < (size * 2.0f);

  if (should_split && node->lod_level < MAX_LOD) {
    if (node->is_leaf) {
      // SPIT: Create 4 children
      // node->children[0] = create_node(...)
      node->is_leaf = false;
    }

    // Recurse
    for (int i = 0; i < 4; i++)
      update_node_lod(node->children[i], cam_pos);

  } else {
    if (!node->is_leaf) {
      // MERGE: Destroy children
      // ... free children ...
      node->is_leaf = true;
    }
  }
}
