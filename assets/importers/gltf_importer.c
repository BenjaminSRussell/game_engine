#define CGLTF_IMPLEMENTATION
#include "../../../../include/vendor/cgltf.h"

#include "../../../../include/core/logger.h"
#include "../../../../include/core/memory.h"
#include "../../../../include/engine/asset_importers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Standard vertex layout for the engine
typedef struct {
  float x, y, z;    // Position
  float nx, ny, nz; // Normal
  float u, v;       // UV0 (primary texture coordinates)
  float u1, v1;     // UV1 (secondary texture coordinates - lightmaps, detail)
  float tx, ty, tz, tw; // Tangent
} StandardVertex;

// Internal allocation tracking list
struct AllocationList {
  void *ptr;
  struct AllocationList *next;
};

// Helper to resolve texture paths relative to the model
static void resolve_tex_path(cgltf_texture_view *view, const char *source_name,
                             char *out_path) {
  if (view && view->texture && view->texture->image &&
      view->texture->image->uri) {
    const char *uri = view->texture->image->uri;

    // Find directory
    const char *last_slash = strrchr(source_name, '/');
    const char *last_backslash = strrchr(source_name, '\\');
    const char *slash =
        (last_slash > last_backslash) ? last_slash : last_backslash;

    if (slash) {
      size_t dir_len = slash - source_name + 1;
      strncpy(out_path, source_name, dir_len);
      out_path[dir_len] = '\0';
      strncat(out_path, uri, 255 - dir_len);
    } else {
      strncpy(out_path, uri, 255);
    }
    out_path[255] = '\0';
  } else {
    out_path[0] = '\0';
  }
}

