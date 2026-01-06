// src/rendering/mesh_optimizer.c
//
// Implementation of mesh optimization systems including LOD generation,
// compression, culling, and vertex cache optimization.

#include <math.h>
#include <renderer/camera.h>
#include <renderer/mesh_optimizer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LOD Generation
// ============================================================================

// Simplify mesh by reducing vertex count
static void simplify_mesh(Mesh *out, const Mesh *source, f32 ratio) {
  // Simple decimation algorithm - keep every Nth vertex
  u32 target_vertices = (u32)(source->vertex_count * ratio);
  if (target_vertices < 3)
    target_vertices = 3; // Minimum for a triangle

  u32 step = source->vertex_count / target_vertices;
  if (step < 1)
    step = 1;

  mesh_init(out, target_vertices, target_vertices * 3);

  // Sample vertices
  for (u32 i = 0;
       i < source->vertex_count && out->vertex_count < target_vertices;
       i += step) {
    out->vertices[out->vertex_count++] = source->vertices[i];
  }

  // Rebuild indices (simple triangle strip)
  for (u32 i = 0; i + 2 < out->vertex_count; i++) {
    out->indices[out->index_count++] = i;
    out->indices[out->index_count++] = i + 1;
    out->indices[out->index_count++] = i + 2;
  }
}

void mesh_lod_generate(MeshLODSet *lod_set, const Mesh *source_mesh,
                       Vec3 center) {
  if (!lod_set || !source_mesh)
    return;

  lod_set->level_count = LOD_LEVEL_COUNT;
  lod_set->center = center;

  // LOD 0: High detail (full mesh)
  lod_set->levels[LOD_LEVEL_HIGH].distance_threshold = 32.0f;
  lod_set->levels[LOD_LEVEL_HIGH].simplification_ratio = 1.0f;
  mesh_init(&lod_set->levels[LOD_LEVEL_HIGH].mesh, source_mesh->vertex_capacity,
            source_mesh->index_capacity);

  // Copy full mesh
  memcpy(lod_set->levels[LOD_LEVEL_HIGH].mesh.vertices, source_mesh->vertices,
         sizeof(Vertex) * source_mesh->vertex_count);
  memcpy(lod_set->levels[LOD_LEVEL_HIGH].mesh.indices, source_mesh->indices,
         sizeof(u32) * source_mesh->index_count);
  lod_set->levels[LOD_LEVEL_HIGH].mesh.vertex_count = source_mesh->vertex_count;
  lod_set->levels[LOD_LEVEL_HIGH].mesh.index_count = source_mesh->index_count;

  // LOD 1: Medium detail (60% vertices)
  lod_set->levels[LOD_LEVEL_MEDIUM].distance_threshold = 64.0f;
  lod_set->levels[LOD_LEVEL_MEDIUM].simplification_ratio = 0.6f;
  simplify_mesh(&lod_set->levels[LOD_LEVEL_MEDIUM].mesh, source_mesh, 0.6f);

  // LOD 2: Low detail (30% vertices)
  lod_set->levels[LOD_LEVEL_LOW].distance_threshold = INFINITY;
  lod_set->levels[LOD_LEVEL_LOW].simplification_ratio = 0.3f;
  simplify_mesh(&lod_set->levels[LOD_LEVEL_LOW].mesh, source_mesh, 0.3f);
}

LODLevel mesh_lod_select(const MeshLODSet *lod_set, Vec3 camera_pos) {
  if (!lod_set)
    return LOD_LEVEL_HIGH;

  // Calculate distance from camera to mesh center
  Vec3 delta = vec3_sub(camera_pos, lod_set->center);
  f32 distance = vec3_length(delta);

  // Select LOD based on distance
  for (u32 i = 0; i < lod_set->level_count; i++) {
    if (distance < lod_set->levels[i].distance_threshold) {
      return (LODLevel)i;
    }
  }

  return LOD_LEVEL_LOW;
}

Mesh *mesh_lod_get(MeshLODSet *lod_set, LODLevel level) {
  if (!lod_set || level >= LOD_LEVEL_COUNT)
    return NULL;
  return &lod_set->levels[level].mesh;
}

