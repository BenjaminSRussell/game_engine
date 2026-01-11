// geometry/nanite/nanite_cluster.h
// Nanite-equivalent clustered geometry system
#ifndef NANITE_CLUSTER_H
#define NANITE_CLUSTER_H

#include "include/common.h"
#include "include/math/bounds.h"
#include "include/math/mat4.h"
#include "include/math/vec2.h"
#include "include/math/vec3.h"

#define NANITE_CLUSTER_SIZE 128 // Triangles per cluster
#define NANITE_MAX_LOD_LEVELS 16

// Cluster: atomic rendering unit (128 triangles)
typedef struct {
  Vec3 bounds_min;
  Vec3 bounds_max;
  Vec3 cone_apex; // For backface culling
  Vec3 cone_axis;
  f32 cone_angle;

  u32 vertex_offset;
  u32 vertex_count;
  u32 triangle_offset;
  u32 triangle_count;

  u32 lod_level;
  f32 lod_error; // Screen space error threshold
  u32 parent_cluster_id;
} NaniteCluster;

// Cluster group: LOD hierarchy node
typedef struct {
  NaniteCluster *clusters;
  u32 cluster_count;

  u32 lod_level;
  Vec3 bounds_min;
  Vec3 bounds_max;
  f32 sphere_bounds_radius;
  Vec3 sphere_bounds_center;
} NaniteClusterGroup;

// Full mesh data with LOD chain
typedef struct {
  // Vertex data (shared across all LODs)
  Vec3 *positions;
  Vec3 *normals;
  Vec2 *uvs;
  u32 vertex_count;

  // Index data per LOD
  u32 *indices;
  u32 index_count;

  // Cluster hierarchy
  NaniteCluster *clusters;
  u32 cluster_count;

  NaniteClusterGroup groups[NANITE_MAX_LOD_LEVELS];
  u32 lod_offsets[NANITE_MAX_LOD_LEVELS];
  u32 lod_count;

  // GPU buffers
  void *gpu_vertex_buffer;
  void *gpu_index_buffer;
  void *gpu_cluster_buffer;
} NaniteMesh;

#ifdef __cplusplus
extern "C" {
#endif

// Mesh conversion: traditional mesh -> clustered Nanite mesh
NaniteMesh *nanite_create_from_mesh(const Vec3 *positions, const Vec3 *normals,
                                    const Vec2 *uvs, const u32 *indices,
                                    u32 vertex_count, u32 index_count);

// Cluster generation
void nanite_generate_clusters(NaniteMesh *mesh);
void nanite_build_lod_hierarchy(NaniteMesh *mesh, u32 target_lod_levels);
void nanite_optimize_clusters(NaniteMesh *mesh); // Cache optimization

// GPU upload
void nanite_upload_to_gpu(NaniteMesh *mesh, void *device);
void nanite_free_mesh(NaniteMesh *mesh);

// Runtime culling (CPU or GPU)
void nanite_cull_clusters(const NaniteMesh *mesh, const Mat4 *view_proj,
                          const Vec3 *camera_pos, u32 *visible_clusters_out,
                          u32 *visible_count_out);

#ifdef __cplusplus
}
#endif

#endif // NANITE_CLUSTER_H
