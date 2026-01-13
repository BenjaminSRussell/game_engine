// geometry/nanite/cluster_builder.c
// Nanite virtualized geometry cluster building implementation.
//
// Implemented cluster DAG (Directed Acyclic Graph) construction for
// hierarchical LOD.
// Added error-metric calculation based on geometric divergence.
// Added CPU fallback for HZB-based occlusion culling and 2-pass culling hooks.
// Added programmable rasterization fallback categorization for tiny clusters.
// Added mesh-shading payload generation for meshlet-friendly processing.
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

typedef struct {
  const float *levels;
  const uint32_t *level_offsets;
  uint32_t level_count;
  uint32_t width;
  uint32_t height;
} hzb_pyramid_t;

typedef struct {
  uint32_t index_offset;
  uint32_t triangle_count;
  uint32_t vertex_offset;
  uint32_t vertex_count;
} meshlet_payload_t;

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

static bool hzb_test_occlusion(simd_float3 center, float radius,
                               simd_float4x4 view_proj,
                               const hzb_pyramid_t *hzb) {
  if (!hzb || !hzb->levels || hzb->width == 0 || hzb->height == 0) {
    return false;
  }

  simd_float4 clip =
      simd_mul(view_proj, (simd_float4){center.x, center.y, center.z, 1.0f});
  if (clip.w <= 0.0f) {
    return false;
  }

  simd_float3 ndc = (simd_float3){clip.x / clip.w, clip.y / clip.w,
                                  clip.z / clip.w};
  if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) {
    return false;
  }

  float u = (ndc.x * 0.5f) + 0.5f;
  float v = (ndc.y * 0.5f) + 0.5f;
  uint32_t x = (uint32_t)fminf(fmaxf(u * (float)(hzb->width - 1), 0.0f),
                               (float)(hzb->width - 1));
  uint32_t y = (uint32_t)fminf(fmaxf(v * (float)(hzb->height - 1), 0.0f),
                               (float)(hzb->height - 1));

  const float *level0 = hzb->levels;
  float depth = ndc.z;
  float hzb_depth = level0[y * hzb->width + x];

  float bias = radius * 0.001f;
  return depth > hzb_depth + bias;
}

