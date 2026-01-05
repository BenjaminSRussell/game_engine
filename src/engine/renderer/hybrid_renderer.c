// src/engine/renderer/hybrid_renderer.c
//
// Purpose: Hybrid rendering pipeline combining raytracing and rasterization
// Provides optimal performance by using raytracing for specific effects and rasterization for base rendering

#include "../include/render/ray_tracing.h"
#include "../include/render/vulkan.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD

// Hybrid rendering modes
typedef enum {
    HYBRID_MODE_RASTER_ONLY = 0,        // Traditional rasterization only
    HYBRID_MODE_RASTER_GI = 1,          // Rasterization + raytraced GI
    HYBRID_MODE_RASTER_REFLECTIONS = 2, // Rasterization + raytraced reflections
    HYBRID_MODE_RASTER_GI_REFLECTIONS = 3, // Rasterization + GI + reflections
    HYBRID_MODE_RAYTRACED = 4           // Full raytracing
} HybridMode;

// Render pass types
typedef enum {
    RENDER_PASS_GEOMETRY = 0,      // Base geometry pass
    RENDER_PASS_GBUFFER = 1,       // G-buffer generation
    RENDER_PASS_RAYTRACED_GI = 2,  // Raytraced GI
    RENDER_PASS_RAYTRACED_REFL = 3, // Raytraced reflections
    RENDER_PASS_COMPOSITING = 4,   // Final compositing
    RENDER_PASS_POST_PROCESS = 5   // Post-processing
} RenderPassType;

// Hybrid renderer configuration
typedef struct {
    HybridMode mode;
    bool enable_gi;
    bool enable_reflections;
    bool enable_shadows;
    bool enable_ambient_occlusion;
    float gi_quality;
    float reflection_quality;
    u32 max_raytraced_pixels;
    bool adaptive_quality;
} HybridConfig;

// G-buffer targets
typedef struct {
    VkImage albedo_image;
    VkImageView albedo_view;
    VkDeviceMemory albedo_memory;
    
    VkImage normal_image;
    VkImageView normal_view;
    VkDeviceMemory normal_memory;
    
    VkImage roughness_image;
    VkImageView roughness_view;
    VkDeviceMemory roughness_memory;
    
    VkImage metalness_image;
    VkImageView metalness_view;
    VkDeviceMemory metalness_memory;
    
    VkImage depth_image;
    VkImageView depth_view;
    VkDeviceMemory depth_memory;
    
    VkImage motion_vector_image;
    VkImageView motion_vector_view;
    VkDeviceMemory motion_vector_memory;
} GBuffer;

// Hybrid renderer system
typedef struct {
    VulkanRenderer* renderer;
    
    // Configuration
    HybridConfig config;
    
    // G-buffer
    GBuffer gbuffer;
    
    // Raytracing systems
    bool gi_enabled;
    bool reflections_enabled;
    
    // Render passes
    VkRenderPass geometry_render_pass;
    VkRenderPass gbuffer_render_pass;
    VkRenderPass raytraced_render_pass;
    VkRenderPass compositing_render_pass;
    VkRenderPass post_process_render_pass;
    
    // Framebuffers
    VkFramebuffer geometry_framebuffer;
    VkFramebuffer gbuffer_framebuffer;
    VkFramebuffer raytraced_framebuffer;
    VkFramebuffer compositing_framebuffer;
    VkFramebuffer post_process_framebuffer;
    
    // Command buffers
    VkCommandBuffer geometry_command_buffer;
    VkCommandBuffer gbuffer_command_buffer;
    VkCommandBuffer raytraced_command_buffer;
    VkCommandBuffer compositing_command_buffer;
    VkCommandBuffer post_process_command_buffer;
    
    // Synchronization
    VkSemaphore geometry_semaphore;
    VkSemaphore gbuffer_semaphore;
    VkSemaphore raytraced_semaphore;
    VkSemaphore compositing_semaphore;
    VkSemaphore post_process_semaphore;
    
    VkFence geometry_fence;
    VkFence gbuffer_fence;
    VkFence raytraced_fence;
    VkFence compositing_fence;
    VkFence post_process_fence;
    
    // Performance tracking
    f32 last_frame_time;
    u32 geometry_time_us;
    u32 raytracing_time_us;
    u32 compositing_time_us;
    u32 total_pixels_rendered;
    
} HybridRenderer;

