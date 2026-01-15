#include "geometry/lod/lod_generator.h"
#include "geometry/geometry_types.h"
#include "geometry/mesh.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONSTANTS AND DEFAULTS
// ============================================================================

#define LOD_MAX_VERTICES_PER_MESH 1000000
#define LOD_MIN_TRIANGLE_AREA 1e-6f
#define LOD_DEFAULT_FEATURE_ANGLE 30.0f
#define LOD_DEFAULT_WELD_DISTANCE 0.001f

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct edge_collapse_t {
  u32 vertex1;
  u32 vertex2;
  Vec3 target_position;
  f32 error;
  bool valid;
} edge_collapse_t;

typedef struct vertex_info_t {
  Vec3 position;
  Vec3 normal;
  Vec2 texcoord;
  u32 collapse_count;
  bool deleted;
  u32 *adjacent_vertices;
  u32 adjacent_count;
  u32 adjacent_capacity;
} vertex_info_t;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static f32 calculate_triangle_area(const Vec3 *v1, const Vec3 *v2,
                                   const Vec3 *v3) {
  Vec3 edge1 = vec3_sub(*v2, *v1);
  Vec3 edge2 = vec3_sub(*v3, *v1);
  Vec3 cross = vec3_cross(edge1, edge2);
  return vec3_length(cross) * 0.5f;
}

static f32 calculate_edge_error(const vertex_info_t *v1,
                                const vertex_info_t *v2) {
  // Simple distance-based error metric
  f32 distance = vec3_distance(v1->position, v2->position);

  // Add normal difference penalty
  f32 normal_diff =
      acosf(fmaxf(-1.0f, fminf(1.0f, vec3_dot(v1->normal, v2->normal))));

  // Add UV difference penalty
  f32 uv_diff = vec2_distance(v1->texcoord, v2->texcoord);

  return distance + normal_diff * 0.1f + uv_diff * 0.05f;
}

static bool is_boundary_edge(const vertex_info_t *vertices, u32 vertex_count,
                             u32 v1, u32 v2) {
  // Count how many triangles share this edge
  u32 shared_triangles = 0;

  // This is a simplified check - in a full implementation we'd need to track
  // triangle adjacency
  for (u32 i = 0; i < vertex_count; i++) {
    if (vertices[i].deleted)
      continue;

    // Check if this vertex is adjacent to both v1 and v2
    bool adjacent_to_v1 = false;
    bool adjacent_to_v2 = false;

    for (u32 j = 0; j < vertices[i].adjacent_count; j++) {
      if (vertices[i].adjacent_vertices[j] == v1)
        adjacent_to_v1 = true;
      if (vertices[i].adjacent_vertices[j] == v2)
        adjacent_to_v2 = true;
    }

    if (adjacent_to_v1 && adjacent_to_v2) {
      shared_triangles++;
    }
  }

  return shared_triangles == 1;
}

// ============================================================================
// QUADRIC ERROR METRICS SIMPLIFICATION
// ============================================================================

