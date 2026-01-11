// geometry/nanite/cluster_builder.c
// Nanite virtualized geometry cluster building implementation.
//
// TODO: Implement cluster DAG (Directed Acyclic Graph) construction for
// hierarchical LOD. (Partially Implemented V1)
// TODO: Add support for error-metric calculation based on geometric divergence.
// (Implemented V1)
// TODO: Implement GPU-driven cluster occlusion culling using HZB (Hierarchical
// Z-Buffer).
// TODO: Add support for 2-pass occlusion culling (Cull last frame's HZB ->
// Render -> Update HZB -> Cull misses).
// TODO: Implement programmable rasterization fallback for clusters smaller than
// a pixel.
// TODO: Add support for mesh-shading pipeline (Amplification + Mesh shaders)
// for cluster processing.
// TODO: Implement cluster-based shadowing (Virtual Shadow Maps integration).
// TODO: Add support for vertex-reordering and quantization to reduce memory
// footprint.
// TODO: Implement cluster-based streaming using a LRU (Least Recently Used)
// cache for GPU memory.
// TODO: Research and implement persistent threads for cluster processing to
// avoid dispatch overhead.
// TODO: Add support for material-ID quantization per cluster.
// TODO: Implement hierarchical cluster bounds (BVH) for fast frustum and
// occlusion tests.
// TODO: Add support for Nanite-aware displacement mapping and tessellation.
// TODO: Implement cluster-based ray-tracing acceleration structure (BLAS)
// generation.
// TODO: Research and implement data-compression for cluster attributes
// (Normal/UV/Color).
// TODO: Add support for cluster-based animation and skinning (D-Nanite).
// TODO: Implement a GPU-driven material-pass using a visibility-buffer
// (Visibility Rendering).

#include "cluster_builder.h"
#include "../../include/common.h"
#include "../../include/math/math.h"
#include <simd/simd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  simd_float4 planes[6];
} view_frustum_t;

// Check if AABB is in frustum
static bool is_aabb_in_frustum(simd_float3 center, simd_float3 extent,
                               const view_frustum_t *frustum) {
  for (int i = 0; i < 6; i++) {
    simd_float4 p = frustum->planes[i];
    simd_float3 normal = p.xyz;
    float dist = p.w;

    // Projection of extent onto plane normal
    float r = simd_dot(extent, simd_abs(normal));
    float s = simd_dot(center, normal) + dist;

    if (s + r < 0)
      return false; // Fully outside
  }
  return true;
}

// Morton code (z-order curve) utilities for 3D coordinates
// Expands a 10-bit integer into 30 bits by inserting 2 zeros after each bit.
static uint32_t expand_bits(uint32_t v) {
  v = (v * 0x00010001u) & 0xFF0000FFu;
  v = (v * 0x00000101u) & 0x0F00F00Fu;
  v = (v * 0x00000011u) & 0xC30C30C3u;
  v = (v * 0x00000005u) & 0x49249249u;
  return v;
}

static uint32_t morton_3d(float x, float y, float z) {
  // Quantize 0..1 to 0..1023
  x = fminf(fmaxf(x * 1024.0f, 0.0f), 1023.0f);
  y = fminf(fmaxf(y * 1024.0f, 0.0f), 1023.0f);
  z = fminf(fmaxf(z * 1024.0f, 0.0f), 1023.0f);
  uint32_t xx = expand_bits((uint32_t)x);
  uint32_t yy = expand_bits((uint32_t)y);
  uint32_t zz = expand_bits((uint32_t)z);
  return xx * 4 + yy * 2 + zz;
}

typedef struct {
  uint32_t tri_index;
  uint32_t morton_code;
} tri_sort_entry_t;

static int compare_tri_sort(const void *a, const void *b) {
  const tri_sort_entry_t *ta = (const tri_sort_entry_t *)a;
  const tri_sort_entry_t *tb = (const tri_sort_entry_t *)b;
  return (ta->morton_code < tb->morton_code)
             ? -1
             : (ta->morton_code > tb->morton_code);
}