static HybridRenderer g_hybrid_renderer = {0};

// Internal helper functions
static bool hybrid_create_gbuffer(void);
static bool hybrid_create_render_passes(void);
static bool hybrid_create_framebuffers(void);
static bool hybrid_create_synchronization(void);
static void hybrid_render_geometry_pass(VkCommandBuffer command_buffer);
static void hybrid_render_gbuffer_pass(VkCommandBuffer command_buffer);
static void hybrid_render_raytraced_pass(VkCommandBuffer command_buffer);
static void hybrid_render_compositing_pass(VkCommandBuffer command_buffer);
static void hybrid_render_post_process_pass(VkCommandBuffer command_buffer);

// Initialize hybrid renderer
bool hybrid_init(VulkanRenderer* renderer, HybridConfig config) {
    if (!renderer) return false;
    
    g_hybrid_renderer.renderer = renderer;
    g_hybrid_renderer.config = config;
    
    // Create G-buffer
    if (!hybrid_create_gbuffer()) {
        LOG_ERROR("Failed to create hybrid renderer G-buffer");
        return false;
    }
    
    // Create render passes
    if (!hybrid_create_render_passes()) {
        LOG_ERROR("Failed to create hybrid renderer render passes");
        return false;
    }
    
    // Create framebuffers
    if (!hybrid_create_framebuffers()) {
        LOG_ERROR("Failed to create hybrid renderer framebuffers");
        return false;
    }
    
    // Create synchronization objects
    if (!hybrid_create_synchronization()) {
        LOG_ERROR("Failed to create hybrid renderer synchronization");
        return false;
    }
    
    // Initialize raytracing systems based on configuration
    g_hybrid_renderer.gi_enabled = config.enable_gi && (config.mode == HYBRID_MODE_RASTER_GI || 
                                                        config.mode == HYBRID_MODE_RASTER_GI_REFLECTIONS || 
                                                        config.mode == HYBRID_MODE_RAYTRACED);
    
    g_hybrid_renderer.reflections_enabled = config.enable_reflections && (config.mode == HYBRID_MODE_RASTER_REFLECTIONS || 
                                                                        config.mode == HYBRID_MODE_RASTER_GI_REFLECTIONS || 
                                                                        config.mode == HYBRID_MODE_RAYTRACED);
    
    LOG_INFO("Hybrid renderer initialized with mode %d", config.mode);
    return true;
}

