// src/engine/core/lod_generator.c
//
// Implementation of runtime LOD generation for meshes.

#include <core/lod_generator.h>
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Simple mesh decimation using uniform triangle skipping
// A production implementation would use edge collapse with quadric error
// metrics
static bool decimate_mesh(Mesh *source, Mesh *dest, f32 reduction_factor) {
  if (!source || !dest || reduction_factor <= 0.0f ||
      reduction_factor >= 1.0f) {
    return false;
  }

  u32 target_triangle_count =
      (u32)((1.0f - reduction_factor) * (source->index_count / 3));
  if (target_triangle_count < 1)
    target_triangle_count = 1;

  u32 stride = (source->index_count / 3) / target_triangle_count;
  if (stride < 1)
    stride = 1;

  dest->vertex_count = 0;
  dest->index_count = 0;

  // Simple decimation: keep every Nth triangle
  for (u32 i = 0; i < source->index_count; i += stride * 3) {
    if (i + 2 >= source->index_count)
      break;

    u32 i0 = source->indices[i];
    u32 i1 = source->indices[i + 1];
    u32 i2 = source->indices[i + 2];

    // Ensure capacity
    if (dest->vertex_count + 3 > dest->vertex_capacity) {
      LOG_WARN("LOD generator: vertex capacity exceeded");
      return false;
    }
    if (dest->index_count + 3 > dest->index_capacity) {
      LOG_WARN("LOD generator: index capacity exceeded");
      return false;
    }

    // Add vertices (simplified - no deduplication)
    u32 new_i0 = dest->vertex_count;
    u32 new_i1 = dest->vertex_count + 1;
    u32 new_i2 = dest->vertex_count + 2;

    dest->vertices[dest->vertex_count++] = source->vertices[i0];
    dest->vertices[dest->vertex_count++] = source->vertices[i1];
    dest->vertices[dest->vertex_count++] = source->vertices[i2];

    dest->indices[dest->index_count++] = new_i0;
    dest->indices[dest->index_count++] = new_i1;
    dest->indices[dest->index_count++] = new_i2;
  }

  LOG_DEBUG("LOD generated: %d -> %d triangles (%.1f%% reduction)",
            source->index_count / 3, dest->index_count / 3,
            reduction_factor * 100.0f);

  return true;
}

bool lod_generate_single(Mesh *source, Mesh *dest, f32 reduction_factor) {
  return decimate_mesh(source, dest, reduction_factor);
}

bool lod_generate_levels(Mesh *source, Mesh *lod_array, u32 num_levels,
                         const f32 *reduction_factors) {
  if (!source || !lod_array || !reduction_factors || num_levels == 0) {
    LOG_ERROR("Invalid parameters for LOD generation");
    return false;
  }

  for (u32 i = 0; i < num_levels; i++) {
    if (!decimate_mesh(source, &lod_array[i], reduction_factors[i])) {
      LOG_ERROR("Failed to generate LOD level %d", i);
      return false;
    }
  }

  LOG_INFO("Generated %d LOD levels successfully", num_levels);
  return true;
}

void lod_calculate_reduction_factors(u32 num_levels, f32 *out_factors) {
  if (!out_factors || num_levels == 0)
    return;

  // Progressive reduction: each level reduces more triangles
  // Level 0: 50%, Level 1: 75%, Level 2: 90%, etc.
  for (u32 i = 0; i < num_levels; i++) {
    f32 t = (f32)(i + 1) / (f32)(num_levels + 1);
    out_factors[i] = 0.5f + (t * 0.45f); // Range from 0.5 to 0.95
  }
}