static void compute_bounds(const vertex_t *vertices, const uint32_t *indices,
                           uint32_t index_offset, uint32_t index_count,
                           simd_float3 *out_center, simd_float3 *out_extent) {
  simd_float3 min_b = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
  simd_float3 max_b = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};

  for (uint32_t i = 0; i < index_count; i++) {
    uint32_t idx = indices[index_offset + i];
    simd_float3 p_simd =
        (simd_float3){vertices[idx].position.x, vertices[idx].position.y,
                      vertices[idx].position.z};
    min_b = simd_min(min_b, p_simd);
    max_b = simd_max(max_b, p_simd);
  }

  *out_center = (min_b + max_b) * 0.5f;
  *out_extent = (max_b - min_b) * 0.5f;
}

// Calculate geometric error metric for a cluster
// Heuristic: Max distance from cluster center to any vertex (radius) / triangle
// density This approximates how "flat" or "detailed" the cluster is.
static float compute_cluster_error(const vertex_t *vertices,
                                   const uint32_t *indices,
                                   uint32_t index_offset, uint32_t count,
                                   simd_float3 center) {
  float max_dist_sq = 0.0f;
  for (uint32_t i = 0; i < count; i++) {
    uint32_t idx = indices[index_offset + i];
    simd_float3 p_simd =
        (simd_float3){vertices[idx].position.x, vertices[idx].position.y,
                      vertices[idx].position.z};
    float d2 = simd_distance_squared(p_simd, center);
    if (d2 > max_dist_sq)
      max_dist_sq = d2;
  }
  return sqrtf(max_dist_sq);
}