void mesh_lod_free(MeshLODSet *lod_set) {
  if (!lod_set)
    return;

  for (u32 i = 0; i < lod_set->level_count; i++) {
    mesh_free(&lod_set->levels[i].mesh);
  }
}

// ============================================================================
// Mesh Compression
// ============================================================================

// Convert float to half-float (16-bit)
static u16 float_to_half(f32 value) {
  // Simplified conversion (not IEEE 754 compliant, but fast)
  u32 f = *(u32 *)&value;
  u16 sign = (f >> 16) & 0x8000;
  u16 exponent = ((f >> 23) & 0xFF) - 112;
  u16 mantissa = (f >> 13) & 0x3FF;

  if (exponent > 30)
    exponent = 31; // Clamp to max
  if (exponent < 0)
    exponent = 0; // Clamp to min

  return sign | (exponent << 10) | mantissa;
}

// Convert half-float to float
static f32 half_to_float(u16 value) {
  u32 sign = (value & 0x8000) << 16;
  u32 exponent = ((value >> 10) & 0x1F) + 112;
  u32 mantissa = (value & 0x3FF) << 13;

  u32 result = sign | (exponent << 23) | mantissa;
  return *(f32 *)&result;
}

// Quantize float to 16-bit integer within bounds
static u16 quantize_float(f32 value, f32 min, f32 max) {
  f32 normalized = (value - min) / (max - min);
  normalized = fmaxf(0.0f, fminf(1.0f, normalized));
  return (u16)(normalized * 65535.0f);
}

// Dequantize 16-bit integer to float
static f32 dequantize_float(u16 value, f32 min, f32 max) {
  f32 normalized = (f32)value / 65535.0f;
  return min + normalized * (max - min);
}

bool mesh_compress(CompressedMesh *out, const Mesh *source,
                   VertexCompressionType type) {
  if (!out || !source || source->vertex_count == 0)
    return false;

  // Calculate bounds
  out->bounds_min = source->vertices[0].position;
  out->bounds_max = source->vertices[0].position;

  for (u32 i = 1; i < source->vertex_count; i++) {
    Vec3 pos = source->vertices[i].position;
    out->bounds_min.x = fminf(out->bounds_min.x, pos.x);
    out->bounds_min.y = fminf(out->bounds_min.y, pos.y);
    out->bounds_min.z = fminf(out->bounds_min.z, pos.z);
    out->bounds_max.x = fmaxf(out->bounds_max.x, pos.x);
    out->bounds_max.y = fmaxf(out->bounds_max.y, pos.y);
    out->bounds_max.z = fmaxf(out->bounds_max.z, pos.z);
  }

  out->original_vertex_count = source->vertex_count;
  out->original_index_count = source->index_count;
  out->compression_type = type;

  switch (type) {
  case VERTEX_COMPRESSION_HALF_FLOAT: {
    // 16-bit floats: position(6) + normal(6) + uv(4) + attributes(4) = 20
    // bytes/vertex
    u32 vertex_size = 20;
    out->compressed_size =
        source->vertex_count * vertex_size + source->index_count * sizeof(u32);
    out->compressed_data = malloc(out->compressed_size);

    u8 *ptr = out->compressed_data;
    for (u32 i = 0; i < source->vertex_count; i++) {
      const Vertex *v = &source->vertices[i];

      // Position (3x u16)
      *(u16 *)ptr = float_to_half(v->position.x);
      ptr += 2;
      *(u16 *)ptr = float_to_half(v->position.y);
      ptr += 2;
      *(u16 *)ptr = float_to_half(v->position.z);
      ptr += 2;

      // Normal (3x u16)
      *(u16 *)ptr = float_to_half(v->normal.x);
      ptr += 2;
      *(u16 *)ptr = float_to_half(v->normal.y);
      ptr += 2;
      *(u16 *)ptr = float_to_half(v->normal.z);
      ptr += 2;

      // UV (2x u16)
      *(u16 *)ptr = float_to_half(v->uv.x);
      ptr += 2;
      *(u16 *)ptr = float_to_half(v->uv.y);
      ptr += 2;

      // Attributes (4x u8)
      *ptr++ = v->ao;
      *ptr++ = v->light;
      *ptr++ = v->texture_id;
      *ptr++ = (u8)(v->wave_phase * 255.0f);
    }

    // Copy indices
    memcpy(ptr, source->indices, source->index_count * sizeof(u32));
    break;
  }

  case VERTEX_COMPRESSION_QUANTIZED: {
    // Quantized: position(6) + normal(6) + uv(4) + attributes(4) = 20
    // bytes/vertex
    u32 vertex_size = 20;
    out->compressed_size =
        source->vertex_count * vertex_size + source->index_count * sizeof(u32);
    out->compressed_data = malloc(out->compressed_size);

    u8 *ptr = out->compressed_data;
    for (u32 i = 0; i < source->vertex_count; i++) {
      const Vertex *v = &source->vertices[i];

      // Quantize position
      *(u16 *)ptr =
          quantize_float(v->position.x, out->bounds_min.x, out->bounds_max.x);
      ptr += 2;
      *(u16 *)ptr =
          quantize_float(v->position.y, out->bounds_min.y, out->bounds_max.y);
      ptr += 2;
      *(u16 *)ptr =
          quantize_float(v->position.z, out->bounds_min.z, out->bounds_max.z);
      ptr += 2;

      // Quantize normal (normalized to [-1, 1])
      *(u16 *)ptr = quantize_float(v->normal.x, -1.0f, 1.0f);
      ptr += 2;
      *(u16 *)ptr = quantize_float(v->normal.y, -1.0f, 1.0f);
      ptr += 2;
      *(u16 *)ptr = quantize_float(v->normal.z, -1.0f, 1.0f);
      ptr += 2;

      // Quantize UV
      *(u16 *)ptr = quantize_float(v->uv.x, 0.0f, 1.0f);
      ptr += 2;
      *(u16 *)ptr = quantize_float(v->uv.y, 0.0f, 1.0f);
      ptr += 2;

      // Attributes
      *ptr++ = v->ao;
      *ptr++ = v->light;
      *ptr++ = v->texture_id;
      *ptr++ = (u8)(v->wave_phase * 255.0f);
    }

    // Copy indices
    memcpy(ptr, source->indices, source->index_count * sizeof(u32));
    break;
  }

  default:
    return false;
  }

  return true;
}

