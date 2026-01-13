// GLTF Asset Loader using cgltf
// Loads meshes, materials, and textures from glTF 2.0 files (.gltf/.glb)
//
// Implementation Plan:
// 1. Parse GLTF file with cgltf
// 2. Extract mesh data (vertices, indices)
// 3. Extract PBR materials (albedo, normal, roughness, metallic)
// 4. Create Vulkan buffers for meshes
// 5. Load textures into GPU

#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <rendering/mesh.h>
#include <rendering/renderer.h>
#include <string.h>

// Platform-specific includes
#ifdef __APPLE__
// On macOS, use Metal backend
// TODO: Implement Metal GLTF loader
#include <rendering/renderer.h>
#else
// On other platforms, use Vulkan
#include <platform/vulkan/vk_instance.h>
#include <platform/vulkan/vk_device.h>
#include <vulkan/vulkan.h>
#endif

// Forward declare VulkanRenderer if not defined
#ifndef VULKAN_RENDERER_DEFINED
#define VULKAN_RENDERER_DEFINED
typedef struct VulkanRenderer VulkanRenderer;
#endif

// Include cgltf implementation in exactly one source file
#define CGLTF_IMPLEMENTATION
#include "include/vendor/cgltf.h"

// Forward declarations
typedef struct GLTFLoadResult GLTFLoadResult;
typedef struct GLTFMesh GLTFMesh;
typedef struct GLTFMaterial GLTFMaterial;

// Loaded mesh data
typedef struct GLTFMesh {
  char name[256];
  Vertex *vertices;
  u32 vertex_count;
  u32 *indices;
  u32 index_count;
  u32 material_index;
  bool has_normals;
  bool has_uvs;
} GLTFMesh;

// Loaded PBR material
typedef struct GLTFMaterial {
  char name[256];
  Vec4 base_color_factor;
  float metallic_factor;
  float roughness_factor;
  char albedo_texture[512];
  char normal_texture[512];
  char metallic_roughness_texture[512];
  char ao_texture[512];
  bool has_albedo;
  bool has_normal;
  bool has_pbr;
  bool has_ao;
} GLTFMaterial;

// Complete load result
typedef struct GLTFLoadResult {
  GLTFMesh *meshes;
  u32 mesh_count;
  GLTFMaterial *materials;
  u32 material_count;
  bool success;
  char error[256];
} GLTFLoadResult;

// Extract vertex data from cgltf accessor
static void extract_vec3(const cgltf_accessor *accessor, u32 index, Vec3 *out) {
  if (!accessor || index >= accessor->count) {
    *out = vec3(0, 0, 0);
    return;
  }
  float values[3] = {0};
  cgltf_accessor_read_float(accessor, index, values, 3);
  *out = vec3(values[0], values[1], values[2]);
}

static void extract_vec2(const cgltf_accessor *accessor, u32 index, Vec2 *out) {
  if (!accessor || index >= accessor->count) {
    *out = vec2(0, 0);
    return;
  }
  float values[2] = {0};
  cgltf_accessor_read_float(accessor, index, values, 2);
  *out = vec2(values[0], values[1]);
}