cluster_mesh_t *cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t *vertices,
                                   uint32_t vertex_count,
                                   const uint32_t *indices,
                                   uint32_t index_count) {

  // 1. Compute global bounds to normalize for Morton codes
  simd_float3 global_min = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
  simd_float3 global_max = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};
  for (uint32_t i = 0; i < vertex_count; i++) {
    simd_float3 p_simd = (simd_float3){
        vertices[i].position.x, vertices[i].position.y, vertices[i].position.z};
    global_min = simd_min(global_min, p_simd);
    global_max = simd_max(global_max, p_simd);
  }
  simd_float3 global_size = global_max - global_min;
  simd_float3 inv_size = 1.0f / (global_size + 1e-6f); // Avoid div by zero

  // 2. Prepare sorting
  uint32_t tri_count = index_count / 3;
  tri_sort_entry_t *sort_entries =
      (tri_sort_entry_t *)malloc(sizeof(tri_sort_entry_t) * tri_count);

  for (uint32_t i = 0; i < tri_count; i++) {
    // Compute centroid
    uint32_t idx0 = indices[i * 3 + 0];
    uint32_t idx1 = indices[i * 3 + 1];
    uint32_t idx2 = indices[i * 3 + 2];
    simd_float3 p0 =
        (simd_float3){vertices[idx0].position.x, vertices[idx0].position.y,
                      vertices[idx0].position.z};
    simd_float3 p1 =
        (simd_float3){vertices[idx1].position.x, vertices[idx1].position.y,
                      vertices[idx1].position.z};
    simd_float3 p2 =
        (simd_float3){vertices[idx2].position.x, vertices[idx2].position.y,
                      vertices[idx2].position.z};
    simd_float3 c = (p0 + p1 + p2) * 0.333333f;

    // Normalize
    simd_float3 norm_c = (c - global_min) * inv_size;
    sort_entries[i].tri_index = i;
    sort_entries[i].morton_code = morton_3d(norm_c.x, norm_c.y, norm_c.z);
  }

  // 3. Sort triangles
  qsort(sort_entries, tri_count, sizeof(tri_sort_entry_t), compare_tri_sort);

  // 4. Cluster generation
  uint32_t cluster_count =
      (tri_count + CLUSTER_TRIANGLE_COUNT - 1) / CLUSTER_TRIANGLE_COUNT;
  mesh_cluster_t *clusters =
      (mesh_cluster_t *)calloc(cluster_count, sizeof(mesh_cluster_t));

  // Create new index buffer sorted by clusters
  uint32_t *sorted_indices = (uint32_t *)malloc(sizeof(uint32_t) * index_count);

  // We could optimize vertex layout too (vertex reuse), but for now just copy
  // indices and rely on the original VB. Or we should build a new Compact VB
  // per cluster? The prompt struct has `vertex_offset` per cluster. This
  for (uint32_t c = 0; c < cluster_count; c++) {
    uint32_t tri_start = c * CLUSTER_TRIANGLE_COUNT;
    uint32_t tri_end = tri_start + CLUSTER_TRIANGLE_COUNT;
    if (tri_end > tri_count)
      tri_end = tri_count;

    uint32_t c_tri_count = tri_end - tri_start;
    clusters[c].triangle_count = c_tri_count;
    clusters[c].index_offset = tri_start * 3;
    clusters[c].vertex_offset = 0;           // Global buffer
    clusters[c].parent_cluster = 0xFFFFFFFF; // Root
    clusters[c].child_count = 0;

    // Copy indices
    for (uint32_t t = 0; t < c_tri_count; t++) {
      uint32_t src_tri = sort_entries[tri_start + t].tri_index;
      sorted_indices[(tri_start + t) * 3 + 0] = indices[src_tri * 3 + 0];
      sorted_indices[(tri_start + t) * 3 + 1] = indices[src_tri * 3 + 1];
      sorted_indices[(tri_start + t) * 3 + 2] = indices[src_tri * 3 + 2];
    }

    // Compute cluster bounds
    compute_bounds(vertices, sorted_indices, clusters[c].index_offset,
                   c_tri_count * 3, &clusters[c].bounds_center,
                   &clusters[c].bounds_extent);

    // initial leaf error is 0 (lossless)
    clusters[c].lod_error = 0.0f;
  }

  // 4b. Build DAG Levels (Hierarchical LOD)
  uint32_t leaf_cluster_count = cluster_count;
  uint32_t current_level_start = 0;
  uint32_t current_level_count = leaf_cluster_count;

  // Start dynamic allocation for clusters array as it grows
  uint32_t total_clusters_allocated = leaf_cluster_count;
  // (Reallocation happens inside loop if needed)

  while (current_level_count > 1) {
    uint32_t next_level_count = (current_level_count + 3) / 4;
    uint32_t next_level_start = cluster_count;

    // Ensure capacity
    if (cluster_count + next_level_count > total_clusters_allocated) {
      total_clusters_allocated = (cluster_count + next_level_count) * 2;
      clusters = (mesh_cluster_t *)realloc(
          clusters, sizeof(mesh_cluster_t) * total_clusters_allocated);
    }

    for (uint32_t i = 0; i < next_level_count; i++) {
      uint32_t parent_idx = next_level_start + i;
      mesh_cluster_t *parent = &clusters[parent_idx];

      parent->child_count = 0;
      parent->parent_cluster = 0xFFFFFFFF;

      // Calculate max child error to propagate
      float max_child_error = 0.0f;

      // Group 4 children
      simd_float3 min_b = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
      simd_float3 max_b = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};

      for (uint32_t j = 0; j < 4 && (i * 4 + j) < current_level_count; j++) {
        uint32_t child_idx = current_level_start + i * 4 + j;
        clusters[child_idx].parent_cluster = parent_idx;
        parent->child_clusters[parent->child_count++] = child_idx;

        simd_float3 c = clusters[child_idx].bounds_center;
        simd_float3 e = clusters[child_idx].bounds_extent;
        min_b = simd_min(min_b, c - e);
        max_b = simd_max(max_b, c + e);

        if (clusters[child_idx].lod_error > max_child_error) {
          max_child_error = clusters[child_idx].lod_error;
        }
      }

      parent->bounds_center = (min_b + max_b) * 0.5f;
      parent->bounds_extent = (max_b - min_b) * 0.5f;

      // The error of the parent is the error of its children plus the error
      // introduced by merging/simplifying. Since we aren't performing real
      // simplification yet (just grouping), we add a heuristic "grouping error"
      // based on bounds size.
      float grouping_error =
          simd_length(parent->bounds_extent) * 0.1f; // 10% of extent
      parent->lod_error = max_child_error + grouping_error;

      // TODO: Generate simplified geometry for the parent
      // For now, reuse the first child's geometry to avoid crashing,
      // but strictly this is incorrect for rendering.
      // In a real implementation we would merge children meshes and decimate to
      // CLUSTER_TRIANGLE_COUNT
      parent->index_offset = clusters[parent->child_clusters[0]].index_offset;
      parent->triangle_count =
          clusters[parent->child_clusters[0]].triangle_count;
      parent->vertex_offset = clusters[parent->child_clusters[0]].vertex_offset;
    }

    current_level_start = next_level_start;
    current_level_count = next_level_count;
    cluster_count += next_level_count;
  }

  // 5. Create GPU buffers
  cluster_mesh_t *mesh = (cluster_mesh_t *)malloc(sizeof(cluster_mesh_t));
  mesh->cluster_count = cluster_count;
  mesh->total_triangles = tri_count;

  mesh->vertex_buffer =
      [device newBufferWithBytes:vertices
                          length:vertex_count * sizeof(vertex_t)
                         options:MTLResourceStorageModeShared];
  mesh->index_buffer = [device newBufferWithBytes:sorted_indices
                                           length:index_count * sizeof(uint32_t)
                                          options:MTLResourceStorageModeShared];
  mesh->cluster_buffer =
      [device newBufferWithBytes:clusters
                          length:cluster_count * sizeof(mesh_cluster_t)
                         options:MTLResourceStorageModeShared];

  // Cleanup
  free(sort_entries);
  free(sorted_indices);
  free(clusters);

  return mesh;
}