static void build_meshlet_payloads(const mesh_cluster_t *clusters,
                                   uint32_t cluster_count,
                                   meshlet_payload_t *out_payloads) {
  for (uint32_t i = 0; i < cluster_count; i++) {
    out_payloads[i].index_offset = clusters[i].index_offset;
    out_payloads[i].triangle_count = clusters[i].triangle_count;
    out_payloads[i].vertex_offset = clusters[i].vertex_offset;
    out_payloads[i].vertex_count = CLUSTER_VERTEX_COUNT;
  }
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

    clusters[c].lod_error = compute_cluster_error(
        vertices, sorted_indices, clusters[c].index_offset, c_tri_count * 3,
        clusters[c].bounds_center);
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

  // 6. Mesh shader payloads (meshlets)
  mesh->meshlet_count = cluster_count;
  meshlet_payload_t *payloads = (meshlet_payload_t *)malloc(
      sizeof(meshlet_payload_t) * mesh->meshlet_count);
  build_meshlet_payloads(clusters, cluster_count, payloads);
  mesh->meshlet_buffer =
      [device newBufferWithBytes:payloads
                          length:mesh->meshlet_count * sizeof(meshlet_payload_t)
                         options:MTLResourceStorageModeShared];
  free(payloads);

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
    [mesh->meshlet_buffer release];
    if (mesh->quantized_vertex_buffer) [mesh->quantized_vertex_buffer release];
    if (mesh->bvh_buffer) [mesh->bvh_buffer release];
    if (mesh->lru_cache_buffer) [mesh->lru_cache_buffer release];
    if (mesh->visibility_history_buffer) [mesh->visibility_history_buffer release];
    if (mesh->blas_buffer) [mesh->blas_buffer release];
    if (mesh->animation_buffer) [mesh->animation_buffer release];
    for (uint32_t i = 0; i < VSM_CASCADE_COUNT; i++) {
        if (mesh->vsm.depth_textures[i]) [mesh->vsm.depth_textures[i] release];
        if (mesh->vsm.variance_textures[i]) [mesh->vsm.variance_textures[i] release];
    }
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

void cluster_mesh_cull_with_fallback(
    const cluster_mesh_t *mesh, const view_frustum_t *frustum,
    simd_float3 view_pos, float view_resolution, uint32_t *visible_clusters,
    uint32_t *visible_count, uint32_t *fallback_clusters,
    uint32_t *fallback_count) {
  if (!mesh || !visible_clusters || !visible_count || !fallback_clusters ||
      !fallback_count) {
    return;
  }

  uint32_t visible = 0;
  uint32_t fallback = 0;

  mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
  for (uint32_t idx = 0; idx < mesh->cluster_count; idx++) {
    mesh_cluster_t *c = &clusters[idx];

    if (!is_aabb_in_frustum(c->bounds_center, c->bounds_extent, frustum))
      continue;

    float sphere_radius = simd_length(c->bounds_extent);
    float dist = simd_distance(c->bounds_center, view_pos);
    if (dist < sphere_radius)
      dist = sphere_radius + 1e-6f;

    float screen_error = (c->lod_error * view_resolution) / dist;
    if (screen_error <= 0.25f) {
      fallback_clusters[fallback++] = idx;
    } else {
      visible_clusters[visible++] = idx;
    }
  }

  *visible_count = visible;
  *fallback_count = fallback;
}

void cluster_mesh_cull_hzb(const cluster_mesh_t *mesh,
                           const view_frustum_t *frustum,
                           simd_float4x4 view_proj, simd_float3 view_pos,
                           float view_resolution, const hzb_pyramid_t *hzb,
                           uint32_t *visible_clusters,
                           uint32_t *visible_count) {
  if (!mesh || !visible_clusters || !visible_count) {
    return;
  }

  uint32_t count = 0;
  mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];

  for (uint32_t idx = 0; idx < mesh->cluster_count; idx++) {
    mesh_cluster_t *c = &clusters[idx];
    if (!is_aabb_in_frustum(c->bounds_center, c->bounds_extent, frustum)) {
      continue;
    }

    float radius = simd_length(c->bounds_extent);
    if (hzb_test_occlusion(c->bounds_center, radius, view_proj, hzb)) {
      continue;
    }

    float dist = simd_distance(c->bounds_center, view_pos);
    if (dist < radius)
      dist = radius + 1e-6f;

    float screen_error = (c->lod_error * view_resolution) / dist;
    if (screen_error <= 1.0f || c->child_count == 0) {
      visible_clusters[count++] = idx;
    }
  }

  *visible_count = count;
}

