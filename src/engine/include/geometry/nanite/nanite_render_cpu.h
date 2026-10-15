#ifndef NANITE_RENDER_CPU_H
#define NANITE_RENDER_CPU_H

#include "geometry/nanite/nanite_cluster.h"

// Bottom-up cluster hierarchy used by the Nanite render path.
// Parents are built only from the previous level, in groups of at most four.

typedef struct {
  u32 cluster_id;
  u32 parent_id;
  u32 *child_ids;
  u32 child_count;
  Vec3 bounds_center;
  f32 bounds_radius;
  u32 lod_level;
  f32 error_metric;
  bool is_leaf;
} ClusterHierarchyNode;

typedef struct {
  ClusterHierarchyNode *nodes;
  u32 node_count;
  u32 max_nodes;
  u32 root_node_id;
  u32 *leaf_nodes;
  u32 leaf_count;

  u32 total_clusters_processed;
  f32 build_time_seconds;
  f32 average_error;
  f32 max_error;
} ClusterHierarchyBuilder;

ClusterHierarchyBuilder *cluster_hierarchy_builder_create(u32 max_nodes);

u32 cluster_hierarchy_add_node(ClusterHierarchyBuilder *builder, u32 parent_id,
                               const Vec3 *center, f32 radius, u32 lod_level,
                               f32 error);

void cluster_hierarchy_build(ClusterHierarchyBuilder *builder, NaniteMesh *mesh);

void cluster_hierarchy_destroy(ClusterHierarchyBuilder *builder);

// Cluster range for one LOD. lod 0 is the finest. Counts never wrap.
void nanite_lod_cluster_range(const NaniteMesh *mesh, u32 lod, u32 *start_out,
                              u32 *count_out);

// Coarsest LOD whose max projected error is within error_threshold_px.
// Falls back to lod 0 when every coarser level is too coarse, or when
// mesh / camera inputs cannot be projected.
u32 nanite_select_lod(const NaniteMesh *mesh, const Vec3 *camera_pos,
                      f32 error_threshold_px, f32 view_height_px);

#endif // NANITE_RENDER_CPU_H