// Create G-buffer for hybrid rendering
static bool hybrid_create_gbuffer(void) {
    VulkanRenderer* renderer = g_hybrid_renderer.renderer;
    u32 width = renderer->swapchain_extent.width;
    u32 height = renderer->swapchain_extent.height;
    
    // Create albedo image
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.albedo_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create albedo image");
        return false;
    }
    
    // Allocate memory for albedo image
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.albedo_image, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = vulkan_find_memory_type(renderer->physical_device, mem_reqs.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.albedo_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate albedo memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.albedo_image, g_hybrid_renderer.gbuffer.albedo_memory, 0);
    
    // Create albedo image view
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = g_hybrid_renderer.gbuffer.albedo_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.albedo_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create albedo view");
        return false;
    }
    
    // Create normal image (RGB16F for world normals)
    image_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.normal_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create normal image");
        return false;
    }
    
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.normal_image, &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.normal_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate normal memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.normal_image, g_hybrid_renderer.gbuffer.normal_memory, 0);
    
    view_info.image = g_hybrid_renderer.gbuffer.normal_image;
    view_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.normal_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create normal view");
        return false;
    }
    
    // Create roughness image (R16F)
    image_info.format = VK_FORMAT_R16_SFLOAT;
    
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.roughness_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create roughness image");
        return false;
    }
    
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.roughness_image, &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.roughness_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate roughness memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.roughness_image, g_hybrid_renderer.gbuffer.roughness_memory, 0);
    
    view_info.image = g_hybrid_renderer.gbuffer.roughness_image;
    view_info.format = VK_FORMAT_R16_SFLOAT;
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.roughness_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create roughness view");
        return false;
    }
    
    // Create metalness image (R16F)
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.metalness_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create metalness image");
        return false;
    }
    
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.metalness_image, &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.metalness_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate metalness memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.metalness_image, g_hybrid_renderer.gbuffer.metalness_memory, 0);
    
    view_info.image = g_hybrid_renderer.gbuffer.metalness_image;
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.metalness_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create metalness view");
        return false;
    }
    
    // Create depth image
    image_info.format = VK_FORMAT_D32_SFLOAT;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.depth_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create depth image");
        return false;
    }
    
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.depth_image, &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.depth_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate depth memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.depth_image, g_hybrid_renderer.gbuffer.depth_memory, 0);
    
    view_info.image = g_hybrid_renderer.gbuffer.depth_image;
    view_info.format = VK_FORMAT_D32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.depth_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create depth view");
        return false;
    }
    
    // Create motion vector image (RG16F)
    image_info.format = VK_FORMAT_R16G16_SFLOAT;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateImage(renderer->device, &image_info, NULL, &g_hybrid_renderer.gbuffer.motion_vector_image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create motion vector image");
        return false;
    }
    
    vkGetImageMemoryRequirements(renderer->device, g_hybrid_renderer.gbuffer.motion_vector_image, &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_hybrid_renderer.gbuffer.motion_vector_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate motion vector memory");
        return false;
    }
    
    vkBindImageMemory(renderer->device, g_hybrid_renderer.gbuffer.motion_vector_image, g_hybrid_renderer.gbuffer.motion_vector_memory, 0);
    
    view_info.image = g_hybrid_renderer.gbuffer.motion_vector_image;
    view_info.format = VK_FORMAT_R16G16_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (vkCreateImageView(renderer->device, &view_info, NULL, &g_hybrid_renderer.gbuffer.motion_vector_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create motion vector view");
        return false;
    }
    
    return true;
}

// Create render passes for hybrid rendering
static bool hybrid_create_render_passes(void) {
    VulkanRenderer* renderer = g_hybrid_renderer.renderer;
    
    // HYBRID-001: Create render passes for each stage
    // This would involve:
    // 1. Geometry pass for base rendering
    // 2. G-buffer pass for material properties
    // 3. Raytraced pass for GI/reflections
    // 4. Compositing pass for final output
    // 5. Post-processing pass for final touches
    
    return true;
}

// Create framebuffers for hybrid rendering
static bool hybrid_create_framebuffers(void) {
    VulkanRenderer* renderer = g_hybrid_renderer.renderer;
    
    // HYBRID-002: Create framebuffers for each render pass
    // This would involve binding the appropriate images to each framebuffer
    
    return true;
}

// Create synchronization objects
static bool hybrid_create_synchronization(void) {
    VulkanRenderer* renderer = g_hybrid_renderer.renderer;
    
    // Create semaphores
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_hybrid_renderer.geometry_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create geometry semaphore");
        return false;
    }
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_hybrid_renderer.gbuffer_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create gbuffer semaphore");
        return false;
    }
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_hybrid_renderer.raytraced_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create raytraced semaphore");
        return false;
    }
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_hybrid_renderer.compositing_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create compositing semaphore");
        return false;
    }
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_hybrid_renderer.post_process_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create post process semaphore");
        return false;
    }
    
    // Create fences
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_hybrid_renderer.geometry_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create geometry fence");
        return false;
    }
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_hybrid_renderer.gbuffer_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create gbuffer fence");
        return false;
    }
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_hybrid_renderer.raytraced_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create raytraced fence");
        return false;
    }
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_hybrid_renderer.compositing_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create compositing fence");
        return false;
    }
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_hybrid_renderer.post_process_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create post process fence");
        return false;
    }
    
    return true;
}