bool mesh_decompress(Mesh *out, const CompressedMesh *compressed) {
  if (!out || !compressed || !compressed->compressed_data)
    return false;

  mesh_init(out, compressed->original_vertex_count,
            compressed->original_index_count);

  const u8 *ptr = compressed->compressed_data;

  switch (compressed->compression_type) {
  case VERTEX_COMPRESSION_HALF_FLOAT: {
    for (u32 i = 0; i < compressed->original_vertex_count; i++) {
      Vertex *v = &out->vertices[i];

      // Position
      v->position.x = half_to_float(*(u16 *)ptr);
      ptr += 2;
      v->position.y = half_to_float(*(u16 *)ptr);
      ptr += 2;
      v->position.z = half_to_float(*(u16 *)ptr);
      ptr += 2;

      // Normal
      v->normal.x = half_to_float(*(u16 *)ptr);
      ptr += 2;
      v->normal.y = half_to_float(*(u16 *)ptr);
      ptr += 2;
      v->normal.z = half_to_float(*(u16 *)ptr);
      ptr += 2;

      // UV
      v->uv.x = half_to_float(*(u16 *)ptr);
      ptr += 2;
      v->uv.y = half_to_float(*(u16 *)ptr);
      ptr += 2;

      // Attributes
      v->ao = *ptr++;
      v->light = *ptr++;
      v->texture_id = *ptr++;
      v->wave_phase = (*ptr++) / 255.0f;
    }
    out->vertex_count = compressed->original_vertex_count;

    // Copy indices
    memcpy(out->indices, ptr, compressed->original_index_count * sizeof(u32));
    out->index_count = compressed->original_index_count;
    break;
  }

  case VERTEX_COMPRESSION_QUANTIZED: {
    for (u32 i = 0; i < compressed->original_vertex_count; i++) {
      Vertex *v = &out->vertices[i];

      // Dequantize position
      v->position.x = dequantize_float(*(u16 *)ptr, compressed->bounds_min.x,
                                       compressed->bounds_max.x);
      ptr += 2;
      v->position.y = dequantize_float(*(u16 *)ptr, compressed->bounds_min.y,
                                       compressed->bounds_max.y);
      ptr += 2;
      v->position.z = dequantize_float(*(u16 *)ptr, compressed->bounds_min.z,
                                       compressed->bounds_max.z);
      ptr += 2;

      // Dequantize normal
      v->normal.x = dequantize_float(*(u16 *)ptr, -1.0f, 1.0f);
      ptr += 2;
      v->normal.y = dequantize_float(*(u16 *)ptr, -1.0f, 1.0f);
      ptr += 2;
      v->normal.z = dequantize_float(*(u16 *)ptr, -1.0f, 1.0f);
      ptr += 2;

      // Dequantize UV
      v->uv.x = dequantize_float(*(u16 *)ptr, 0.0f, 1.0f);
      ptr += 2;
      v->uv.y = dequantize_float(*(u16 *)ptr, 0.0f, 1.0f);
      ptr += 2;

      // Attributes
      v->ao = *ptr++;
      v->light = *ptr++;
      v->texture_id = *ptr++;
      v->wave_phase = (*ptr++) / 255.0f;
    }
    out->vertex_count = compressed->original_vertex_count;

    // Copy indices
    memcpy(out->indices, ptr, compressed->original_index_count * sizeof(u32));
    out->index_count = compressed->original_index_count;
    break;
  }

  default:
    return false;
  }

  return true;
}