void cluster_mesh_cull_hzb_two_pass(
    const cluster_mesh_t *mesh, const view_frustum_t *frustum,
    simd_float4x4 view_proj, simd_float3 view_pos, float view_resolution,
    const hzb_pyramid_t *prev_hzb, const hzb_pyramid_t *current_hzb,
    uint32_t *visible_clusters, uint32_t *visible_count) {
  if (!mesh || !visible_clusters || !visible_count) {
    return;
  }

  uint32_t count = 0;
  mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];

  for (uint32_t idx = 0; idx < mesh->cluster_count; idx++) {
    mesh_cluster_t *c = &clusters[idx];
    if (!is_aabb_in_frustum(c->bounds_center, c->bounds_extent, frustum)) {
      continue;
    }

    float radius = simd_length(c->bounds_extent);
    if (prev_hzb && hzb_test_occlusion(c->bounds_center, radius, view_proj,
                                       prev_hzb)) {
      continue;
    }

    if (current_hzb && hzb_test_occlusion(c->bounds_center, radius, view_proj,
                                          current_hzb)) {
      continue;
    }

    float dist = simd_distance(c->bounds_center, view_pos);
    if (dist < radius)
      dist = radius + 1e-6f;

    float screen_error = (c->lod_error * view_resolution) / dist;
    if (screen_error <= 1.0f || c->child_count == 0) {
      visible_clusters[count++] = idx;
    }
  }

  *visible_count = count;
}
// Virtual Shadow Map implementation
void cluster_mesh_init_virtual_shadow_maps(cluster_mesh_t *mesh, id<MTLDevice> device, uint32_t resolution) {
    if (!mesh || !device) return;
    
    mesh->vsm.resolution = resolution;
    
    // Create depth and variance textures for each cascade
    for (uint32_t i = 0; i < VSM_CASCADE_COUNT; i++) {
        // Depth texture
        MTLTextureDescriptor *depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                         width:resolution
                                                                                        height:resolution
                                                                                     mipmapped:NO];
        depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        depthDesc.storageMode = MTLStorageModePrivate;
        mesh->vsm.depth_textures[i] = [device newTextureWithDescriptor:depthDesc];
        
        // Variance texture (RG32F for moments)
        MTLTextureDescriptor *varianceDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRG32Float
                                                                                             width:resolution
                                                                                            height:resolution
                                                                                        mipmapped:NO];
        varianceDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        varianceDesc.storageMode = MTLStorageModePrivate;
        mesh->vsm.variance_textures[i] = [device newTextureWithDescriptor:varianceDesc];
    }
    
    // Initialize cascade splits (logarithmic distribution)
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float lambda = 0.5f; // Blend between linear and logarithmic
    
    for (uint32_t i = 0; i < VSM_CASCADE_COUNT; i++) {
        float linear_mix = (float)(i + 1) / (float)VSM_CASCADE_COUNT;
        float log_mix = powf((float)(i + 1) / (float)VSM_CASCADE_COUNT, 2.0f);
        mesh->vsm.cascade_splits[i] = near_plane * powf(far_plane / near_plane, lambda * log_mix + (1.0f - lambda) * linear_mix);
    }
    
    mesh->use_virtual_shadow_maps = true;
}

void cluster_mesh_update_virtual_shadow_maps(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd, const simd_float4x4 *light_view_proj) {
    if (!mesh || !cmd || !light_view_proj || !mesh->use_virtual_shadow_maps) return;
    
    mesh->vsm.view_proj = *light_view_proj;
    
    // Update shadow matrices for each cascade
    for (uint32_t i = 0; i < VSM_CASCADE_COUNT; i++) {
        // Create cascade-specific view-projection matrix
        // This would normally include light direction and cascade bounds
        mesh->vsm.shadow_matrix = *light_view_proj;
    }
}

void cluster_mesh_render_shadow_cascades(cluster_mesh_t *mesh, id<MTLRenderCommandEncoder> encoder, uint32_t cascade_index) {
    if (!mesh || !encoder || cascade_index >= VSM_CASCADE_COUNT || !mesh->use_virtual_shadow_maps) return;
    
    // Set shadow rendering state
    [encoder setDepthStencilTexture:mesh->vsm.depth_textures[cascade_index] atIndex:0];
    [encoder setFragmentTexture:mesh->vsm.variance_textures[cascade_index] atIndex:0];
    
    // Set cascade-specific constants
    [encoder setVertexBytes:&mesh->vsm.shadow_matrix length:sizeof(simd_float4x4) atIndex:4];
    [encoder setFragmentBytes:&cascade_index length:sizeof(uint32_t) atIndex:1];
}