// Render geometry pass
static void hybrid_render_geometry_pass(VkCommandBuffer command_buffer) {
    // HYBRID-003: Implement geometry pass
    // This would render the base geometry using traditional rasterization
    LOG_TRACE("Rendering geometry pass");
}

// Render G-buffer pass
static void hybrid_render_gbuffer_pass(VkCommandBuffer command_buffer) {
    // HYBRID-004: Implement G-buffer pass
    // This would generate material properties for raytracing
    LOG_TRACE("Rendering G-buffer pass");
}

// Render raytraced pass
static void hybrid_render_raytraced_pass(VkCommandBuffer command_buffer) {
    // HYBRID-005: Implement raytraced pass
    // This would perform raytracing for GI and/or reflections
    if (g_hybrid_renderer.gi_enabled) {
        // Render raytraced GI
        LOG_TRACE("Rendering raytraced GI");
    }
    
    if (g_hybrid_renderer.reflections_enabled) {
        // Render raytraced reflections
        LOG_TRACE("Rendering raytraced reflections");
    }
}

// Render compositing pass
static void hybrid_render_compositing_pass(VkCommandBuffer command_buffer) {
    // HYBRID-006: Implement compositing pass
    // This would combine rasterized and raytraced results
    LOG_TRACE("Rendering compositing pass");
}

// Render post-process pass
static void hybrid_render_post_process_pass(VkCommandBuffer command_buffer) {
    // HYBRID-007: Implement post-process pass
    // This would apply final post-processing effects
    LOG_TRACE("Rendering post-process pass");
}

// Main hybrid rendering function
void hybrid_render_frame(VkCommandBuffer command_buffer, VkAccelerationStructureKHR tlas, 
                         const mat4* view_matrix, const mat4* proj_matrix, const mat4* prev_view_matrix) {
    // Render based on hybrid mode
    switch (g_hybrid_renderer.config.mode) {
        case HYBRID_MODE_RASTER_ONLY:
            hybrid_render_geometry_pass(command_buffer);
            break;
            
        case HYBRID_MODE_RASTER_GI:
            hybrid_render_geometry_pass(command_buffer);
            hybrid_render_gbuffer_pass(command_buffer);
            hybrid_render_raytraced_pass(command_buffer);
            hybrid_render_compositing_pass(command_buffer);
            break;
            
        case HYBRID_MODE_RASTER_REFLECTIONS:
            hybrid_render_geometry_pass(command_buffer);
            hybrid_render_gbuffer_pass(command_buffer);
            hybrid_render_raytraced_pass(command_buffer);
            hybrid_render_compositing_pass(command_buffer);
            break;
            
        case HYBRID_MODE_RASTER_GI_REFLECTIONS:
            hybrid_render_geometry_pass(command_buffer);
            hybrid_render_gbuffer_pass(command_buffer);
            hybrid_render_raytraced_pass(command_buffer);
            hybrid_render_compositing_pass(command_buffer);
            break;
            
        case HYBRID_MODE_RAYTRACED:
            hybrid_render_raytraced_pass(command_buffer);
            break;
    }
    
    // Always do post-processing
    hybrid_render_post_process_pass(command_buffer);
}

