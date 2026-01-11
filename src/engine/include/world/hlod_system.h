// World/HLOD System (Hierarchical Level of Detail)
#ifndef HLOD_SYSTEM_H
#define HLOD_SYSTEM_H

#include "include/common.h"
#include "include/math/bounds.h"
#include "include/math/vec3.h"
#import <Metal/Metal.h>

#define HLOD_MAX_LEVELS 4
#define HLOD_MAX_CHILDREN 8

// HLOD Node in the hierarchy
typedef struct HLODNode {
  u32 id;
  u32 level; // 0 is highest detail (leaf), increasing numbers are simpler

  BoundingBox bounds;

  // If this is a leaf (level 0), it references original mesh instances
  u32 *instance_ids;
  u32 instance_count;

  // If this is an HLOD (level > 0), it has a proxy mesh
  id<MTLBuffer> proxy_vertex_buffer;
  id<MTLBuffer> proxy_index_buffer;
  id<MTLTexture> proxy_albedo_atlas;
  id<MTLTexture> proxy_normal_atlas;
  u32 proxy_index_count;
  bool is_streaming;
  bool is_loaded;

  // Hierarchy
  struct HLODNode *parent;
  struct HLODNode *children[HLOD_MAX_CHILDREN];
  u32 child_count;

  f32 switch_distance; // Distance to switch to this LOD

} HLODNode;

typedef struct {
  HLODNode *root;
  u32 max_depth;

  // Streaming logic
  f32 loading_range;
  bool enable_streaming;

  id<MTLDevice> device;

} HLODSystem;

#ifdef __cplusplus
extern "C" {
#endif

HLODSystem *hlod_create(id<MTLDevice> device);
void hlod_destroy(HLODSystem *system);

// Building (Offline or Editor)
void hlod_build_structure(HLODSystem *system, const void *world_entities,
                          u32 count);
void hlod_generate_proxy_mesh(HLODSystem *system,
                              HLODNode *node); // Merge meshes, bake textures

// Runtime
void hlod_update(HLODSystem *system, Vec3 camera_pos);
void hlod_render(HLODSystem *system, id<MTLRenderCommandEncoder> encoder,
                 const void *frustum);

// Streaming
void hlod_stream_node(HLODSystem *system, HLODNode *node);
void hlod_unload_node(HLODSystem *system, HLODNode *node);

#ifdef __cplusplus
}
#endif

#endif // HLOD_SYSTEM_H