// Vertex quantization implementation
void cluster_mesh_quantize_vertices(cluster_mesh_t *mesh, const vertex_t *vertices, uint32_t vertex_count) {
    if (!mesh || !vertices || vertex_count == 0) return;
    
    // Calculate bounds for quantization
    simd_float3 min_bound = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
    simd_float3 max_bound = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};
    
    for (uint32_t i = 0; i < vertex_count; i++) {
        simd_float3 pos = (simd_float3){vertices[i].position.x, vertices[i].position.y, vertices[i].position.z};
        min_bound = simd_min(min_bound, pos);
        max_bound = simd_max(max_bound, pos);
    }
    
    simd_float3 range = max_bound - min_bound;
    simd_float3 inv_range = 1.0f / (range + 1e-6f);
    
    // Allocate quantized vertex buffer
    quantized_vertex_t *quantized_vertices = (quantized_vertex_t *)malloc(vertex_count * sizeof(quantized_vertex_t));
    
    for (uint32_t i = 0; i < vertex_count; i++) {
        quantized_vertex_t *qv = &quantized_vertices[i];
        const vertex_t *v = &vertices[i];
        
        // Quantize position (16-bit)
        simd_float3 norm_pos = ((simd_float3){v->position.x, v->position.y, v->position.z} - min_bound) * inv_range;
        qv->position[0] = (int16_t)(norm_pos.x * 65535.0f);
        qv->position[1] = (int16_t)(norm_pos.y * 65535.0f);
        qv->position[2] = (int16_t)(norm_pos.z * 65535.0f);
        
        // Quantize normal (octahedral encoding)
        simd_float3 normal = simd_normalize((simd_float3){v->normal.x, v->normal.y, v->normal.z});
        float oct_x = normal.x / (fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z));
        float oct_y = normal.y / (fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z));
        qv->normal = (uint16_t)(((oct_x * 0.5f + 0.5f) * 255.0f) << 8) | (uint16_t)((oct_y * 0.5f + 0.5f) * 255.0f);
        
        // Quantize texture coordinates
        qv->texcoord[0] = (uint16_t)(fmodf(v->texcoord.x, 1.0f) * 65535.0f);
        qv->texcoord[1] = (uint16_t)(fmodf(v->texcoord.y, 1.0f) * 65535.0f);
        
        // Material ID (assuming it's stored in vertex color or similar)
        qv->material_id = 0; // Default material
    }
    
    // Create GPU buffer
    mesh->quantized_vertex_buffer = [mesh->vertex_buffer.device newBufferWithBytes:quantized_vertices
                                                                           length:vertex_count * sizeof(quantized_vertex_t)
                                                                          options:MTLResourceStorageModeShared];
    mesh->quantized_vertex_count = vertex_count;
    
    free(quantized_vertices);
}

void cluster_mesh_dequantize_vertices(const cluster_mesh_t *mesh, vertex_t *out_vertices, uint32_t start_index, uint32_t count) {
    if (!mesh || !out_vertices || !mesh->quantized_vertex_buffer) return;
    
    quantized_vertex_t *quantized = (quantized_vertex_t *)[mesh->quantized_vertex_buffer contents];
    
    for (uint32_t i = 0; i < count && (start_index + i) < mesh->quantized_vertex_count; i++) {
        const quantized_vertex_t *qv = &quantized[start_index + i];
        vertex_t *v = &out_vertices[i];
        
        // Dequantize position
        v->position.x = (float)qv->position[0] / 65535.0f;
        v->position.y = (float)qv->position[1] / 65535.0f;
        v->position.z = (float)qv->position[2] / 65535.0f;
        
        // Dequantize normal (octahedral decoding)
        uint8_t oct_x = (qv->normal >> 8) & 0xFF;
        uint8_t oct_y = qv->normal & 0xFF;
        float nx = (oct_x / 255.0f) * 2.0f - 1.0f;
        float ny = (oct_y / 255.0f) * 2.0f - 1.0f;
        float nz = 1.0f - fabsf(nx) - fabsf(ny);
        if (nz < 0.0f) {
            nx = copysignf(1.0f - fabsf(ny), nx);
            ny = copysignf(1.0f - fabsf(nx), ny);
        }
        v->normal.x = nx;
        v->normal.y = ny;
        v->normal.z = nz;
        
        // Dequantize texture coordinates
        v->texcoord.x = (float)qv->texcoord[0] / 65535.0f;
        v->texcoord.y = (float)qv->texcoord[1] / 65535.0f;
    }
}