// Get G-buffer views for raytracing
void hybrid_get_gbuffer_views(VkImageView* albedo, VkImageView* normal, VkImageView* roughness, 
                             VkImageView* metalness, VkImageView* depth, VkImageView* motion_vector) {
    if (albedo) *albedo = g_hybrid_renderer.gbuffer.albedo_view;
    if (normal) *normal = g_hybrid_renderer.gbuffer.normal_view;
    if (roughness) *roughness = g_hybrid_renderer.gbuffer.roughness_view;
    if (metalness) *metalness = g_hybrid_renderer.gbuffer.metalness_view;
    if (depth) *depth = g_hybrid_renderer.gbuffer.depth_view;
    if (motion_vector) *motion_vector = g_hybrid_renderer.gbuffer.motion_vector_view;
}

// Set hybrid rendering mode
void hybrid_set_mode(HybridMode mode) {
    if (g_hybrid_renderer.config.mode == mode) return;
    
    g_hybrid_renderer.config.mode = mode;
    
    // Update raytracing system states
    g_hybrid_renderer.gi_enabled = g_hybrid_renderer.config.enable_gi && 
                                (mode == HYBRID_MODE_RASTER_GI || mode == HYBRID_MODE_RASTER_GI_REFLECTIONS || mode == HYBRID_MODE_RAYTRACED);
    
    g_hybrid_renderer.reflections_enabled = g_hybrid_renderer.config.enable_reflections && 
                                          (mode == HYBRID_MODE_RASTER_REFLECTIONS || mode == HYBRID_MODE_RASTER_GI_REFLECTIONS || mode == HYBRID_MODE_RAYTRACED);
    
    LOG_INFO("Hybrid renderer mode set to %d", mode);
}

// Get performance statistics
void hybrid_get_stats(f32* last_frame_time, u32* geometry_time_us, u32* raytracing_time_us, 
                     u32* compositing_time_us, u32* total_pixels) {
    if (last_frame_time) *last_frame_time = g_hybrid_renderer.last_frame_time;
    if (geometry_time_us) *geometry_time_us = g_hybrid_renderer.geometry_time_us;
    if (raytracing_time_us) *raytracing_time_us = g_hybrid_renderer.raytracing_time_us;
    if (compositing_time_us) *compositing_time_us = g_hybrid_renderer.compositing_time_us;
    if (total_pixels) *total_pixels = g_hybrid_renderer.total_pixels_rendered;
}