static vertex_info_t *create_vertex_info(const mesh_t *mesh) {
  vertex_info_t *vertices = calloc(mesh->vertex_count, sizeof(vertex_info_t));
  if (!vertices)
    return NULL;

  for (u32 i = 0; i < mesh->vertex_count; i++) {
    vertices[i].position = mesh->vertices[i].position;
    vertices[i].normal = mesh->vertices[i].normal;
    vertices[i].texcoord = mesh->vertices[i].texcoord;
    vertices[i].collapse_count = 0;
    vertices[i].deleted = false;
    vertices[i].adjacent_capacity = 8;
    vertices[i].adjacent_vertices =
        malloc(vertices[i].adjacent_capacity * sizeof(u32));
    vertices[i].adjacent_count = 0;
  }

  // Build adjacency information
  for (u32 i = 0; i < mesh->index_count; i += 3) {
    u32 i0 = mesh->indices[i];
    u32 i1 = mesh->indices[i + 1];
    u32 i2 = mesh->indices[i + 2];

    // Add adjacency for each vertex in the triangle
    u32 triangle_vertices[3] = {i0, i1, i2};

    for (u32 j = 0; j < 3; j++) {
      u32 vertex_idx = triangle_vertices[j];
      u32 next_idx = triangle_vertices[(j + 1) % 3];
      u32 prev_idx = triangle_vertices[(j + 2) % 3];

      // Add next vertex as adjacent
      bool already_adjacent = false;
      for (u32 k = 0; k < vertices[vertex_idx].adjacent_count; k++) {
        if (vertices[vertex_idx].adjacent_vertices[k] == next_idx) {
          already_adjacent = true;
          break;
        }
      }

      if (!already_adjacent) {
        if (vertices[vertex_idx].adjacent_count >=
            vertices[vertex_idx].adjacent_capacity) {
          vertices[vertex_idx].adjacent_capacity *= 2;
          vertices[vertex_idx].adjacent_vertices =
              realloc(vertices[vertex_idx].adjacent_vertices,
                      vertices[vertex_idx].adjacent_capacity * sizeof(u32));
        }
        vertices[vertex_idx]
            .adjacent_vertices[vertices[vertex_idx].adjacent_count++] =
            next_idx;
      }

      // Add previous vertex as adjacent
      already_adjacent = false;
      for (u32 k = 0; k < vertices[vertex_idx].adjacent_count; k++) {
        if (vertices[vertex_idx].adjacent_vertices[k] == prev_idx) {
          already_adjacent = true;
          break;
        }
      }

      if (!already_adjacent) {
        if (vertices[vertex_idx].adjacent_count >=
            vertices[vertex_idx].adjacent_capacity) {
          vertices[vertex_idx].adjacent_capacity *= 2;
          vertices[vertex_idx].adjacent_vertices =
              realloc(vertices[vertex_idx].adjacent_vertices,
                      vertices[vertex_idx].adjacent_capacity * sizeof(u32));
        }
        vertices[vertex_idx]
            .adjacent_vertices[vertices[vertex_idx].adjacent_count++] =
            prev_idx;
      }
    }
  }

  return vertices;
}

static void destroy_vertex_info(vertex_info_t *vertices, u32 count) {
  if (!vertices)
    return;

  for (u32 i = 0; i < count; i++) {
    free(vertices[i].adjacent_vertices);
  }

  free(vertices);
}

static edge_collapse_t *create_edge_collapse_list(const vertex_info_t *vertices,
                                                  u32 vertex_count,
                                                  u32 *collapse_count) {
  u32 max_collapses = vertex_count * 8; // Estimate
  edge_collapse_t *collapses = malloc(max_collapses * sizeof(edge_collapse_t));
  if (!collapses)
    return NULL;

  *collapse_count = 0;

  for (u32 i = 0; i < vertex_count; i++) {
    if (vertices[i].deleted)
      continue;

    for (u32 j = 0; j < vertices[i].adjacent_count; j++) {
      u32 adjacent_idx = vertices[i].adjacent_vertices[j];

      if (adjacent_idx <= i || vertices[adjacent_idx].deleted)
        continue;

      if (*collapse_count >= max_collapses) {
        // Resize array
        max_collapses *= 2;
        edge_collapse_t *new_collapses =
            realloc(collapses, max_collapses * sizeof(edge_collapse_t));
        if (!new_collapses) {
          free(collapses);
          return NULL;
        }
        collapses = new_collapses;
      }

      edge_collapse_t *collapse = &collapses[*collapse_count];
      collapse->vertex1 = i;
      collapse->vertex2 = adjacent_idx;
      collapse->error =
          calculate_edge_error(&vertices[i], &vertices[adjacent_idx]);

      // Target position is the midpoint
      collapse->target_position = vec3_lerp(
          vertices[i].position, vertices[adjacent_idx].position, 0.5f);
      collapse->valid = true;

      (*collapse_count)++;
    }
  }

  return collapses;
}