f32 mesh_get_compression_ratio(const CompressedMesh *compressed) {
  if (!compressed)
    return 1.0f;

  u32 original_size = compressed->original_vertex_count * sizeof(Vertex) +
                      compressed->original_index_count * sizeof(u32);

  return (f32)original_size / (f32)compressed->compressed_size;
}

void mesh_compressed_free(CompressedMesh *compressed) {
  if (!compressed)
    return;
  if (compressed->compressed_data) {
    free(compressed->compressed_data);
    compressed->compressed_data = NULL;
  }
}

// ============================================================================
// Culling Systems
// ============================================================================

void frustum_from_matrix(Frustum *frustum, const Mat4 *view_proj) {
  if (!frustum || !view_proj)
    return;

  // Extract frustum planes from view-projection matrix
  // Left plane
  frustum->planes[0].x = view_proj->m[3] + view_proj->m[0];
  frustum->planes[0].y = view_proj->m[7] + view_proj->m[4];
  frustum->planes[0].z = view_proj->m[11] + view_proj->m[8];

  // Right plane
  frustum->planes[1].x = view_proj->m[3] - view_proj->m[0];
  frustum->planes[1].y = view_proj->m[7] - view_proj->m[4];
  frustum->planes[1].z = view_proj->m[11] - view_proj->m[8];

  // Bottom plane
  frustum->planes[2].x = view_proj->m[3] + view_proj->m[1];
  frustum->planes[2].y = view_proj->m[7] + view_proj->m[5];
  frustum->planes[2].z = view_proj->m[11] + view_proj->m[9];

  // Top plane
  frustum->planes[3].x = view_proj->m[3] - view_proj->m[1];
  frustum->planes[3].y = view_proj->m[7] - view_proj->m[5];
  frustum->planes[3].z = view_proj->m[11] - view_proj->m[9];

  // Near plane
  frustum->planes[4].x = view_proj->m[3] + view_proj->m[2];
  frustum->planes[4].y = view_proj->m[7] + view_proj->m[6];
  frustum->planes[4].z = view_proj->m[11] + view_proj->m[10];

  // Far plane
  frustum->planes[5].x = view_proj->m[3] - view_proj->m[2];
  frustum->planes[5].y = view_proj->m[7] - view_proj->m[6];
  frustum->planes[5].z = view_proj->m[11] - view_proj->m[10];

  // Normalize planes
  for (u32 i = 0; i < 6; i++) {
    f32 length = vec3_length(frustum->planes[i]);
    if (length > 0.0f) {
      frustum->planes[i] = vec3_div(frustum->planes[i], length);
    }
  }
}

