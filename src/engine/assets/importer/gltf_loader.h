#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <rendering/mesh.h>
#include <math/vec4.h>
#include <stdbool.h>

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

GLTFLoadResult gltf_load(const char *filepath);
void gltf_free(GLTFLoadResult *result);

// Stub for creating vulkan buffers if VulkanRenderer is available
bool gltf_create_mesh_buffers(void *renderer, GLTFMesh *gltf_mesh,
                              void *out_vertex_buffer,
                              void *out_vertex_memory,
                              void *out_index_buffer,
                              void *out_index_memory);

#endif // GLTF_LOADER_H