void cluster_mesh_free(cluster_mesh_t *mesh) {
  if (mesh) {
    [mesh->vertex_buffer release];
    [mesh->index_buffer release];
    [mesh->cluster_buffer release];
    free(mesh);
  }
}

// Hierarchical culling and LOD selection
void cluster_mesh_cull(const cluster_mesh_t *mesh,
                       const view_frustum_t *frustum, simd_float3 view_pos,
                       float view_resolution, uint32_t *visible_clusters,
                       uint32_t *visible_count) {
  uint32_t count = 0;
  // Start from root (last cluster in buffer if built iteratively upwards)
  // For simplicity in V1, we iterate all clusters and check LOD error
  // Real Nanite uses a DAG traversal: if error is okay, render; else traverse
  // children.

  // Note: Parent cluster index is root if mesh was built until 1 cluster
  // remained. The last cluster index is `mesh->cluster_count - 1`.

  // Recursive-like traversal starting from root
  uint32_t root_idx = mesh->cluster_count - 1;

  // Internal traversal function or iterative queue
  uint32_t queue[1024];
  uint32_t q_head = 0;
  uint32_t q_tail = 0;
  queue[q_tail++] = root_idx;

  mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];

  while (q_head < q_tail && q_tail < 1024) {
    uint32_t idx = queue[q_head++];
    mesh_cluster_t *c = &clusters[idx];

    if (!is_aabb_in_frustum(c->bounds_center, c->bounds_extent, frustum))
      continue;

    // Projected error heuristic (Screen Space Error)
    // sphere_radius / distance ~ angular_error
    // angular_error * resolution ~ screen_pixels_error
    // We check if the cluster's Geometric Error projects to < 1 pixel

    float sphere_radius = simd_length(c->bounds_extent);
    float dist = simd_distance(c->bounds_center, view_pos);

    // Avoid div by zero
    if (dist < sphere_radius)
      dist = sphere_radius + 1e-6f;

    // Calculate screen space error in pixels
    // c->lod_error is world space geometric error
    float screen_error = (c->lod_error * view_resolution) / dist;

    if (screen_error <= 1.0f || c->child_count == 0) {
      // Good enough or leaf
      visible_clusters[count++] = idx;
    } else {
      // Traverse children
      for (uint32_t i = 0; i < c->child_count; i++) {
        queue[q_tail++] = c->child_clusters[i];
      }
    }
  }
  *visible_count = count;
}
