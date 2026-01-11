// geometry/nanite/nanite_cluster.c
// Nanite cluster generation and LOD building
#include "include/geometry/nanite/nanite_cluster.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Metis graph partitioning for cluster generation (simplified)
static void partition_triangles_into_clusters(const u32 *indices,
                                              u32 triangle_count,
                                              u32 **cluster_indices,
                                              u32 *cluster_count) {
  // Simplified: just split into groups of 128 triangles
  u32 num_clusters =
      (triangle_count + NANITE_CLUSTER_SIZE - 1) / NANITE_CLUSTER_SIZE;
  *cluster_count = num_clusters;
  *cluster_indices = (u32 *)calloc(num_clusters, sizeof(u32));

  for (u32 i = 0; i < num_clusters; i++) {
    (*cluster_indices)[i] = i * NANITE_CLUSTER_SIZE;
  }
}

// Calculate cluster bounds and culling cone
static void calculate_cluster_bounds(NaniteCluster *cluster,
                                     const Vec3 *positions, const u32 *indices,
                                     u32 start_tri, u32 tri_count) {
  Vec3 min_bound = vec3(1e30f, 1e30f, 1e30f);
  Vec3 max_bound = vec3(-1e30f, -1e30f, -1e30f);

  for (u32 i = 0; i < tri_count; i++) {
    u32 base = (start_tri + i) * 3;
    for (u32 j = 0; j < 3; j++) {
      Vec3 pos = positions[indices[base + j]];
      min_bound = vec3_min(min_bound, pos);
      max_bound = vec3_max(max_bound, pos);
    }
  }

  cluster->bounds_min = min_bound;
  cluster->bounds_max = max_bound;

  // Simplified cone calculation (proper implementation uses normal cone)
  Vec3 center = vec3_mul(vec3_add(min_bound, max_bound), 0.5f);
  cluster->cone_apex = center;
  cluster->cone_axis = vec3(0, 0, 1); // Placeholder
  cluster->cone_angle = M_PI;         // Full sphere for now
}

NaniteMesh *nanite_create_from_mesh(const Vec3 *positions, const Vec3 *normals,
                                    const Vec2 *uvs, const u32 *indices,
                                    u32 vertex_count, u32 index_count) {
  NaniteMesh *mesh = (NaniteMesh *)calloc(1, sizeof(NaniteMesh));

  // Copy vertex data
  mesh->vertex_count = vertex_count;
  mesh->positions = (Vec3 *)malloc(vertex_count * sizeof(Vec3));
  mesh->normals = (Vec3 *)malloc(vertex_count * sizeof(Vec3));
  mesh->uvs = (Vec2 *)malloc(vertex_count * sizeof(Vec2));

  memcpy(mesh->positions, positions, vertex_count * sizeof(Vec3));
  memcpy(mesh->normals, normals, vertex_count * sizeof(Vec3));
  memcpy(mesh->uvs, uvs, vertex_count * sizeof(Vec2));

  // Copy indices
  mesh->index_count = index_count;
  mesh->indices = (u32 *)malloc(index_count * sizeof(u32));
  memcpy(mesh->indices, indices, index_count * sizeof(u32));

  LOG_INFO("Created Nanite mesh with %u vertices, %u triangles", vertex_count,
           index_count / 3);

  return mesh;
}

void nanite_generate_clusters(NaniteMesh *mesh) {
  u32 triangle_count = mesh->index_count / 3;

  u32 *cluster_starts;
  u32 cluster_count;
  partition_triangles_into_clusters(mesh->indices, triangle_count,
                                    &cluster_starts, &cluster_count);

  mesh->cluster_count = cluster_count;
  mesh->clusters =
      (NaniteCluster *)calloc(cluster_count, sizeof(NaniteCluster));

  for (u32 i = 0; i < cluster_count; i++) {
    u32 start_tri = cluster_starts[i];
    u32 tri_count = NANITE_CLUSTER_SIZE;
    if (start_tri + tri_count > triangle_count) {
      tri_count = triangle_count - start_tri;
    }

    NaniteCluster *cluster = &mesh->clusters[i];
    cluster->triangle_offset = start_tri * 3;
    cluster->triangle_count = tri_count;
    cluster->lod_level = 0; // Base LOD

    calculate_cluster_bounds(cluster, mesh->positions, mesh->indices, start_tri,
                             tri_count);
  }

  free(cluster_starts);

  LOG_INFO("Generated %u clusters from mesh", cluster_count);
}

void nanite_build_lod_hierarchy(NaniteMesh *mesh, u32 target_lod_levels) {
  // Simplified: each LOD level decimates geometry by ~50%
  mesh->lod_count = target_lod_levels < NANITE_MAX_LOD_LEVELS
                        ? target_lod_levels
                        : NANITE_MAX_LOD_LEVELS;

  for (u32 lod = 0; lod < mesh->lod_count; lod++) {
    NaniteClusterGroup *group = &mesh->groups[lod];
    group->lod_level = lod;

    // TODO: Implement mesh simplification per LOD
    // For now, just reference base clusters
    if (lod == 0) {
      group->clusters = mesh->clusters;
      group->cluster_count = mesh->cluster_count;
    }
  }

  LOG_INFO("Built %u LOD levels for Nanite mesh", mesh->lod_count);
}

void nanite_optimize_clusters(NaniteMesh *mesh) {
  // TODO: Implement cache-optimized vertex ordering
  // Use Forsyth algorithm or similar
  LOG_INFO("Optimizing cluster cache performance...");
}

void nanite_upload_to_gpu(NaniteMesh *mesh, void *device) {
  // TODO: Create Metal buffers and upload
  LOG_INFO("Uploading Nanite mesh to GPU");
}

void nanite_free_mesh(NaniteMesh *mesh) {
  if (!mesh)
    return;

  free(mesh->positions);
  free(mesh->normals);
  free(mesh->uvs);
  free(mesh->indices);
  free(mesh->clusters);
  free(mesh);
}

void nanite_cull_clusters(const NaniteMesh *mesh, const Mat4 *view_proj,
                          const Vec3 *camera_pos, u32 *visible_clusters_out,
                          u32 *visible_count_out) {
  // Simplified frustum + distance culling
  u32 visible_count = 0;

  for (u32 i = 0; i < mesh->cluster_count; i++) {
    const NaniteCluster *cluster = &mesh->clusters[i];

    // Calculate cluster center
    Vec3 center =
        vec3_mul(vec3_add(cluster->bounds_min, cluster->bounds_max), 0.5f);

    // Distance-based LOD selection (simplified)
    f32 dist = vec3_length(vec3_sub(center, *camera_pos));

    // Frustum cull (TODO: proper implementation)
    // For now, accept all
    visible_clusters_out[visible_count++] = i;
  }

  *visible_count_out = visible_count;
}