static int compare_edge_collapse(const void *a, const void *b) {
  const edge_collapse_t *ca = (const edge_collapse_t *)a;
  const edge_collapse_t *cb = (const edge_collapse_t *)b;

  if (ca->error < cb->error)
    return -1;
  if (ca->error > cb->error)
    return 1;
  return 0;
}

static mesh_t *simplify_mesh_qem(const mesh_t *source, f32 target_ratio,
                                 const lod_generation_config_t *config) {
  if (!source || target_ratio >= 1.0f || target_ratio <= 0.0f)
    return NULL;

  u32 target_triangles = (u32)(source->index_count / 3 * target_ratio);
  u32 current_triangles = source->index_count / 3;

  if (target_triangles >= current_triangles) {
    // No simplification needed
    mesh_t *result = mesh_create("simplified_copy");
    if (!result)
      return NULL;

    mesh_allocate_buffers(result, source->vertex_count, source->index_count);
    mesh_set_vertices(result, source->vertices, source->vertex_count, 0);
    mesh_set_indices(result, source->indices, source->index_count, 0);

    return result;
  }

  vertex_info_t *vertices = create_vertex_info(source);
  if (!vertices)
    return NULL;

  u32 collapse_count;
  edge_collapse_t *collapses = create_edge_collapse_list(
      vertices, source->vertex_count, &collapse_count);
  if (!collapses) {
    destroy_vertex_info(vertices, source->vertex_count);
    return NULL;
  }

  // Sort edge collapses by error
  qsort(collapses, collapse_count, sizeof(edge_collapse_t),
        compare_edge_collapse);

  // Perform edge collapses
  u32 collapsed_edges = 0;
  u32 target_collapses = current_triangles - target_triangles;

  for (u32 i = 0; i < collapse_count && collapsed_edges < target_collapses;
       i++) {
    edge_collapse_t *collapse = &collapses[i];

    if (!collapse->valid || vertices[collapse->vertex1].deleted ||
        vertices[collapse->vertex2].deleted) {
      continue;
    }

    // Check if this is a boundary edge and we should preserve it
    if (config && config->preserve_boundaries) {
      if (is_boundary_edge(vertices, source->vertex_count, collapse->vertex1,
                           collapse->vertex2)) {
        continue;
      }
    }

    // Perform the collapse
    vertex_info_t *v1 = &vertices[collapse->vertex1];
    vertex_info_t *v2 = &vertices[collapse->vertex2];

    // Move v1 to target position
    v1->position = collapse->target_position;

    // Update v1's normal (average)
    v1->normal = vec3_normalize(vec3_add(v1->normal, v2->normal));

    // Mark v2 as deleted
    v2->deleted = true;

    // Update adjacency for v1
    for (u32 j = 0; j < v2->adjacent_count; j++) {
      u32 adjacent_idx = v2->adjacent_vertices[j];

      if (adjacent_idx == collapse->vertex1)
        continue;

      // Add adjacency to v1 if not already present
      bool already_adjacent = false;
      for (u32 k = 0; k < v1->adjacent_count; k++) {
        if (v1->adjacent_vertices[k] == adjacent_idx) {
          already_adjacent = true;
          break;
        }
      }

      if (!already_adjacent) {
        if (v1->adjacent_count >= v1->adjacent_capacity) {
          v1->adjacent_capacity *= 2;
          v1->adjacent_vertices = realloc(v1->adjacent_vertices,
                                          v1->adjacent_capacity * sizeof(u32));
        }
        v1->adjacent_vertices[v1->adjacent_count++] = adjacent_idx;
      }

      // Update adjacency of the other vertex to point to v1 instead of v2
      vertex_info_t *adjacent_vertex = &vertices[adjacent_idx];
      for (u32 k = 0; k < adjacent_vertex->adjacent_count; k++) {
        if (adjacent_vertex->adjacent_vertices[k] == collapse->vertex2) {
          adjacent_vertex->adjacent_vertices[k] = collapse->vertex1;
          break;
        }
      }
    }

    collapsed_edges++;
  }

  // Count remaining vertices
  u32 new_vertex_count = 0;
  for (u32 i = 0; i < source->vertex_count; i++) {
    if (!vertices[i].deleted) {
      new_vertex_count++;
    }
  }

  // Create simplified mesh
  mesh_t *simplified = mesh_create("simplified_mesh");
  if (!simplified) {
    destroy_vertex_info(vertices, source->vertex_count);
    free(collapses);
    return NULL;
  }

  mesh_allocate_buffers(simplified, new_vertex_count, source->index_count);

  // Create vertex remapping
  u32 *vertex_remap = malloc(source->vertex_count * sizeof(u32));
  u32 vertex_index = 0;

  for (u32 i = 0; i < source->vertex_count; i++) {
    if (!vertices[i].deleted) {
      vertex_remap[i] = vertex_index;

      geometry_vertex_t *new_vertex = &simplified->vertices[vertex_index];
      new_vertex->position = vertices[i].position;
      new_vertex->normal = vertices[i].normal;
      new_vertex->texcoord = vertices[i].texcoord;
      new_vertex->tangent = source->vertices[i].tangent; // Preserve tangents

      vertex_index++;
    } else {
      vertex_remap[i] = UINT32_MAX;
    }
  }

  // Rebuild index buffer
  u32 new_index_count = 0;
  for (u32 i = 0; i < source->index_count; i += 3) {
    u32 i0 = vertex_remap[source->indices[i]];
    u32 i1 = vertex_remap[source->indices[i + 1]];
    u32 i2 = vertex_remap[source->indices[i + 2]];

    // Skip triangles with deleted vertices
    if (i0 == UINT32_MAX || i1 == UINT32_MAX || i2 == UINT32_MAX) {
      continue;
    }

    // Check for degenerate triangles
    f32 area = calculate_triangle_area(&simplified->vertices[i0].position,
                                       &simplified->vertices[i1].position,
                                       &simplified->vertices[i2].position);

    if (area < LOD_MIN_TRIANGLE_AREA) {
      continue;
    }

    simplified->indices[new_index_count++] = i0;
    simplified->indices[new_index_count++] = i1;
    simplified->indices[new_index_count++] = i2;
  }

  // Update mesh counts
  simplified->vertex_count = new_vertex_count;
  simplified->index_count = new_index_count;

  // Copy submesh information (simplified - just copy for now)
  simplified->submesh_count = source->submesh_count;
  if (source->submesh_count > 0) {
    simplified->submeshes = malloc(source->submesh_count * sizeof(submesh_t));
    memcpy(simplified->submeshes, source->submeshes,
           source->submesh_count * sizeof(submesh_t));
  }

  // Copy material information
  simplified->material_count = source->material_count;
  memcpy(simplified->material_ids, source->material_ids,
         source->material_count * sizeof(u32));

  // Recalculate bounds
  mesh_calculate_bounds(simplified);

  // Cleanup
  free(vertex_remap);
  destroy_vertex_info(vertices, source->vertex_count);
  free(collapses);

  return simplified;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

lod_generation_config_t lod_get_default_config(void) {
  lod_generation_config_t config = {0};

  // Default reduction ratios for 4 LOD levels
  config.reduction_ratios[0] = 0.75f; // LOD 1: 75% of original
  config.reduction_ratios[1] = 0.5f;  // LOD 2: 50% of original
  config.reduction_ratios[2] = 0.25f; // LOD 3: 25% of original
  config.reduction_ratios[3] = 0.1f;  // LOD 4: 10% of original

  // Quality weights
  config.quality_weights[0] = 1.0f; // Position weight
  config.quality_weights[1] = 0.5f; // Normal weight
  config.quality_weights[2] = 0.3f; // UV weight

  // Preservation options
  config.preserve_boundaries = true;
  config.preserve_seams = true;
  config.preserve_features = true;
  config.feature_angle = LOD_DEFAULT_FEATURE_ANGLE;
  config.weld_distance = LOD_DEFAULT_WELD_DISTANCE;
  config.normal_smoothing = 0.5f;

  return config;
}

lod_chain_t *
lod_generate_chain_advanced(const mesh_t *source,
                            const lod_generation_config_t *config) {
  if (!source)
    return NULL;

  lod_chain_t *chain = calloc(1, sizeof(lod_chain_t));
  if (!chain)
    return NULL;

  lod_generation_config_t defaults;
  if (!config) {
    defaults = lod_get_default_config();
    config = &defaults;
  }
  const lod_generation_config_t *cfg = config;
  chain->config = *cfg;

  // LOD 0 is the original mesh
  chain->lods[0] = (mesh_t *)source;
  chain->geometric_errors[0] = 0.0f;
  chain->switch_distances[0] = 1.0f; // Always visible at close range
  chain->lod_count = 1;

  // Generate LOD levels
  u32 max_lods =
      sizeof(cfg->reduction_ratios) / sizeof(cfg->reduction_ratios[0]);

  for (u32 i = 0; i < max_lods && chain->lod_count < MESH_MAX_LODS; i++) {
    if (cfg->reduction_ratios[i] <= 0.0f || cfg->reduction_ratios[i] >= 1.0f) {
      continue;
    }

    mesh_t *lod_mesh = simplify_mesh_qem(source, cfg->reduction_ratios[i], cfg);
    if (!lod_mesh) {
      printf("Warning: Failed to generate LOD %u\n", chain->lod_count);
      continue;
    }

    chain->lods[chain->lod_count] = lod_mesh;

    // Calculate geometric error (simplified - based on vertex reduction)
    f32 reduction_ratio = (f32)lod_mesh->vertex_count / source->vertex_count;
    chain->geometric_errors[chain->lod_count] = 1.0f - reduction_ratio;

    // Set switch distance (screen size threshold)
    chain->switch_distances[chain->lod_count] = cfg->reduction_ratios[i];

    chain->lod_count++;

    printf("Generated LOD %u: %u vertices, %u triangles (%.1f%% of original)\n",
           chain->lod_count - 1, lod_mesh->vertex_count,
           lod_mesh->index_count / 3, reduction_ratio * 100.0f);
  }

  return chain;
}

lod_chain_t *lod_generate_chain(const mesh_t *source, const f32 *ratios,
                                u32 count) {
  if (!source || !ratios || count == 0)
    return NULL;

  lod_generation_config_t config = lod_get_default_config();

  // Copy provided ratios
  u32 copy_count = fminf(count, sizeof(config.reduction_ratios) /
                                    sizeof(config.reduction_ratios[0]));
  for (u32 i = 0; i < copy_count; i++) {
    config.reduction_ratios[i] = ratios[i];
  }

  return lod_generate_chain_advanced(source, &config);
}

void lod_chain_destroy(lod_chain_t *chain) {
  if (!chain)
    return;

  // Don't destroy LOD 0 (original mesh) as it's owned by caller
  for (u32 i = 1; i < chain->lod_count; i++) {
    if (chain->lods[i]) {
      mesh_destroy(chain->lods[i]);
    }
  }

  free(chain);
}

mesh_t *qem_simplify(const mesh_t *source, f32 target_ratio) {
  lod_generation_config_t config = lod_get_default_config();
  return simplify_mesh_qem(source, target_ratio, &config);
}

mesh_t *qem_simplify_advanced(const mesh_t *source, f32 target_ratio,
                              const lod_generation_config_t *config,
                              f32 *out_error) {
  mesh_t *result = simplify_mesh_qem(source, target_ratio, config);

  if (out_error && result) {
    // Calculate error as vertex reduction ratio
    f32 reduction_ratio = (f32)result->vertex_count / source->vertex_count;
    *out_error = 1.0f - reduction_ratio;
  }

  return result;
}
