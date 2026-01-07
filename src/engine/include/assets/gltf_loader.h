// GLTF Asset Loader Header
// Public API for loading glTF 2.0 files

#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <common.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <include/rendering/mesh.h>

#ifdef VULKAN_BUILD
#include <include/rendering/vulkan.h>
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;

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
  f32 metallic_factor;
  f32 roughness_factor;
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

// Load GLTF/GLB file
GLTFLoadResult gltf_load(const char *filepath);

// Free loaded GLTF data
void gltf_free(GLTFLoadResult *result);

// Create Vulkan buffers from loaded mesh
bool gltf_create_mesh_buffers(VulkanRenderer *renderer, GLTFMesh *gltf_mesh,
                              VkBuffer *out_vertex_buffer,
                              VkDeviceMemory *out_vertex_memory,
                              VkBuffer *out_index_buffer,
                              VkDeviceMemory *out_index_memory);

#endif // GLTF_LOADER_H