bool frustum_test_sphere(const Frustum *frustum, Vec3 center, f32 radius) {
  if (!frustum)
    return true;

  for (u32 i = 0; i < 6; i++) {
    f32 distance = vec3_dot(frustum->planes[i], center);
    if (distance < -radius) {
      return false; // Outside frustum
    }
  }

  return true; // Inside or intersecting
}

bool frustum_test_aabb(const Frustum *frustum, Vec3 min, Vec3 max) {
  if (!frustum)
    return true;

  for (u32 i = 0; i < 6; i++) {
    Vec3 plane = frustum->planes[i];

    // Get positive vertex (furthest point in plane direction)
    Vec3 positive;
    positive.x = (plane.x >= 0) ? max.x : min.x;
    positive.y = (plane.y >= 0) ? max.y : min.y;
    positive.z = (plane.z >= 0) ? max.z : min.z;

    if (vec3_dot(plane, positive) < 0) {
      return false; // Outside frustum
    }
  }

  return true; // Inside or intersecting
}

void mesh_frustum_cull(CullingResult *result, const Mesh *meshes,
                       u32 mesh_count, const Frustum *frustum,
                       const Vec3 *mesh_positions) {
  if (!result || !meshes || !frustum || !mesh_positions)
    return;

  result->visible = malloc(sizeof(bool) * mesh_count);
  result->total_count = mesh_count;
  result->visible_count = 0;

  for (u32 i = 0; i < mesh_count; i++) {
    // Use sphere test with chunk radius (assume 16x16x16 chunk = radius ~14)
    f32 radius = 14.0f;
    result->visible[i] =
        frustum_test_sphere(frustum, mesh_positions[i], radius);

    if (result->visible[i]) {
      result->visible_count++;
    }
  }
}

void mesh_occlusion_cull(CullingResult *result, const Mesh *meshes,
                         u32 mesh_count, const Vec3 *mesh_positions,
                         Vec3 camera_pos, f32 max_distance) {
  if (!result || !meshes || !mesh_positions)
    return;

  result->visible = malloc(sizeof(bool) * mesh_count);
  result->total_count = mesh_count;
  result->visible_count = 0;

  for (u32 i = 0; i < mesh_count; i++) {
    Vec3 delta = vec3_sub(mesh_positions[i], camera_pos);
    f32 distance = vec3_length(delta);

    result->visible[i] = (distance <= max_distance);

    if (result->visible[i]) {
      result->visible_count++;
    }
  }
}

void culling_result_free(CullingResult *result) {
  if (!result)
    return;
  if (result->visible) {
    free(result->visible);
    result->visible = NULL;
  }
}

// ============================================================================
// Vertex Cache Optimization
// ============================================================================

void mesh_optimize_vertex_cache(Mesh *mesh) {
  if (!mesh || mesh->index_count == 0)
    return;

  // Tom Forsyth's vertex cache optimization algorithm
  // This reorders triangles to maximize post-transform cache hits

  const u32 triangle_count = mesh->index_count / 3;
  const u32 cache_size = 32; // Typical GPU vertex cache size

  // Simple implementation: sort triangles by vertex reuse
  u32 *triangle_scores = calloc(triangle_count, sizeof(u32));
  u8 *triangle_used = calloc(triangle_count, sizeof(u8));

  // Calculate triangle scores based on vertex reuse potential
  for (u32 i = 0; i < triangle_count; i++) {
    u32 score = 0;
    for (u32 j = 0; j < 3; j++) {
      u32 vertex = mesh->indices[i * 3 + j];
      // Count how many triangles use this vertex
      for (u32 k = 0; k < triangle_count; k++) {
        if (k != i) {
          for (u32 l = 0; l < 3; l++) {
            if (mesh->indices[k * 3 + l] == vertex) {
              score++;
              break;
            }
          }
        }
      }
    }
    triangle_scores[i] = score;
  }

  // Sort triangles by score (higher score = better for cache)
  u32 *output_indices = malloc(mesh->index_count * sizeof(u32));
  u32 output_pos = 0;

  while (output_pos < triangle_count) {
    // Find best unused triangle
    u32 best_idx = 0;
    u32 best_score = 0;

    for (u32 i = 0; i < triangle_count; i++) {
      if (!triangle_used[i] && triangle_scores[i] > best_score) {
        best_score = triangle_scores[i];
        best_idx = i;
      }
    }

    // Add triangle to output
    triangle_used[best_idx] = 1;
    for (u32 j = 0; j < 3; j++) {
      output_indices[output_pos * 3 + j] = mesh->indices[best_idx * 3 + j];
    }
    output_pos++;
  }

  // Copy optimized indices back to mesh
  memcpy(mesh->indices, output_indices, mesh->index_count * sizeof(u32));

  // Cleanup
  free(triangle_scores);
  free(triangle_used);
  free(output_indices);
}

