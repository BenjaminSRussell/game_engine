#include "cluster_builder.h"
#include <stdlib.h>
#include <string.h>

cluster_mesh_t *cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t *vertices,
                                   uint32_t vertex_count,
                                   const uint32_t *indices,
                                   uint32_t index_count) {
  if (!device || !vertices || !indices || index_count == 0)
    return NULL;

  cluster_mesh_t *mesh = (cluster_mesh_t *)calloc(1, sizeof(cluster_mesh_t));
  if (!mesh)
    return NULL;

  uint32_t triangle_count = index_count / 3;
  uint32_t cluster_count =
      (triangle_count + CLUSTER_TRIANGLE_COUNT - 1) / CLUSTER_TRIANGLE_COUNT;

  mesh->cluster_count = cluster_count;
  mesh->total_triangles = triangle_count;

  // Allocate cluster descriptors
  mesh_cluster_t *clusters =
      (mesh_cluster_t *)calloc(cluster_count, sizeof(mesh_cluster_t));

  for (uint32_t i = 0; i < cluster_count; i++) {
    uint32_t tri_start = i * CLUSTER_TRIANGLE_COUNT;
    uint32_t tri_end = tri_start + CLUSTER_TRIANGLE_COUNT;
    if (tri_end > triangle_count)
      tri_end = triangle_count;

    mesh_cluster_t *cluster = &clusters[i];
    cluster->index_offset = tri_start * 3;
    cluster->triangle_count = tri_end - tri_start;

    // Calculate bounds (simplified)
    simd_float3 min_p = {1e30f, 1e30f, 1e30f};
    simd_float3 max_p = {-1e30f, -1e30f, -1e30f};

    for (uint32_t t = tri_start; t < tri_end; t++) {
      for (uint32_t v = 0; v < 3; v++) {
        uint32_t idx = indices[t * 3 + v];
        Vec3 v_pos = vertices[idx].position;
        simd_float3 p = simd_make_float3(v_pos.x, v_pos.y, v_pos.z);
        min_p = simd_min(min_p, p);
        max_p = simd_max(max_p, p);
      }
    }

    cluster->bounds_center = (min_p + max_p) * 0.5f;
    cluster->bounds_extent = (max_p - min_p) * 0.5f;
    cluster->lod_error = 0.0f; // Simplified
  }

  // Create Metal buffers
  mesh->vertex_buffer =
      [device newBufferWithBytes:vertices
                          length:vertex_count * sizeof(vertex_t)
                         options:MTLResourceStorageModeShared];

  mesh->index_buffer = [device newBufferWithBytes:indices
                                           length:index_count * sizeof(uint32_t)
                                          options:MTLResourceStorageModeShared];

  mesh->cluster_buffer =
      [device newBufferWithBytes:clusters
                          length:cluster_count * sizeof(mesh_cluster_t)
                         options:MTLResourceStorageModeShared];

  free(clusters);

  return mesh;
}

void cluster_mesh_free(cluster_mesh_t *mesh) {
  if (!mesh)
    return;
  mesh->vertex_buffer = nil;
  mesh->index_buffer = nil;
  mesh->cluster_buffer = nil;
  free(mesh);
}
