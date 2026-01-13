#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <renderer/lighting.h>
#include <renderer/vulkan.h>
#include <stdbool.h>

// Deferred Rendering G-Buffer Layout
// Attachment 0: Position (RGB16F) + Depth (A16F)
// Attachment 1: Normal (RGB16F) + Roughness (A8)
// Attachment 2: Albedo (RGBA8)
// Attachment 3: Metallic (R8) + AO (G8) + Emissive (BA16F)
// Attachment 4: Depth/Stencil (D24S8)

#define GBUFFER_ATTACHMENT_POSITION 0
#define GBUFFER_ATTACHMENT_NORMAL 1
#define GBUFFER_ATTACHMENT_ALBEDO 2
#define GBUFFER_ATTACHMENT_MATERIAL 3
#define GBUFFER_ATTACHMENT_DEPTH 4
#define GBUFFER_ATTACHMENT_COUNT 5

typedef struct {
  VkImage images[GBUFFER_ATTACHMENT_COUNT];
  VkImageView views[GBUFFER_ATTACHMENT_COUNT];
  VkDeviceMemory memory[GBUFFER_ATTACHMENT_COUNT];
  VkFormat formats[GBUFFER_ATTACHMENT_COUNT];
  u32 width;
  u32 height;
} GBuffer;

typedef struct {
  // PBR Material Properties
  Vec4 albedo;   // Base color (RGB) + Alpha
  f32 metallic;  // 0 = dielectric, 1 = metal
  f32 roughness; // 0 = smooth, 1 = rough
  f32 ao;        // Ambient occlusion
  Vec3 emissive; // Self-illumination

  // Texture indices (for bindless textures)
  u32 albedo_texture;
  u32 normal_texture;
  u32 metallic_roughness_texture;
  u32 ao_texture;
  u32 emissive_texture;
} PBRMaterial;

typedef struct {
  VulkanRenderer *vk_renderer;
  GBuffer gbuffer;

  // Render passes
  VkRenderPass geometry_pass;
  VkRenderPass lighting_pass;
  VkFramebuffer geometry_framebuffer;

  // Pipelines
  VkPipeline geometry_pipeline;
  VkPipeline lighting_pipeline;
  VkPipelineLayout geometry_layout;
  VkPipelineLayout lighting_layout;

  // Descriptor sets
  VkDescriptorSetLayout material_set_layout;
  VkDescriptorSetLayout lighting_set_layout;
  VkDescriptorPool descriptor_pool;

  // Uniform buffers
  VkBuffer camera_ubo;
  VkDeviceMemory camera_ubo_memory;
  VkBuffer lights_ssbo;
  VkDeviceMemory lights_ssbo_memory;

  // State
  bool initialized;
  u32 frame_index;
} DeferredRenderer;

// Lifecycle
bool deferred_renderer_init(DeferredRenderer *renderer,
                            VulkanRenderer *vk_renderer, u32 width, u32 height);
void deferred_renderer_shutdown(DeferredRenderer *renderer);
bool deferred_renderer_resize(DeferredRenderer *renderer, u32 new_width,
                              u32 new_height);

// G-Buffer management
bool gbuffer_create(GBuffer *gbuffer, VkDevice device,
                    VkPhysicalDevice physical_device, u32 width, u32 height);
void gbuffer_destroy(GBuffer *gbuffer, VkDevice device);

// Rendering
void deferred_renderer_begin_geometry_pass(DeferredRenderer *renderer,
                                           VkCommandBuffer cmd);
void deferred_renderer_end_geometry_pass(DeferredRenderer *renderer,
                                         VkCommandBuffer cmd);
void deferred_renderer_lighting_pass(DeferredRenderer *renderer,
                                     VkCommandBuffer cmd,
                                     LightingSystem *lighting);

// Material system
void deferred_renderer_bind_material(DeferredRenderer *renderer,
                                     VkCommandBuffer cmd,
                                     PBRMaterial *material);
void deferred_renderer_draw_mesh(DeferredRenderer *renderer,
                                 VkCommandBuffer cmd, VkBuffer vertex_buffer,
                                 VkBuffer index_buffer, u32 index_count);

// Debug visualization
void deferred_renderer_visualize_gbuffer(DeferredRenderer *renderer,
                                         VkCommandBuffer cmd,
                                         u32 attachment_index);

#endif // DEFERRED_RENDERER_H