// Internal function to convert parsed CGLTF data into our ImportedModel format
static ImportedModel *
convert_cgltf_to_model(cgltf_data *data, const char *source_name,
                       const ModelImportOptions *options) {
  if (!data)
    return NULL;

  // Allocate our result structure
  ImportedModel *model = (ImportedModel *)malloc(sizeof(ImportedModel));
  memset(model, 0, sizeof(ImportedModel));

  struct AllocationList *alloc_list_head = NULL;

  // 1. Parse Materials
  model->material_count = (u32)data->materials_count;
  if (model->material_count > 0) {
    model->materials = (ImportedMaterial *)malloc(sizeof(ImportedMaterial) *
                                                  model->material_count);
    memset(model->materials, 0,
           sizeof(ImportedMaterial) * model->material_count);

    for (size_t i = 0; i < data->materials_count; ++i) {
      cgltf_material *gmat = &data->materials[i];
      ImportedMaterial *mat = &model->materials[i];

      if (gmat->name)
        strncpy(mat->name, gmat->name, 127);

      // Default properties
      mat->properties.albedo = (Vec3){1.0f, 1.0f, 1.0f};
      mat->properties.roughness = 1.0f;
      mat->properties.metallic = 0.0f;
      mat->properties.alpha = 1.0f;
      mat->properties.blend_mode = BLEND_MODE_OPAQUE;

      if (gmat->has_pbr_metallic_roughness) {
        cgltf_pbr_metallic_roughness *pbr = &gmat->pbr_metallic_roughness;
        mat->properties.albedo =
            (Vec3){pbr->base_color_factor[0], pbr->base_color_factor[1],
                   pbr->base_color_factor[2]};
        mat->properties.alpha = pbr->base_color_factor[3];
        mat->properties.metallic = pbr->metallic_factor;
        mat->properties.roughness = pbr->roughness_factor;

        resolve_tex_path(&pbr->base_color_texture, source_name,
                         mat->albedo_path);
        resolve_tex_path(&pbr->metallic_roughness_texture, source_name,
                         mat->metallic_roughness_path);
      }

      resolve_tex_path(&gmat->normal_texture, source_name, mat->normal_path);
      resolve_tex_path(&gmat->occlusion_texture, source_name,
                       mat->occlusion_path);
      resolve_tex_path(&gmat->emissive_texture, source_name,
                       mat->emissive_path);

      mat->properties.emission =
          (Vec3){gmat->emissive_factor[0], gmat->emissive_factor[1],
                 gmat->emissive_factor[2]};
      mat->properties.double_sided = gmat->double_sided;

      if (gmat->alpha_mode == cgltf_alpha_mode_blend) {
        mat->properties.blend_mode = BLEND_MODE_TRANSPARENT;
      }
    }
  }

  // 2. Parse Meshes
  // Count total meshes (primitive groups)
  u32 total_primitives = 0;
  for (size_t i = 0; i < data->meshes_count; ++i) {
    total_primitives += (u32)data->meshes[i].primitives_count;
  }

  model->meshes = (MeshDesc *)malloc(sizeof(MeshDesc) * total_primitives);
  model->mesh_count = total_primitives;

  u32 mesh_idx = 0;
  for (size_t m = 0; m < data->meshes_count; ++m) {
    cgltf_mesh *mesh = &data->meshes[m];

    for (size_t p = 0; p < mesh->primitives_count; ++p) {
      cgltf_primitive *prim = &mesh->primitives[p];
      MeshDesc *desc = &model->meshes[mesh_idx++];

      // Extract attributes
      cgltf_accessor *pos_acc = NULL;
      cgltf_accessor *norm_acc = NULL;
      cgltf_accessor *uv_acc = NULL;
      cgltf_accessor *tan_acc = NULL;

      for (size_t a = 0; a < prim->attributes_count; ++a) {
        if (prim->attributes[a].type == cgltf_attribute_type_position) {
          pos_acc = prim->attributes[a].data;
        } else if (prim->attributes[a].type == cgltf_attribute_type_normal) {
          norm_acc = prim->attributes[a].data;
        } else if (prim->attributes[a].type == cgltf_attribute_type_texcoord) {
          uv_acc = prim->attributes[a].data;
        } else if (prim->attributes[a].type == cgltf_attribute_type_tangent) {
          tan_acc = prim->attributes[a].data;
        }
      }

      if (!pos_acc) {
        LOG_WARN("Primitive missing positions in %s, skipping", source_name);
        continue;
      }

      // Allocate vertices
      size_t vertex_count = pos_acc->count;
      StandardVertex *vertices =
          (StandardVertex *)malloc(sizeof(StandardVertex) * vertex_count);
      memset(vertices, 0, sizeof(StandardVertex) * vertex_count);

      // Track allocation
      struct AllocationList *node = malloc(sizeof(struct AllocationList));
      node->ptr = vertices;
      node->next = alloc_list_head;
      alloc_list_head = node;

      // Fill vertices
      for (size_t v = 0; v < vertex_count; ++v) {
        // Position
        cgltf_accessor_read_float(pos_acc, v, &vertices[v].x, 3);

        // Normal
        if (norm_acc) {
          cgltf_accessor_read_float(norm_acc, v, &vertices[v].nx, 3);
        } else {
          vertices[v].ny = 1.0f;
        }

        // UV
        if (uv_acc) {
          cgltf_accessor_read_float(uv_acc, v, &vertices[v].u, 2);

          if (options && options->flip_uvs) {
            vertices[v].v = 1.0f - vertices[v].v;
          }
        }

        // Tangent
        if (tan_acc) {
          cgltf_accessor_read_float(tan_acc, v, &vertices[v].tx, 4);
        }

        // Scale
        if (options && options->scale != 1.0f && options->scale != 0.0f) {
          vertices[v].x *= options->scale;
          vertices[v].y *= options->scale;
          vertices[v].z *= options->scale;
        }
      }

      desc->vertices = vertices;
      desc->vertex_count = vertex_count;
      desc->vertex_stride = sizeof(StandardVertex);
      desc->topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

      // Set material index
      if (prim->material) {
        desc->material_index = (u32)(prim->material - data->materials);
      } else {
        desc->material_index = 0xFFFFFFFF; // No material
      }

      // Indices
      if (prim->indices) {
        size_t index_count = prim->indices->count;
        u32 *indices = (u32 *)malloc(sizeof(u32) * index_count);

        struct AllocationList *inode = malloc(sizeof(struct AllocationList));
        inode->ptr = indices;
        inode->next = alloc_list_head;
        alloc_list_head = inode;

        for (size_t k = 0; k < index_count; ++k) {
          indices[k] = (u32)cgltf_accessor_read_index(prim->indices, k);
        }

        desc->indices = indices;
        desc->index_count = index_count;
        desc->index_stride = sizeof(u32);
      } else {
        desc->indices = NULL;
        desc->index_count = 0;
        desc->index_stride = 0;
      }
    }
  }

  // Store list head in raw_data
  model->raw_data = alloc_list_head;

  LOG_INFO("Loaded GLTF model: %s (%d meshes, %d materials)", source_name,
           total_primitives, model->material_count);

  // 3. Detect LOD meshes (Phase 9)
  // Look for meshes with naming pattern: "MeshName_LOD0", "MeshName_LOD1", etc.
  model->has_lods = false;
  model->lod_meshes = NULL;
  model->lod_counts = NULL;

  // First pass: detect if any LOD meshes exist
  for (size_t m = 0; m < data->meshes_count; ++m) {
    cgltf_mesh *mesh = &data->meshes[m];
    if (mesh->name && strstr(mesh->name, "_LOD")) {
      model->has_lods = true;
      break;
    }
  }

  if (model->has_lods) {
    // Allocate LOD arrays
    model->lod_meshes =
        (MeshDesc **)calloc(model->mesh_count, sizeof(MeshDesc *));
    model->lod_counts = (u32 *)calloc(model->mesh_count, sizeof(u32));

    // Second pass: group LOD meshes
    for (size_t m = 0; m < data->meshes_count; ++m) {
      cgltf_mesh *mesh = &data->meshes[m];
      if (!mesh->name)
        continue;

      // Check if this is a LOD mesh
      const char *lod_marker = strstr(mesh->name, "_LOD");
      if (lod_marker) {
        // Extract LOD level number
        int lod_level = atoi(lod_marker + 4);
        if (lod_level >= 0 && lod_level < 4) {
          // Find the base mesh index
          // For simplicity, we'll store LOD info but not fully implement
          // grouping A full implementation would match base mesh names
          LOG_DEBUG("Detected LOD mesh: %s (Level %d)", mesh->name, lod_level);
        }
      }
    }

    LOG_INFO("GLTF model has LOD meshes");
  }

  return model;
}