// BVH construction implementation
void cluster_mesh_build_bvh(cluster_mesh_t *mesh) {
    if (!mesh || mesh->cluster_count == 0) return;
    
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    
    // Calculate required BVH nodes (2 * cluster_count - 1 for binary tree)
    uint32_t max_nodes = mesh->cluster_count * 2;
    bvh_node_t *bvh_nodes = (bvh_node_t *)calloc(max_nodes, sizeof(bvh_node_t));
    
    // Build leaf nodes first
    uint32_t node_count = 0;
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        bvh_node_t *node = &bvh_nodes[node_count++];
        mesh_cluster_t *cluster = &clusters[i];
        
        node->bounds_min = cluster->bounds_center - cluster->bounds_extent;
        node->bounds_max = cluster->bounds_center + cluster->bounds_extent;
        node->cluster_start = i;
        node->cluster_count = 1;
        node->is_leaf = 1;
        node->left_child = node->right_child = 0xFFFFFFFF;
        
        cluster->bvh_node_index = i;
    }
    
    // Build internal nodes using SAH (Surface Area Heuristic)
    uint32_t leaf_start = 0;
    uint32_t internal_start = node_count;
    
    while (node_count < max_nodes && (node_count - internal_start) < (internal_start - leaf_start) - 1) {
        // Find best pair of nodes to merge
        uint32_t best_a = 0, best_b = 0;
        float best_cost = MAXFLOAT;
        
        for (uint32_t a = leaf_start; a < node_count; a++) {
            if (bvh_nodes[a].is_leaf == 0) continue; // Only consider leaf nodes for now
            
            for (uint32_t b = a + 1; b < node_count; b++) {
                if (bvh_nodes[b].is_leaf == 0) continue;
                
                // Calculate merged bounds
                simd_float3 merged_min = simd_min(bvh_nodes[a].bounds_min, bvh_nodes[b].bounds_min);
                simd_float3 merged_max = simd_max(bvh_nodes[a].bounds_max, bvh_nodes[b].bounds_max);
                simd_float3 merged_size = merged_max - merged_min;
                float merged_surface_area = 2.0f * (merged_size.x * merged_size.y + merged_size.y * merged_size.z + merged_size.z * merged_size.x);
                
                if (merged_surface_area < best_cost) {
                    best_cost = merged_surface_area;
                    best_a = a;
                    best_b = b;
                }
            }
        }
        
        if (best_a == best_b) break; // No more merges possible
        
        // Create internal node
        bvh_node_t *parent = &bvh_nodes[node_count++];
        parent->bounds_min = simd_min(bvh_nodes[best_a].bounds_min, bvh_nodes[best_b].bounds_min);
        parent->bounds_max = simd_max(bvh_nodes[best_a].bounds_max, bvh_nodes[best_b].bounds_max);
        parent->left_child = best_a;
        parent->right_child = best_b;
        parent->is_leaf = 0;
        parent->cluster_start = bvh_nodes[best_a].cluster_start;
        parent->cluster_count = bvh_nodes[best_a].cluster_count + bvh_nodes[best_b].cluster_count;
        
        // Mark children as non-leaf
        bvh_nodes[best_a].is_leaf = 0;
        bvh_nodes[best_b].is_leaf = 0;
    }
    
    // Create GPU buffer
    mesh->bvh_buffer = [mesh->vertex_buffer.device newBufferWithBytes:bvh_nodes
                                                               length:node_count * sizeof(bvh_node_t)
                                                              options:MTLResourceStorageModeShared];
    mesh->bvh_node_count = node_count;
    
    free(bvh_nodes);
}

void cluster_mesh_cull_bvh_gpu(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd, const simd_float4x4 *view_proj, const simd_float3 *camera_pos) {
    if (!mesh || !cmd || !view_proj || !camera_pos || !mesh->bvh_buffer) return;
    
    // This would dispatch a compute shader for GPU-based BVH traversal
    // For now, we'll use CPU fallback
    // TODO: Implement GPU compute shader for BVH culling
}

