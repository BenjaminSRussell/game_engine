// include/render/gpu_material.h
//
// Purpose: GPU-side material binding and management.
// Handles Vulkan descriptor sets, push constants, and pipeline integration
// for physically-based materials.
//
#ifndef GPU_MATERIAL_H
#define GPU_MATERIAL_H

#include "include/rendering/material.h"
#include "include/rendering/texture_system.h"
#include "include/rendering/vulkan.h"
#include <common.h>

// Maximum descriptor sets per material
#define MAX_MATERIAL_DESCRIPTORS 256

// GPU material push constants (256 bytes max on most GPUs)
typedef struct {
  // PBR Properties (aligned to 16 bytes)
  f32 metallic;
  f32 roughness;
  f32 ambient_occlusion;
  f32 normal_strength;

  f32 parallax_height;
  f32 subsurface_amount;
  f32 anisotropy;
  f32 ior;

  f32 transmission;
  f32 alpha_cutoff;
  u32 material_flags;
  u32 blend_mode;

  // Color properties
  Vec4 albedo;
  Vec4 emissive;

  // Padding to 256 bytes (16 * 16)
  f32 padding[48];
} GPUMaterialPushConstant;

// GPU texture bindings for a material
typedef struct {
  VkImageView image_views[8]; // Albedo, normal, metallic, roughness, AO,
                              // emissive, height
  VkSampler samplers[8];
  VkDescriptorSet descriptor_set;
  bool initialized;
} GPUMaterialTextureBindings;

// GPU material instance
typedef struct {
  u32 material_id;
  Material *cpu_material;
  GPUMaterialPushConstant push_constant;
  GPUMaterialTextureBindings texture_bindings;
  VkPipeline pipeline; // Material-specific pipeline
  VkPipelineLayout pipeline_layout;
  bool requires_update;
  bool initialized;
} GPUMaterial;

// GPU material manager
typedef struct {
  GPUMaterial materials[MAX_MATERIAL_DESCRIPTORS];
  u32 material_count;

  // Shared resources
  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout material_descriptor_layout;
  VkPipelineLayout shared_pipeline_layout;

  // Samplers (shared across all materials)
  VkSampler linear_sampler;
  VkSampler nearest_sampler;
  VkSampler anisotropic_sampler;

  // Render state
  VkRenderPass render_pass;
  VkDevice device;
  VkPhysicalDevice physical_device;

  bool initialized;
} GPUMaterialManager;

// ==============================================================================
// GPU Material Manager Lifecycle
// ==============================================================================

// Initialize GPU material manager
bool gpu_material_init(GPUMaterialManager *manager, VkDevice device,
                       VkPhysicalDevice physical_device,
                       VkRenderPass render_pass);

// Shutdown GPU material manager
void gpu_material_shutdown(GPUMaterialManager *manager);

// ==============================================================================
// GPU Material Registration and Updates
// ==============================================================================

// Register material for GPU rendering
bool gpu_material_register(GPUMaterialManager *manager, Material *material);

// Update GPU material from CPU material
bool gpu_material_update(GPUMaterialManager *manager, u32 material_id);

// Unregister material (cleanup GPU resources)
void gpu_material_unregister(GPUMaterialManager *manager, u32 material_id);

// Get GPU material by ID
GPUMaterial *gpu_material_get(GPUMaterialManager *manager, u32 material_id);

// ==============================================================================
// Texture Binding
// ==============================================================================

// Bind texture to material slot
bool gpu_material_bind_texture(GPUMaterialManager *manager, u32 material_id,
                               u32 slot, VkImageView image_view,
                               VkSampler sampler);

// Create descriptor set for material
bool gpu_material_create_descriptor_set(GPUMaterialManager *manager,
                                        u32 material_id);

// Update descriptor set with new texture bindings
bool gpu_material_update_descriptor_set(GPUMaterialManager *manager,
                                        u32 material_id);

// ==============================================================================
// Pipeline Creation and Management
// ==============================================================================

// Create pipeline for material based on its properties
bool gpu_material_create_pipeline(GPUMaterialManager *manager, u32 material_id,
                                  VkShaderModule vert_shader,
                                  VkShaderModule frag_shader);

// Destroy material pipeline
void gpu_material_destroy_pipeline(GPUMaterialManager *manager,
                                   u32 material_id);

// Get pipeline for material
VkPipeline gpu_material_get_pipeline(GPUMaterialManager *manager,
                                     u32 material_id);

// ==============================================================================
// Rendering Commands
// ==============================================================================

// Bind material for rendering (sets descriptor sets, pipelines, push constants)
void gpu_material_bind(GPUMaterialManager *manager, VkCommandBuffer cmd_buffer,
                       u32 material_id);

// Push material constants to shader
void gpu_material_push_constants(GPUMaterialManager *manager,
                                 VkCommandBuffer cmd_buffer, u32 material_id);

// ==============================================================================
// Sampler Management
// ==============================================================================

// Create material samplers (linear, nearest, anisotropic)
bool gpu_material_create_samplers(GPUMaterialManager *manager,
                                  f32 max_anisotropy);

// Destroy samplers
void gpu_material_destroy_samplers(GPUMaterialManager *manager);

// Get sampler for filtering mode
VkSampler gpu_material_get_sampler(GPUMaterialManager *manager,
                                   TextureFilter filter);

// ==============================================================================
// Descriptor Pool Management
// ==============================================================================

// Create descriptor pool for materials
bool gpu_material_create_descriptor_pool(GPUMaterialManager *manager,
                                         u32 pool_size);

// Destroy descriptor pool
void gpu_material_destroy_descriptor_pool(GPUMaterialManager *manager);

// Allocate descriptor set from pool
VkDescriptorSet
gpu_material_allocate_descriptor_set(GPUMaterialManager *manager);

// Free descriptor set back to pool
void gpu_material_free_descriptor_set(GPUMaterialManager *manager,
                                      VkDescriptorSet set);

// ==============================================================================
// Push Constant Management
// ==============================================================================

// Update push constants from material properties
void gpu_material_update_push_constants(GPUMaterialManager *manager,
                                        u32 material_id);

// Get push constant data
GPUMaterialPushConstant *
gpu_material_get_push_constants(GPUMaterialManager *manager, u32 material_id);

// ==============================================================================
// Batch Rendering
// ==============================================================================

// Sort materials by pipeline for efficient batch rendering
void gpu_material_sort_for_batching(GPUMaterialManager *manager,
                                    u32 *material_ids, u32 count);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

// Log material GPU information
void gpu_material_log_info(GPUMaterialManager *manager, u32 material_id);

// Log GPU material manager statistics
void gpu_material_log_statistics(GPUMaterialManager *manager);

// Validate material GPU resources
bool gpu_material_validate(GPUMaterialManager *manager, u32 material_id);

#endif // GPU_MATERIAL_H