void mesh_optimize_vertex_fetch(Mesh *mesh) {
  if (!mesh || mesh->vertex_count == 0)
    return;

  // Reorder vertices to match index order for better cache locality
  // This improves vertex fetch performance by reducing memory jumps

  // Create vertex remapping table
  u32 *vertex_remap = calloc(mesh->vertex_count, sizeof(u32));
  u8 *vertex_used = calloc(mesh->vertex_count, sizeof(u8));

  // Count how many times each vertex is used
  u32 *vertex_usage = calloc(mesh->vertex_count, sizeof(u32));
  for (u32 i = 0; i < mesh->index_count; i++) {
    u32 vertex = mesh->indices[i];
    vertex_usage[vertex]++;
  }

  // Create new vertex buffer in order of first appearance in indices
  Vertex *new_vertices = malloc(mesh->vertex_count * sizeof(Vertex));
  u32 new_vertex_count = 0;

  // First pass: add vertices in order they appear in indices
  for (u32 i = 0; i < mesh->index_count; i++) {
    u32 old_vertex = mesh->indices[i];
    if (!vertex_used[old_vertex]) {
      vertex_remap[old_vertex] = new_vertex_count;
      new_vertices[new_vertex_count] = mesh->vertices[old_vertex];
      vertex_used[old_vertex] = 1;
      new_vertex_count++;
    }
  }

  // Second pass: add any remaining vertices (shouldn't be any, but just in
  // case)
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    if (!vertex_used[i]) {
      vertex_remap[i] = new_vertex_count;
      new_vertices[new_vertex_count] = mesh->vertices[i];
      vertex_used[i] = 1;
      new_vertex_count++;
    }
  }

  // Update indices to use new vertex positions
  for (u32 i = 0; i < mesh->index_count; i++) {
    mesh->indices[i] = vertex_remap[mesh->indices[i]];
  }

  // Replace old vertex buffer with optimized one
  free(mesh->vertices);
  mesh->vertices = new_vertices;
  mesh->vertex_count = new_vertex_count;

  // Cleanup
  free(vertex_remap);
  free(vertex_used);
  free(vertex_usage);
}

f32 mesh_calculate_acmr(const Mesh *mesh, u32 cache_size) {
  if (!mesh || mesh->index_count == 0)
    return 0.0f;

  // Simulate LRU cache
  u32 *cache = calloc(cache_size, sizeof(u32));
  u32 cache_pos = 0;
  u32 cache_misses = 0;

  for (u32 i = 0; i < mesh->index_count; i++) {
    u32 index = mesh->indices[i];

    // Check if in cache
    bool in_cache = false;
    for (u32 j = 0; j < cache_size; j++) {
      if (cache[j] == index + 1) { // +1 to distinguish from 0
        in_cache = true;
        break;
      }
    }

    if (!in_cache) {
      cache_misses++;
      cache[cache_pos] = index + 1;
      cache_pos = (cache_pos + 1) % cache_size;
    }
  }

  free(cache);

  // ACMR = cache misses / triangle count
  u32 triangle_count = mesh->index_count / 3;
  return (f32)cache_misses / (f32)triangle_count;
}

// ============================================================================
// Mesh Statistics
// ============================================================================