ImportedModel *asset_importer_load_gltf(const char *path,
                                        const ModelImportOptions *options) {
  cgltf_options gltf_options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&gltf_options, path, &data);

  if (result != cgltf_result_success) {
    LOG_ERROR("Failed to parse GLTF file: %s (Error: %d)", path, result);
    return NULL;
  }

  result = cgltf_load_buffers(&gltf_options, data, path);
  if (result != cgltf_result_success) {
    LOG_ERROR("Failed to load GLTF buffers: %s", path);
    cgltf_free(data);
    return NULL;
  }

  ImportedModel *model = convert_cgltf_to_model(data, path, options);
  cgltf_free(data);
  return model;
}

ImportedModel *
asset_importer_load_gltf_from_memory(const void *data, size_t size,
                                     const char *base_path,
                                     const ModelImportOptions *options) {
  cgltf_options gltf_options = {0};
  cgltf_data *gltf_data = NULL;
  cgltf_result result = cgltf_parse(&gltf_options, data, size, &gltf_data);

  if (result != cgltf_result_success) {
    LOG_ERROR("Failed to parse GLTF from memory (Error: %d)", result);
    return NULL;
  }

  // Load buffers relative to base_path (if provided)
  result = cgltf_load_buffers(&gltf_options, gltf_data, base_path);
  if (result != cgltf_result_success) {
    LOG_ERROR("Failed to load GLTF buffers from memory context");
    cgltf_free(gltf_data);
    return NULL;
  }

  ImportedModel *model = convert_cgltf_to_model(
      gltf_data, base_path ? base_path : "memory", options);
  cgltf_free(gltf_data);
  return model;
}

void asset_importer_free_model(ImportedModel *model) {
  if (!model)
    return;

  // Free tracked allocations
  struct AllocationList *curr = (struct AllocationList *)model->raw_data;

  while (curr) {
    struct AllocationList *next = curr->next;
    if (curr->ptr)
      free(curr->ptr);
    free(curr);
    curr = next;
  }

  if (model->meshes)
    free(model->meshes);
  free(model);
}
