// geometry/nanite/nanite_cluster.c
// Nanite cluster generation and LOD building
#include "include/geometry/nanite/nanite_cluster.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef __OBJC__
#import <Metal/Metal.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

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

  if (mesh->lod_count == 0 || !mesh->clusters) {
    return;
  }

  mesh->lod_offsets[0] = 0;
  mesh->groups[0].lod_level = 0;
  mesh->groups[0].clusters = mesh->clusters;
  mesh->groups[0].cluster_count = mesh->cluster_count;

  u32 total_clusters = mesh->cluster_count;
  for (u32 lod = 1; lod < mesh->lod_count; lod++) {
    NaniteClusterGroup *prev_group = &mesh->groups[lod - 1];
    NaniteClusterGroup *group = &mesh->groups[lod];
    u32 prev_count = prev_group->cluster_count;
    u32 new_count = (prev_count + 1) / 2;
    if (new_count == 0) {
      break;
    }

    mesh->clusters = (NaniteCluster *)realloc(
        mesh->clusters, (total_clusters + new_count) * sizeof(NaniteCluster));
    if (!mesh->clusters) {
      mesh->lod_count = lod;
      return;
    }

    NaniteCluster *new_clusters = &mesh->clusters[total_clusters];
    memset(new_clusters, 0, new_count * sizeof(NaniteCluster));

    for (u32 i = 0; i < new_count; i++) {
      u32 child0 = i * 2;
      u32 child1 = child0 + 1;
      const NaniteCluster *c0 = &prev_group->clusters[child0];
      const NaniteCluster *c1 =
          (child1 < prev_count) ? &prev_group->clusters[child1] : c0;

      NaniteCluster *parent = &new_clusters[i];
      parent->lod_level = lod;
      parent->triangle_offset = c0->triangle_offset;

      u32 combined_tris = c0->triangle_count + c1->triangle_count;
      u32 simplified_tris = combined_tris / 2;
      if (simplified_tris == 0) {
        simplified_tris = c0->triangle_count > 0 ? c0->triangle_count : 1;
      }
      parent->triangle_count = simplified_tris;

      parent->bounds_min = vec3_min(c0->bounds_min, c1->bounds_min);
      parent->bounds_max = vec3_max(c0->bounds_max, c1->bounds_max);

      parent->cone_apex = vec3_mul(vec3_add(parent->bounds_min, parent->bounds_max), 0.5f);
      parent->cone_axis = c0->cone_axis;
      parent->cone_angle = fminf((c0->cone_angle + c1->cone_angle) * 0.5f, (f32)M_PI);

      parent->lod_error = fmaxf(c0->lod_error, c1->lod_error) * 1.5f;
      parent->parent_cluster_id = 0xFFFFFFFF;
    }

    mesh->lod_offsets[lod] = total_clusters;
    group->lod_level = lod;
    group->clusters = new_clusters;
    group->cluster_count = new_count;

    total_clusters += new_count;
  }

  mesh->cluster_count = total_clusters;
  for (u32 lod = 0; lod < mesh->lod_count; lod++) {
    mesh->groups[lod].clusters = &mesh->clusters[mesh->lod_offsets[lod]];
  }

  LOG_INFO("Built %u LOD levels for Nanite mesh", mesh->lod_count);
}

void nanite_optimize_clusters(NaniteMesh *mesh) {
  if (!mesh || !mesh->positions || !mesh->indices || mesh->vertex_count == 0 ||
      mesh->index_count == 0) {
    return;
  }

  u32 *remap = (u32 *)malloc(mesh->vertex_count * sizeof(u32));
  if (!remap) {
    return;
  }
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    remap[i] = UINT32_MAX;
  }

  u32 new_vertex_count = 0;
  for (u32 i = 0; i < mesh->index_count; i++) {
    u32 idx = mesh->indices[i];
    if (idx < mesh->vertex_count && remap[idx] == UINT32_MAX) {
      remap[idx] = new_vertex_count++;
    }
  }

  Vec3 *new_positions = (Vec3 *)malloc(new_vertex_count * sizeof(Vec3));
  Vec3 *new_normals = (Vec3 *)malloc(new_vertex_count * sizeof(Vec3));
  Vec2 *new_uvs = (Vec2 *)malloc(new_vertex_count * sizeof(Vec2));
  if (!new_positions || !new_normals || !new_uvs) {
    free(remap);
    free(new_positions);
    free(new_normals);
    free(new_uvs);
    return;
  }

  for (u32 old = 0; old < mesh->vertex_count; old++) {
    u32 mapped = remap[old];
    if (mapped != UINT32_MAX) {
      new_positions[mapped] = mesh->positions[old];
      new_normals[mapped] = mesh->normals[old];
      new_uvs[mapped] = mesh->uvs[old];
    }
  }

  for (u32 i = 0; i < mesh->index_count; i++) {
    mesh->indices[i] = remap[mesh->indices[i]];
  }

  free(mesh->positions);
  free(mesh->normals);
  free(mesh->uvs);

  mesh->positions = new_positions;
  mesh->normals = new_normals;
  mesh->uvs = new_uvs;
  mesh->vertex_count = new_vertex_count;

  for (u32 i = 0; i < mesh->cluster_count; i++) {
    mesh->clusters[i].vertex_offset = 0;
    mesh->clusters[i].vertex_count = mesh->vertex_count;
  }

  free(remap);

  LOG_INFO("Optimized Nanite vertex ordering (vertices: %u)", new_vertex_count);
}