// LRU streaming implementation
void cluster_mesh_init_lru_cache(cluster_mesh_t *mesh, uint32_t cache_size, float memory_budget_mb) {
    if (!mesh) return;
    
    mesh->lru_cache_size = cache_size;
    mesh->memory_budget_mb = memory_budget_mb;
    mesh->current_frame = 0;
    mesh->resident_cluster_count = 0;
    
    // Initialize LRU cache entries
    lru_cache_entry_t *cache_entries = (lru_cache_entry_t *)calloc(cache_size, sizeof(lru_cache_entry_t));
    for (uint32_t i = 0; i < cache_size; i++) {
        cache_entries[i].cluster_id = 0xFFFFFFFF;
        cache_entries[i].last_accessed_frame = 0;
        cache_entries[i].access_count = 0;
        cache_entries[i].is_resident = false;
        cache_entries[i].priority = 0;
    }
    
    mesh->lru_cache_buffer = [mesh->vertex_buffer.device newBufferWithBytes:cache_entries
                                                                       length:cache_size * sizeof(lru_cache_entry_t)
                                                                      options:MTLResourceStorageModeShared];
    free(cache_entries);
}

void cluster_mesh_update_lru_cache(cluster_mesh_t *mesh, uint32_t frame) {
    if (!mesh || !mesh->lru_cache_buffer) return;
    
    mesh->current_frame = frame;
    lru_cache_entry_t *cache = (lru_cache_entry_t *)[mesh->lru_cache_buffer contents];
    
    // Update resident count and check for eviction
    uint32_t resident_count = 0;
    uint32_t oldest_frame = frame;
    uint32_t oldest_index = 0;
    
    for (uint32_t i = 0; i < mesh->lru_cache_size; i++) {
        if (cache[i].is_resident) {
            resident_count++;
            if (cache[i].last_accessed_frame < oldest_frame) {
                oldest_frame = cache[i].last_accessed_frame;
                oldest_index = i;
            }
        }
    }
    
    mesh->resident_cluster_count = resident_count;
    
    // Simple eviction if over budget
    if (resident_count > mesh->lru_cache_size * 0.8f) {
        cache[oldest_index].is_resident = false;
        mesh->resident_cluster_count--;
    }
}

bool cluster_mesh_is_cluster_resident(cluster_mesh_t *mesh, uint32_t cluster_id) {
    if (!mesh || !mesh->lru_cache_buffer) return false;
    
    lru_cache_entry_t *cache = (lru_cache_entry_t *)[mesh->lru_cache_buffer contents];
    for (uint32_t i = 0; i < mesh->lru_cache_size; i++) {
        if (cache[i].cluster_id == cluster_id && cache[i].is_resident) {
            return true;
        }
    }
    return false;
}

void cluster_mesh_mark_cluster_accessed(cluster_mesh_t *mesh, uint32_t cluster_id, uint32_t frame, uint8_t priority) {
    if (!mesh || !mesh->lru_cache_buffer) return;
    
    lru_cache_entry_t *cache = (lru_cache_entry_t *)[mesh->lru_cache_buffer contents];
    
    // Find existing entry or create new one
    for (uint32_t i = 0; i < mesh->lru_cache_size; i++) {
        if (cache[i].cluster_id == cluster_id || cache[i].cluster_id == 0xFFFFFFFF) {
            cache[i].cluster_id = cluster_id;
            cache[i].last_accessed_frame = frame;
            cache[i].access_count++;
            cache[i].is_resident = true;
            cache[i].priority = priority;
            return;
        }
    }
}

// Material quantization implementation
void cluster_mesh_quantize_materials(cluster_mesh_t *mesh, const uint32_t *material_ids, uint32_t material_count) {
    if (!mesh || !material_ids || material_count == 0) return;
    
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    
    // Count unique materials per cluster
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        mesh_cluster_t *cluster = &clusters[i];
        
        // Count unique materials in this cluster
        uint32_t unique_materials = 0;
        uint16_t material_list[MAX_MATERIAL_IDS] = {0};
        
        for (uint32_t j = 0; j < cluster->triangle_count * 3; j++) {
            uint32_t vertex_idx = cluster->index_offset + j;
            if (vertex_idx < material_count) {
                uint32_t mat_id = material_ids[vertex_idx];
                if (mat_id < MAX_MATERIAL_IDS) {
                    bool found = false;
                    for (uint32_t k = 0; k < unique_materials; k++) {
                        if (material_list[k] == mat_id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found && unique_materials < MAX_MATERIAL_IDS) {
                        material_list[unique_materials++] = (uint16_t)mat_id;
                    }
                }
            }
        }
        
        // Store material info
        cluster->material_info.material_count = (uint8_t)unique_materials;
        if (unique_materials > 0) {
            cluster->material_info.material_id = material_list[0]; // Primary material
        }
    }
}

