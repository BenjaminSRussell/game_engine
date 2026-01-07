// GLTF Material Bridge Header
#ifndef GLTF_MATERIAL_BRIDGE_H
#define GLTF_MATERIAL_BRIDGE_H

#include <assets/gltf_loader.h>
#include <renderer/gpu_material.h>

// Map GLTF material to GPUMaterial push constants
void gltf_material_to_gpu(const GLTFMaterial *gltf_mat,
                          GPUMaterialPushConstant *gpu_push);

// Load GLTF texture paths into GPUMaterial texture bindings
bool gltf_load_material_textures(const GLTFMaterial *gltf_mat,
                                 GPUMaterialTextureBindings *tex_bindings,
                                 const char *gltf_base_path);

// Register GLTF material with GPUMaterialManager (returns material ID)
u32 gltf_register_material(GPUMaterialManager *manager,
                           const GLTFMaterial *gltf_mat,
                           const char *gltf_base_path);

#endif // GLTF_MATERIAL_BRIDGE_H