void nanite_upload_to_gpu(NaniteMesh *mesh, void *device) {
  if (!mesh) {
    return;
  }

  if (mesh->gpu_vertex_buffer) {
#ifdef __OBJC__
    CFRelease(mesh->gpu_vertex_buffer);
#else
    free(mesh->gpu_vertex_buffer);
#endif
    mesh->gpu_vertex_buffer = NULL;
  }
  if (mesh->gpu_index_buffer) {
#ifdef __OBJC__
    CFRelease(mesh->gpu_index_buffer);
#else
    free(mesh->gpu_index_buffer);
#endif
    mesh->gpu_index_buffer = NULL;
  }
  if (mesh->gpu_cluster_buffer) {
#ifdef __OBJC__
    CFRelease(mesh->gpu_cluster_buffer);
#else
    free(mesh->gpu_cluster_buffer);
#endif
    mesh->gpu_cluster_buffer = NULL;
  }

#ifdef __OBJC__
  if (device) {
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)device;
    id<MTLBuffer> vbuf = [mtl_device newBufferWithBytes:mesh->positions
                                                 length:mesh->vertex_count * sizeof(Vec3)
                                                options:MTLResourceStorageModeShared];
    id<MTLBuffer> ibuf = [mtl_device newBufferWithBytes:mesh->indices
                                                 length:mesh->index_count * sizeof(u32)
                                                options:MTLResourceStorageModeShared];
    id<MTLBuffer> cbuf = [mtl_device newBufferWithBytes:mesh->clusters
                                                 length:mesh->cluster_count * sizeof(NaniteCluster)
                                                options:MTLResourceStorageModeShared];

    mesh->gpu_vertex_buffer = (__bridge_retained void *)vbuf;
    mesh->gpu_index_buffer = (__bridge_retained void *)ibuf;
    mesh->gpu_cluster_buffer = (__bridge_retained void *)cbuf;
  }
#else
  (void)device;
  mesh->gpu_vertex_buffer = malloc(mesh->vertex_count * sizeof(Vec3));
  mesh->gpu_index_buffer = malloc(mesh->index_count * sizeof(u32));
  mesh->gpu_cluster_buffer = malloc(mesh->cluster_count * sizeof(NaniteCluster));
  if (mesh->gpu_vertex_buffer) {
    memcpy(mesh->gpu_vertex_buffer, mesh->positions,
           mesh->vertex_count * sizeof(Vec3));
  }
  if (mesh->gpu_index_buffer) {
    memcpy(mesh->gpu_index_buffer, mesh->indices,
           mesh->index_count * sizeof(u32));
  }
  if (mesh->gpu_cluster_buffer) {
    memcpy(mesh->gpu_cluster_buffer, mesh->clusters,
           mesh->cluster_count * sizeof(NaniteCluster));
  }
#endif

  LOG_INFO("Uploaded Nanite mesh to GPU buffers");
}

void nanite_free_mesh(NaniteMesh *mesh) {
  if (!mesh)
    return;

  free(mesh->positions);
  free(mesh->normals);
  free(mesh->uvs);
  free(mesh->indices);
  free(mesh->clusters);
#ifdef __OBJC__
  if (mesh->gpu_vertex_buffer) {
    CFRelease(mesh->gpu_vertex_buffer);
  }
  if (mesh->gpu_index_buffer) {
    CFRelease(mesh->gpu_index_buffer);
  }
  if (mesh->gpu_cluster_buffer) {
    CFRelease(mesh->gpu_cluster_buffer);
  }
#else
  free(mesh->gpu_vertex_buffer);
  free(mesh->gpu_index_buffer);
  free(mesh->gpu_cluster_buffer);
#endif
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