// Cleanup hybrid renderer
void hybrid_cleanup(void) {
    VulkanRenderer* renderer = g_hybrid_renderer.renderer;
    if (!renderer) return;
    
    vkDeviceWaitIdle(renderer->device);
    
    // Cleanup G-buffer
    if (g_hybrid_renderer.gbuffer.albedo_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.albedo_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.albedo_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.albedo_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.albedo_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.albedo_memory, NULL);
    }
    
    if (g_hybrid_renderer.gbuffer.normal_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.normal_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.normal_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.normal_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.normal_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.normal_memory, NULL);
    }
    
    if (g_hybrid_renderer.gbuffer.roughness_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.roughness_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.roughness_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.roughness_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.roughness_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.roughness_memory, NULL);
    }
    
    if (g_hybrid_renderer.gbuffer.metalness_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.metalness_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.metalness_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.metalness_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.metalness_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.metalness_memory, NULL);
    }
    
    if (g_hybrid_renderer.gbuffer.depth_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.depth_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.depth_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.depth_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.depth_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.depth_memory, NULL);
    }
    
    if (g_hybrid_renderer.gbuffer.motion_vector_view) {
        vkDestroyImageView(renderer->device, g_hybrid_renderer.gbuffer.motion_vector_view, NULL);
    }
    if (g_hybrid_renderer.gbuffer.motion_vector_image) {
        vkDestroyImage(renderer->device, g_hybrid_renderer.gbuffer.motion_vector_image, NULL);
    }
    if (g_hybrid_renderer.gbuffer.motion_vector_memory) {
        vkFreeMemory(renderer->device, g_hybrid_renderer.gbuffer.motion_vector_memory, NULL);
    }
    
    // Cleanup synchronization
    if (g_hybrid_renderer.geometry_semaphore) {
        vkDestroySemaphore(renderer->device, g_hybrid_renderer.geometry_semaphore, NULL);
    }
    if (g_hybrid_renderer.gbuffer_semaphore) {
        vkDestroySemaphore(renderer->device, g_hybrid_renderer.gbuffer_semaphore, NULL);
    }
    if (g_hybrid_renderer.raytraced_semaphore) {
        vkDestroySemaphore(renderer->device, g_hybrid_renderer.raytraced_semaphore, NULL);
    }
    if (g_hybrid_renderer.compositing_semaphore) {
        vkDestroySemaphore(renderer->device, g_hybrid_renderer.compositing_semaphore, NULL);
    }
    if (g_hybrid_renderer.post_process_semaphore) {
        vkDestroySemaphore(renderer->device, g_hybrid_renderer.post_process_semaphore, NULL);
    }
    
    if (g_hybrid_renderer.geometry_fence) {
        vkDestroyFence(renderer->device, g_hybrid_renderer.geometry_fence, NULL);
    }
    if (g_hybrid_renderer.gbuffer_fence) {
        vkDestroyFence(renderer->device, g_hybrid_renderer.gbuffer_fence, NULL);
    }
    if (g_hybrid_renderer.raytraced_fence) {
        vkDestroyFence(renderer->device, g_hybrid_renderer.raytraced_fence, NULL);
    }
    if (g_hybrid_renderer.compositing_fence) {
        vkDestroyFence(renderer->device, g_hybrid_renderer.compositing_fence, NULL);
    }
    if (g_hybrid_renderer.post_process_fence) {
        vkDestroyFence(renderer->device, g_hybrid_renderer.post_process_fence, NULL);
    }
    
    // Cleanup render passes and framebuffers
    if (g_hybrid_renderer.geometry_render_pass) {
        vkDestroyRenderPass(renderer->device, g_hybrid_renderer.geometry_render_pass, NULL);
    }
    if (g_hybrid_renderer.gbuffer_render_pass) {
        vkDestroyRenderPass(renderer->device, g_hybrid_renderer.gbuffer_render_pass, NULL);
    }
    if (g_hybrid_renderer.raytraced_render_pass) {
        vkDestroyRenderPass(renderer->device, g_hybrid_renderer.raytraced_render_pass, NULL);
    }
    if (g_hybrid_renderer.compositing_render_pass) {
        vkDestroyRenderPass(renderer->device, g_hybrid_renderer.compositing_render_pass, NULL);
    }
    if (g_hybrid_renderer.post_process_render_pass) {
        vkDestroyRenderPass(renderer->device, g_hybrid_renderer.post_process_render_pass, NULL);
    }
    
    if (g_hybrid_renderer.geometry_framebuffer) {
        vkDestroyFramebuffer(renderer->device, g_hybrid_renderer.geometry_framebuffer, NULL);
    }
    if (g_hybrid_renderer.gbuffer_framebuffer) {
        vkDestroyFramebuffer(renderer->device, g_hybrid_renderer.gbuffer_framebuffer, NULL);
    }
    if (g_hybrid_renderer.raytraced_framebuffer) {
        vkDestroyFramebuffer(renderer->device, g_hybrid_renderer.raytraced_framebuffer, NULL);
    }
    if (g_hybrid_renderer.compositing_framebuffer) {
        vkDestroyFramebuffer(renderer->device, g_hybrid_renderer.compositing_framebuffer, NULL);
    }
    if (g_hybrid_renderer.post_process_framebuffer) {
        vkDestroyFramebuffer(renderer->device, g_hybrid_renderer.post_process_framebuffer, NULL);
    }
    
    memset(&g_hybrid_renderer, 0, sizeof(HybridRenderer));
    LOG_INFO("Hybrid renderer cleaned up");
}

#endif // VULKAN_BUILD