// Load GLTF/GLB file
GLTFLoadResult gltf_load(const char *filepath) {
  GLTFLoadResult result = {0};

  if (!filepath) {
    snprintf(result.error, sizeof(result.error), "Null filepath");
    return result;
  }

  // Parse GLTF/GLB file
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result parse_result = cgltf_parse_file(&options, filepath, &data);

  if (parse_result != cgltf_result_success) {
    snprintf(result.error, sizeof(result.error), "Failed to parse GLTF: %d",
             (int)parse_result);
    return result;
  }

  // Load buffers (required for accessing mesh data)
  cgltf_result load_result = cgltf_load_buffers(&options, data, filepath);
  if (load_result != cgltf_result_success) {
    snprintf(result.error, sizeof(result.error),
             "Failed to load GLTF buffers: %d", (int)load_result);
    cgltf_free(data);
    return result;
  }

  // Validate data
  cgltf_result validate_result = cgltf_validate(data);
  if (validate_result != cgltf_result_success) {
    LOG_WARN("GLTF validation warning: %d", (int)validate_result);
    // Continue anyway, validation is just a warning
  }

  LOG_INFO("GLTF loaded: %s", filepath);
  LOG_INFO("  Meshes: %zu", data->meshes_count);
  LOG_INFO("  Materials: %zu", data->materials_count);
  LOG_INFO("  Textures: %zu", data->textures_count);

  // Allocate result arrays
  result.meshes = calloc(data->meshes_count * 16, sizeof(GLTFMesh));
  result.materials = calloc(data->materials_count, sizeof(GLTFMaterial));

  // Extract materials
  for (size_t i = 0; i < data->materials_count; i++) {
    cgltf_material *mat = &data->materials[i];
    GLTFMaterial *out_mat = &result.materials[i];

    if (mat->name) {
      strncpy(out_mat->name, mat->name, sizeof(out_mat->name) - 1);
    }

    if (mat->has_pbr_metallic_roughness) {
      cgltf_pbr_metallic_roughness *pbr = &mat->pbr_metallic_roughness;
      out_mat->base_color_factor =
          vec4(pbr->base_color_factor[0], pbr->base_color_factor[1],
               pbr->base_color_factor[2], pbr->base_color_factor[3]);
      out_mat->metallic_factor = pbr->metallic_factor;
      out_mat->roughness_factor = pbr->roughness_factor;
      out_mat->has_pbr = true;

      if (pbr->base_color_texture.texture &&
          pbr->base_color_texture.texture->image) {
        strncpy(out_mat->albedo_texture,
                pbr->base_color_texture.texture->image->uri,
                sizeof(out_mat->albedo_texture) - 1);
        out_mat->has_albedo = true;
      }

      if (pbr->metallic_roughness_texture.texture &&
          pbr->metallic_roughness_texture.texture->image) {
        strncpy(out_mat->metallic_roughness_texture,
                pbr->metallic_roughness_texture.texture->image->uri,
                sizeof(out_mat->metallic_roughness_texture) - 1);
      }
    }

    if (mat->normal_texture.texture && mat->normal_texture.texture->image) {
      strncpy(out_mat->normal_texture, mat->normal_texture.texture->image->uri,
              sizeof(out_mat->normal_texture) - 1);
      out_mat->has_normal = true;
    }

    if (mat->occlusion_texture.texture &&
        mat->occlusion_texture.texture->image) {
      strncpy(out_mat->ao_texture, mat->occlusion_texture.texture->image->uri,
              sizeof(out_mat->ao_texture) - 1);
      out_mat->has_ao = true;
    }

    result.material_count++;
  }

  // Extract meshes (each mesh can have multiple primitives)
  u32 mesh_idx = 0;
  for (size_t i = 0; i < data->meshes_count; i++) {
    cgltf_mesh *mesh = &data->meshes[i];

    for (size_t p = 0; p < mesh->primitives_count; p++) {
      cgltf_primitive *prim = &mesh->primitives[p];

      if (prim->type != cgltf_primitive_type_triangles) {
        LOG_WARN("Skipping non-triangle primitive in mesh %zu", i);
        continue;
      }

      GLTFMesh *out_mesh = &result.meshes[mesh_idx];
      snprintf(out_mesh->name, sizeof(out_mesh->name), "%s_%zu",
               mesh->name ? mesh->name : "mesh", p);

      // Find position, normal, texcoord accessors
      cgltf_accessor *pos_acc = NULL;
      cgltf_accessor *norm_acc = NULL;
      cgltf_accessor *uv_acc = NULL;

      for (size_t a = 0; a < prim->attributes_count; a++) {
        if (prim->attributes[a].type == cgltf_attribute_type_position) {
          pos_acc = prim->attributes[a].data;
        } else if (prim->attributes[a].type == cgltf_attribute_type_normal) {
          norm_acc = prim->attributes[a].data;
        } else if (prim->attributes[a].type == cgltf_attribute_type_texcoord) {
          uv_acc = prim->attributes[a].data;
        }
      }

      if (!pos_acc) {
        LOG_WARN("Mesh %s has no position data", out_mesh->name);
        continue;
      }

      out_mesh->vertex_count = (u32)pos_acc->count;
      out_mesh->vertices = calloc(out_mesh->vertex_count, sizeof(Vertex));
      out_mesh->has_normals = norm_acc != NULL;
      out_mesh->has_uvs = uv_acc != NULL;

      // Extract vertices
      for (u32 v = 0; v < out_mesh->vertex_count; v++) {
        extract_vec3(pos_acc, v, &out_mesh->vertices[v].position);
        if (norm_acc) {
          extract_vec3(norm_acc, v, &out_mesh->vertices[v].normal);
        }
        if (uv_acc) {
          extract_vec2(uv_acc, v, &out_mesh->vertices[v].uv);
        }
      }

      // Extract indices
      if (prim->indices) {
        out_mesh->index_count = (u32)prim->indices->count;
        out_mesh->indices = calloc(out_mesh->index_count, sizeof(u32));
        for (u32 idx = 0; idx < out_mesh->index_count; idx++) {
          out_mesh->indices[idx] =
              (u32)cgltf_accessor_read_index(prim->indices, idx);
        }
      }

      // Material index
      if (prim->material) {
        out_mesh->material_index = (u32)(prim->material - data->materials);
      }

      LOG_INFO("  Loaded mesh: %s (%u verts, %u indices)", out_mesh->name,
               out_mesh->vertex_count, out_mesh->index_count);

      mesh_idx++;
    }
  }
  result.mesh_count = mesh_idx;

  cgltf_free(data);
  result.success = true;
  return result;
}

// Free loaded GLTF data
void gltf_free(GLTFLoadResult *result) {
  if (!result)
    return;

  for (u32 i = 0; i < result->mesh_count; i++) {
    if (result->meshes[i].vertices)
      free(result->meshes[i].vertices);
    if (result->meshes[i].indices)
      free(result->meshes[i].indices);
  }
  if (result->meshes)
    free(result->meshes);
  if (result->materials)
    free(result->materials);

  memset(result, 0, sizeof(GLTFLoadResult));
}

#ifdef VULKAN_BUILD
// Create Vulkan buffers from GLTF mesh
bool gltf_create_mesh_buffers(VulkanRenderer *renderer, GLTFMesh *gltf_mesh,
                              VkBuffer *out_vertex_buffer,
                              VkDeviceMemory *out_vertex_memory,
                              VkBuffer *out_index_buffer,
                              VkDeviceMemory *out_index_memory) {
  if (!renderer || !gltf_mesh || !out_vertex_buffer)
    return false;

  VkDeviceSize vertex_size = gltf_mesh->vertex_count * sizeof(Vertex);

  if (!vulkan_create_buffer(renderer, vertex_size,
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            out_vertex_buffer, out_vertex_memory)) {
    LOG_ERROR("Failed to create vertex buffer for GLTF mesh");
    return false;
  }
  return true;
}
#else
// Stub implementation for non-Vulkan platforms
bool gltf_create_mesh_buffers(void *renderer, void *gltf_mesh,
                              void *out_vertex_buffer,
                              void *out_vertex_memory,
                              void *out_index_buffer,
                              void *out_index_memory) {
  return false;
}
#endif
