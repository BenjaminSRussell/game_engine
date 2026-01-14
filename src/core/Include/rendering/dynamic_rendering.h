// Dynamic Rendering API - Modern Vulkan 1.3 rendering without render passes
// Replaces legacy VkRenderPass and VkFramebuffer with dynamic rendering info

#ifndef DYNAMIC_RENDERING_H
#define DYNAMIC_RENDERING_H

#include <common.h>

#ifdef VULKAN_BUILD

#include <include/rendering/vulkan.h>

// Render target descriptor for dynamic rendering
typedef struct {
    VkImageView image_view;
    VkImageLayout image_layout;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkClearValue clear_value;
} DynamicRenderTarget;

// Dynamic rendering configuration
typedef struct {
    DynamicRenderTarget* color_attachments;
    u32 color_attachment_count;
    DynamicRenderTarget* depth_attachment;
    DynamicRenderTarget* stencil_attachment;
    
    VkRect2D render_area;
    u32 layer_count;
    u32 view_mask;  // For multiview/VR
    
    // TBDR optimization (Apple Silicon, mobile GPUs)
    bool use_local_read;  // VK_KHR_dynamic_rendering_local_read
} DynamicRenderingInfo;

// Initialize dynamic rendering (checks for VK 1.3 or extension)
bool dynamic_rendering_init(VkDevice device);

// Begin dynamic rendering pass
void dynamic_rendering_begin(VkCommandBuffer cmd_buffer, const DynamicRenderingInfo* info);

// End dynamic rendering pass
void dynamic_rendering_end(VkCommandBuffer cmd_buffer);

// Query capabilities
bool dynamic_rendering_is_available(void);
bool dynamic_rendering_has_local_read(void);

// Helper functions to create common configurations
DynamicRenderingInfo dynamic_rendering_info_simple(
    VkImageView color_target,
    u32 width, u32 height,
    bool clear_color, float* clear_color_value
);

DynamicRenderingInfo dynamic_rendering_info_with_depth(
    VkImageView color_target,
    VkImageView depth_target,
    u32 width, u32 height,
    bool clear_color, float* clear_color_value,
    bool clear_depth, float clear_depth_value
);

#endif // VULKAN_BUILD

#endif // DYNAMIC_RENDERING_H
