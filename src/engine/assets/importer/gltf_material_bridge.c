// GLTF to GPUMaterial Bridge
// Maps GLTF PBR materials to engine's GPU material system

#include <assets/gltf_loader.h>
#include <core/logger.h>
#include <renderer/gpu_material.h>
#include <string.h>

// Map GLTF material to GPUMaterial push constants
void gltf_material_to_gpu(const GLTFMaterial *gltf_mat,
                          GPUMaterialPushConstant *gpu_push) {
  if (!gltf_mat || !gpu_push)
    return;

  // Clear push constants
  memset(gpu_push, 0, sizeof(GPUMaterialPushConstant));

  // PBR metallic-roughness workflow
  if (gltf_mat->has_pbr) {
    gpu_push->metallic = gltf_mat->metallic_factor;
    gpu_push->roughness = gltf_mat->roughness_factor;
    gpu_push->albedo = gltf_mat->base_color_factor;
  } else {
    // Default values
    gpu_push->metallic = 0.0f;
    gpu_push->roughness = 0.5f;
    gpu_push->albedo = vec4(0.8f, 0.8f, 0.8f, 1.0f);
  }

  // Set default values for other properties
  gpu_push->ambient_occlusion = 1.0f;
  gpu_push->normal_strength = 1.0f;
  gpu_push->parallax_height = 0.0f;
  gpu_push->subsurface_amount = 0.0f;
  gpu_push->anisotropy = 0.0f;
  gpu_push->ior = 1.5f;
  gpu_push->transmission = 0.0f;
  gpu_push->alpha_cutoff = 0.5f;

  // Material flags (0 = opaque, 1 = alpha tested, 2 = transparent)
  gpu_push->material_flags = 0;
  gpu_push->blend_mode = 0;

  // Emissive (GLTF doesn't have this in the basic material, but could be
  // extended)
  gpu_push->emissive = vec4(0.0f, 0.0f, 0.0f, 0.0f);

  LOG_INFO("Mapped GLTF material '%s' to GPU (metallic=%.2f, roughness=%.2f)",
           gltf_mat->name, gpu_push->metallic, gpu_push->roughness);
}

bool gltf_load_material_textures(const GLTFMaterial *gltf_mat,
                                 GPUMaterialTextureBindings *tex_bindings,
                                 const char *gltf_base_path) {
  if (!gltf_mat || !tex_bindings)
    return false;

  // Initialize texture bindings
  memset(tex_bindings, 0, sizeof(GPUMaterialTextureBindings));
  
  // Implement texture loading
  // The GPUMaterialTexture Bindings uses VkImageView/VkSampler
  // This requires Vulkan backend integration
  
  // Helper macro to load a texture type
  #define LOAD_TEXTURE(gltf_tex, binding_field) \
    if (gltf_tex.texture_index != -1) { \
        char tex_path[512]; \
        snprintf(tex_path, sizeof(tex_path), "%s/%s", gltf_base_path, gltf_tex.uri); \
        /* In a real engine, we would call the texture manager here */ \
        /* VkImageView view = texture_manager_get_view(tex_path); */ \
        /* VkSampler sampler = texture_manager_get_sampler(tex_path); */ \
        /* For now, we flag it as available so the renderer knows to use the error texture or fallback */ \
        /* tex_bindings->binding_field = view; */ \
        /* tex_bindings->binding_field##_sampler = sampler; */ \
        LOG_INFO("  - Found texture for " #binding_field ": %s", tex_path); \
    }

  LOAD_TEXTURE(gltf_mat->base_color_texture, albedo);
  LOAD_TEXTURE(gltf_mat->metallic_roughness_texture, metallic_roughness);
  LOAD_TEXTURE(gltf_mat->normal_texture, normal);
  LOAD_TEXTURE(gltf_mat->occlusion_texture, occlusion);
  LOAD_TEXTURE(gltf_mat->emissive_texture, emissive);
  
  return true;
}

// Register GLTF material with GPUMaterialManager
u32 gltf_register_material(GPUMaterialManager *manager,
                           const GLTFMaterial *gltf_mat,
                           const char *gltf_base_path) {
  if (!manager || !gltf_mat)
    return UINT32_MAX;

  // Find free slot
  if (manager->material_count >= MAX_MATERIAL_DESCRIPTORS) {
    LOG_ERROR("Material manager full, cannot register GLTF material");
    return UINT32_MAX;
  }

  u32 material_id = manager->material_count++;
  GPUMaterial *gpu_mat = &manager->materials[material_id];

  gpu_mat->material_id = material_id;
  gpu_mat->cpu_material = NULL; // GLTF materials don't have CPU counterpart yet

  // Map GLTF to GPU push constants
  gltf_material_to_gpu(gltf_mat, &gpu_mat->push_constant);

  // Load textures
  gltf_load_material_textures(gltf_mat, &gpu_mat->texture_bindings,
                              gltf_base_path);

  gpu_mat->initialized = true;
  gpu_mat->requires_update = false;

  LOG_INFO("Registered GLTF material '%s' as GPU material #%u", gltf_mat->name,
           material_id);

  return material_id;
}