void mesh_calculate_stats(MeshStats *stats, const Mesh *mesh) {
  if (!stats || !mesh)
    return;

  stats->vertex_count = mesh->vertex_count;
  stats->index_count = mesh->index_count;
  stats->triangle_count = mesh->index_count / 3;

  // Calculate memory usage
  u32 vertex_bytes = mesh->vertex_count * sizeof(Vertex);
  u32 index_bytes = mesh->index_count * sizeof(u32);
  stats->memory_usage_mb = (vertex_bytes + index_bytes) / (1024.0f * 1024.0f);

  // Calculate ACMR
  stats->acmr = mesh_calculate_acmr(mesh, 32); // Typical GPU cache size

  // Calculate bounds
  if (mesh->vertex_count > 0) {
    stats->bounds_min = mesh->vertices[0].position;
    stats->bounds_max = mesh->vertices[0].position;

    for (u32 i = 1; i < mesh->vertex_count; i++) {
      Vec3 pos = mesh->vertices[i].position;
      stats->bounds_min.x = fminf(stats->bounds_min.x, pos.x);
      stats->bounds_min.y = fminf(stats->bounds_min.y, pos.y);
      stats->bounds_min.z = fminf(stats->bounds_min.z, pos.z);
      stats->bounds_max.x = fmaxf(stats->bounds_max.x, pos.x);
      stats->bounds_max.y = fmaxf(stats->bounds_max.y, pos.y);
      stats->bounds_max.z = fmaxf(stats->bounds_max.z, pos.z);
    }

    stats->center =
        vec3_mul(vec3_add(stats->bounds_min, stats->bounds_max), 0.5f);
    Vec3 extents = vec3_sub(stats->bounds_max, stats->bounds_min);
    stats->radius = vec3_length(extents) * 0.5f;
  }
}

void mesh_print_stats(const MeshStats *stats, const char *mesh_name) {
  if (!stats)
    return;

  printf("=== Mesh Statistics: %s ===\n", mesh_name ? mesh_name : "Unknown");
  printf("Vertices: %u\n", stats->vertex_count);
  printf("Indices: %u\n", stats->index_count);
  printf("Triangles: %u\n", stats->triangle_count);
  printf("Memory: %.2f MB\n", stats->memory_usage_mb);
  printf("ACMR: %.2f\n", stats->acmr);
  printf("Bounds: (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)\n",
         stats->bounds_min.x, stats->bounds_min.y, stats->bounds_min.z,
         stats->bounds_max.x, stats->bounds_max.y, stats->bounds_max.z);
  printf("Center: (%.1f, %.1f, %.1f)\n", stats->center.x, stats->center.y,
         stats->center.z);
  printf("Radius: %.1f\n", stats->radius);
  printf("==============================\n");
}

// ============================================================================
// Batch Optimization
// ============================================================================

void mesh_batch_create(MeshBatch *batch, Mesh **meshes, u32 count) {
  if (!batch || !meshes)
    return;

  batch->meshes = meshes;
  batch->mesh_count = count;
  batch->total_vertex_count = 0;
  batch->total_index_count = 0;

  for (u32 i = 0; i < count; i++) {
    batch->total_vertex_count += meshes[i]->vertex_count;
    batch->total_index_count += meshes[i]->index_count;
  }
}

bool mesh_batch_merge(Mesh *out, const MeshBatch *batch) {
  if (!out || !batch || batch->mesh_count == 0)
    return false;

  mesh_init(out, batch->total_vertex_count, batch->total_index_count);

  u32 vertex_offset = 0;

  for (u32 i = 0; i < batch->mesh_count; i++) {
    Mesh *mesh = batch->meshes[i];

    // Copy vertices
    memcpy(&out->vertices[out->vertex_count], mesh->vertices,
           sizeof(Vertex) * mesh->vertex_count);
    out->vertex_count += mesh->vertex_count;

    // Copy and offset indices
    for (u32 j = 0; j < mesh->index_count; j++) {
      out->indices[out->index_count++] = mesh->indices[j] + vertex_offset;
    }

    vertex_offset += mesh->vertex_count;
  }

  return true;
}

void mesh_batch_free(MeshBatch *batch) {
  if (!batch)
    return;
  // Note: We don't free the individual meshes, just the batch structure
  batch->meshes = NULL;
  batch->mesh_count = 0;
}