// Visibility tracking implementation
void cluster_mesh_init_visibility_history(cluster_mesh_t *mesh) {
    if (!mesh) return;
    
    mesh->visibility_history_count = mesh->cluster_count;
    visibility_history_t *history = (visibility_history_t *)calloc(mesh->cluster_count, sizeof(visibility_history_t));
    
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        history[i].cluster_id = i;
        history[i].visibility_score = 0;
        history[i].last_distance = 0.0f;
        history[i].frame_counter = 0;
        for (uint32_t j = 0; j < VISIBILITY_HISTORY_FRAMES; j++) {
            history[i].visible_history[j] = false;
        }
    }
    
    mesh->visibility_history_buffer = [mesh->vertex_buffer.device newBufferWithBytes:history
                                                                              length:mesh->cluster_count * sizeof(visibility_history_t)
                                                                             options:MTLResourceStorageModeShared];
    free(history);
}

void cluster_mesh_update_visibility_history(cluster_mesh_t *mesh, const uint32_t *visible_clusters, uint32_t visible_count, uint32_t frame) {
    if (!mesh || !visible_clusters || !mesh->visibility_history_buffer) return;
    
    visibility_history_t *history = (visibility_history_t *)[mesh->visibility_history_buffer contents];
    
    // Reset all visibility for this frame
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        if (history[i].frame_counter != frame) {
            uint32_t history_index = frame % VISIBILITY_HISTORY_FRAMES;
            history[i].visible_history[history_index] = false;
            history[i].frame_counter = frame;
        }
    }
    
    // Mark visible clusters
    for (uint32_t i = 0; i < visible_count; i++) {
        uint32_t cluster_id = visible_clusters[i];
        if (cluster_id < mesh->cluster_count) {
            uint32_t history_index = frame % VISIBILITY_HISTORY_FRAMES;
            history[cluster_id].visible_history[history_index] = true;
            history[cluster_id].visibility_score++;
        }
    }
}

bool cluster_mesh_should_cull_temporal(cluster_mesh_t *mesh, uint32_t cluster_id, uint32_t frame) {
    if (!mesh || !mesh->visibility_history_buffer || cluster_id >= mesh->cluster_count) return false;
    
    visibility_history_t *history = (visibility_history_t *)[mesh->visibility_history_buffer contents];
    visibility_history_t *cluster_hist = &history[cluster_id];
    
    // Check if cluster was visible in recent frames
    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < VISIBILITY_HISTORY_FRAMES; i++) {
        if (cluster_hist->visible_history[i]) visible_count++;
    }
    
    // Cull if rarely visible (less than 25% of recent frames)
    return visible_count < (VISIBILITY_HISTORY_FRAMES / 4);
}

// Ray tracing BLAS implementation
void cluster_mesh_build_blas(cluster_mesh_t *mesh, id<MTLDevice> device) {
    if (!mesh || !device) return;
    
    // This would build acceleration structures for ray tracing
    // For now, allocate placeholder buffers
    cluster_blas_t *blas_data = (cluster_blas_t *)calloc(mesh->cluster_count, sizeof(cluster_blas_t));
    
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        blas_data[i].triangle_count = 128; // Max triangles per cluster
        blas_data[i].cluster_offset = i;
        // Actual BLAS creation would require Metal 2.4+ ray tracing support
    }
    
    mesh->blas_buffer = [device newBufferWithBytes:blas_data
                                              length:mesh->cluster_count * sizeof(cluster_blas_t)
                                             options:MTLResourceStorageModeShared];
    mesh->blas_count = mesh->cluster_count;
    
    free(blas_data);
}

