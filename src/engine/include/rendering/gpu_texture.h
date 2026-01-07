// include/render/gpu_texture.h
//
// Purpose: GPU-side texture binding, sampler management, and image resource handling.
// Manages Vulkan image creation, views, samplers, and descriptor sets for textures.
//
#ifndef GPU_TEXTURE_H
#define GPU_TEXTURE_H

#include <common.h>
#include "include/rendering/texture_system.h"
#include "include/rendering/vulkan.h"

// GPU texture resource
typedef struct {
    u32 texture_id;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    VkSampler sampler;
    VkDescriptorSet descriptor_set;
    u32 width, height;
    VkFormat format;
    VkImageLayout layout;
    bool initialized;
} GPUTexture;

// GPU texture manager
#define MAX_GPU_TEXTURES 2048

typedef struct {
    GPUTexture textures[MAX_GPU_TEXTURES];
    u32 texture_count;

    // Shared samplers
    VkSampler linear_sampler;
    VkSampler nearest_sampler;
    VkSampler anisotropic_sampler[3];  // 4x, 8x, 16x

    // Descriptor management
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_layout;

    // Device info
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool transfer_command_pool;

    bool initialized;
} GPUTextureManager;

// ==============================================================================
// GPU Texture Manager Lifecycle
// ==============================================================================

// Initialize GPU texture manager
bool gpu_texture_init(GPUTextureManager* manager, VkDevice device,
                      VkPhysicalDevice physical_device, VkCommandPool transfer_pool);

// Shutdown GPU texture manager
void gpu_texture_shutdown(GPUTextureManager* manager);

// ==============================================================================
// Texture Creation and Upload
// ==============================================================================

// Create GPU texture from pixel data
bool gpu_texture_create(GPUTextureManager* manager, u32 texture_id, const char* name,
                        u8* pixel_data, u32 width, u32 height,
                        TextureFormat format);

// Upload texture to GPU (asynchronously via transfer queue)
bool gpu_texture_upload_async(GPUTextureManager* manager, u32 texture_id,
                              u8* pixel_data, u32 data_size);

// Create texture from existing Vulkan image
bool gpu_texture_from_image(GPUTextureManager* manager, u32 texture_id,
                            VkImage image, VkImageView view, u32 width, u32 height);

// Delete GPU texture
void gpu_texture_delete(GPUTextureManager* manager, u32 texture_id);

// Get GPU texture
GPUTexture* gpu_texture_get(GPUTextureManager* manager, u32 texture_id);

// ==============================================================================
// Image Management
// ==============================================================================

// Create Vulkan image
VkImage gpu_texture_create_image(GPUTextureManager* manager,
                                 u32 width, u32 height, VkFormat format,
                                 VkImageUsageFlags usage);

// Create image view
VkImageView gpu_texture_create_image_view(GPUTextureManager* manager,
                                          VkImage image, VkFormat format);

// Transition image layout
void gpu_texture_transition_layout(VkCommandBuffer cmd_buffer,
                                   VkImage image, VkImageLayout old_layout,
                                   VkImageLayout new_layout);

// Copy buffer to image
void gpu_texture_copy_buffer_to_image(VkCommandBuffer cmd_buffer,
                                      VkBuffer buffer, VkImage image,
                                      u32 width, u32 height);

// ==============================================================================
// Sampler Management
// ==============================================================================

// Create samplers for different filtering modes
bool gpu_texture_create_samplers(GPUTextureManager* manager, f32 max_anisotropy);

// Destroy samplers
void gpu_texture_destroy_samplers(GPUTextureManager* manager);

// Get sampler for texture filter mode
VkSampler gpu_texture_get_sampler(GPUTextureManager* manager, TextureFilter filter);

// Get anisotropic sampler with specified level
VkSampler gpu_texture_get_anisotropic_sampler(GPUTextureManager* manager, f32 level);

// ==============================================================================
// Mipmap Support
// ==============================================================================

// Generate mipmaps for texture (via compute shader or blit)
bool gpu_texture_generate_mipmaps(GPUTextureManager* manager, u32 texture_id);

// Create mipmaps from CPU data
bool gpu_texture_create_with_mipmaps(GPUTextureManager* manager, u32 texture_id,
                                     const char* name, u8** mipmap_data,
                                     u32* mipmap_sizes, u32 mipmap_count,
                                     u32 base_width, u32 base_height);

// ==============================================================================
// Descriptor Set Management
// ==============================================================================

// Create descriptor set for texture
bool gpu_texture_create_descriptor_set(GPUTextureManager* manager, u32 texture_id);

// Update descriptor set with texture binding
bool gpu_texture_update_descriptor_set(GPUTextureManager* manager, u32 texture_id);

// Bind texture descriptor set for rendering
void gpu_texture_bind_descriptor_set(GPUTextureManager* manager,
                                     VkCommandBuffer cmd_buffer,
                                     u32 texture_id, VkPipelineLayout layout,
                                     u32 set_index);

// ==============================================================================
// Image Transitions and Synchronization
// ==============================================================================

// Transition all textures to shader read layout
void gpu_texture_transition_all_to_read(GPUTextureManager* manager,
                                        VkCommandBuffer cmd_buffer);

// Transition texture for writing
void gpu_texture_transition_for_write(GPUTextureManager* manager,
                                      VkCommandBuffer cmd_buffer, u32 texture_id);

// ==============================================================================
// Texture Queries
// ==============================================================================

// Get texture dimensions
void gpu_texture_get_dimensions(GPUTextureManager* manager, u32 texture_id,
                                u32* width, u32* height);

// Get texture Vulkan format
VkFormat gpu_texture_get_format(GPUTextureManager* manager, u32 texture_id);

// Get texture layout
VkImageLayout gpu_texture_get_layout(GPUTextureManager* manager, u32 texture_id);

// ==============================================================================
// Binding Utilities
// ==============================================================================

// Bind multiple textures for rendering
void gpu_texture_bind_textures(GPUTextureManager* manager,
                               VkCommandBuffer cmd_buffer,
                               u32* texture_ids, u32 count,
                               VkPipelineLayout layout);

// Update texture in descriptor set
void gpu_texture_update_in_set(GPUTextureManager* manager, u32 texture_id,
                               VkDescriptorSet desc_set, u32 binding);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

// Log texture information
void gpu_texture_log_info(GPUTextureManager* manager, u32 texture_id);

// Log GPU texture manager statistics
void gpu_texture_log_statistics(GPUTextureManager* manager);

// Validate texture GPU resources
bool gpu_texture_validate(GPUTextureManager* manager, u32 texture_id);

// Calculate GPU memory usage for texture
u32 gpu_texture_get_memory_usage(GPUTextureManager* manager, u32 texture_id);

#endif // GPU_TEXTURE_H