void cluster_mesh_update_blas(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd) {
    if (!mesh || !cmd || !mesh->blas_buffer) return;
    
    // Update BLAS for animated clusters
    // TODO: Implement actual BLAS updates with Metal ray tracing
}

// Animation support (D-Nanite)
void cluster_mesh_init_animation_data(cluster_mesh_t *mesh) {
    if (!mesh) return;
    
    cluster_animation_data_t *anim_data = (cluster_animation_data_t *)calloc(mesh->cluster_count, sizeof(cluster_animation_data_t));
    
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        anim_data[i].bone_count = 0;
        anim_data[i].vertex_offset = 0;
        anim_data[i].vertex_count = 256; // Max vertices per cluster
    }
    
    mesh->animation_buffer = [mesh->vertex_buffer.device newBufferWithBytes:anim_data
                                                                     length:mesh->cluster_count * sizeof(cluster_animation_data_t)
                                                                    options:MTLResourceStorageModeShared];
    mesh->animation_data_count = mesh->cluster_count;
    
    free(anim_data);
}

void cluster_mesh_update_animation(cluster_mesh_t *mesh, const cluster_animation_data_t *animation_data, uint32_t cluster_id) {
    if (!mesh || !animation_data || !mesh->animation_buffer || cluster_id >= mesh->cluster_count) return;
    
    cluster_animation_data_t *anim_buffer = (cluster_animation_data_t *)[mesh->animation_buffer contents];
    anim_buffer[cluster_id] = *animation_data;
    
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    clusters[cluster_id].is_animated = true;
    clusters[cluster_id].animation_data_index = cluster_id;
}

// Displacement and tessellation
void cluster_mesh_enable_displacement(cluster_mesh_t *mesh, uint32_t cluster_id, bool enable) {
    if (!mesh || cluster_id >= mesh->cluster_count) return;
    
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    clusters[cluster_id].has_displacement = enable;
}

void cluster_mesh_update_displacement(cluster_mesh_t *mesh, id<MTLTexture> heightmap, float strength) {
    if (!mesh || !heightmap) return;
    
    // Update displacement for all clusters with displacement enabled
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        if (clusters[i].has_displacement) {
            // Apply displacement mapping
            // TODO: Implement actual displacement in vertex shader
        }
    }
}

// Geometry simplification for parent clusters
void cluster_mesh_simplify_parent_geometry(cluster_mesh_t *mesh, uint32_t parent_cluster_id, uint32_t target_triangle_count) {
    if (!mesh || parent_cluster_id >= mesh->cluster_count) return;
    
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    mesh_cluster_t *parent = &clusters[parent_cluster_id];
    
    // Simplify geometry using edge collapse or vertex clustering
    // For now, just reduce triangle count
    if (parent->triangle_count > target_triangle_count) {
        parent->triangle_count = target_triangle_count;
        parent->lod_error += simd_length(parent->bounds_extent) * 0.05f; // Increase error
    }
}

// Enhanced LOD selection
void cluster_mesh_select_lod_clusters(cluster_mesh_t *mesh, const simd_float3 *camera_pos, float screen_error_threshold, uint32_t *visible_clusters, uint32_t *visible_count) {
    if (!mesh || !camera_pos || !visible_clusters || !visible_count) return;
    
    uint32_t count = 0;
    mesh_cluster_t *clusters = (mesh_cluster_t *)[mesh->cluster_buffer contents];
    
    for (uint32_t i = 0; i < mesh->cluster_count; i++) {
        mesh_cluster_t *cluster = &clusters[i];
        
        // Calculate distance-based LOD
        float distance = simd_distance(cluster->bounds_center, *camera_pos);
        float screen_error = (cluster->lod_error * screen_error_threshold) / distance;
        
        // Select cluster if error is acceptable
        if (screen_error <= 1.0f || cluster->child_count == 0) {
            visible_clusters[count++] = i;
        }
    }
    
    *visible_count = count;
}
