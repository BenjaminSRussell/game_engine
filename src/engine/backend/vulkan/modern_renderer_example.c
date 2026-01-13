// Example: Modern Renderer using Dynamic Rendering
// Demonstrates migration from VkRenderPass to Dynamic Rendering API

#include <rendering/dynamic_rendering.h>
#include <rendering/vulkan_dispatch.h>
#include <core/logger.h>

#ifdef VULKAN_BUILD

// Example modern renderer structure (replaces old render pass system)
typedef struct {
    VkDevice device;
    VkCommandBuffer cmd_buffer;
    
    // Image views for rendering (NO VkFramebuffer!)
    VkImageView swapchain_view;
    VkImageView depth_view;
    
    u32 width;
    u32 height;
    
} ModernRenderer;

// Initialize modern renderer
bool modern_renderer_init(ModernRenderer* renderer, VkDevice device, 
                          VkImageView swapchain_view, VkImageView depth_view,
                          u32 width, u32 height) {
    if (!renderer) return false;
    
    renderer->device = device;
    renderer->swapchain_view = swapchain_view;
    renderer->depth_view = depth_view;
    renderer->width = width;
    renderer->height = height;
    
    // Initialize dynamic rendering (checks for VK 1.3 or extension)
    if (!dynamic_rendering_init(device)) {
        LOG_ERROR("Dynamic Rendering not supported on this device");
        return false;
    }
    
    LOG_INFO("Modern renderer initialized with Dynamic Rendering");
    return true;
}

// Render frame using dynamic rendering (NO render passes!)
void modern_renderer_render_frame(ModernRenderer* renderer) {
    if (!renderer || !renderer->cmd_buffer) return;
    
    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    g_vk_dispatch.BeginCommandBuffer(renderer->cmd_buffer, &begin_info);
    
    // Setup dynamic rendering info (replaces VkRenderPassBeginInfo)
    float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    DynamicRenderingInfo render_info = dynamic_rendering_info_with_depth(
        renderer->swapchain_view,
        renderer->depth_view,
        renderer->width,
        renderer->height,
        true, clear_color,   // Clear color
        true, 1.0f           // Clear depth
    );
    
    // Begin rendering (replaces vkCmdBeginRenderPass)
    dynamic_rendering_begin(renderer->cmd_buffer, &render_info);
    
    // *** DRAW COMMANDS GO HERE ***
    // - Bind pipeline
    // - Bind vertex buffers
    // - Draw calls
    // (No changes needed - same as before!)
    
    // End rendering (replaces vkCmdEndRenderPass)
    dynamic_rendering_end(renderer->cmd_buffer);
    
    // End command buffer
    g_vk_dispatch.EndCommandBuffer(renderer->cmd_buffer);
}

// Example: G-Buffer pass with TBDR optimization (Apple Silicon, mobile)
void modern_renderer_gbuffer_pass_tbdr(ModernRenderer* renderer,
                                       VkImageView albedo_view,
                                       VkImageView normal_view,
                                       VkImageView depth_view) {
    if (!renderer || !renderer->cmd_buffer) return;
    
    // Setup multiple render targets for G-Buffer
    DynamicRenderTarget color_attachments[2];
    
    // Albedo
    color_attachments[0].image_view = albedo_view;
    color_attachments[0].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachments[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachments[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    memset(&color_attachments[0].clear_value, 0, sizeof(VkClearValue));
    
    // Normals
    color_attachments[1].image_view = normal_view;
    color_attachments[1].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachments[1].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachments[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    memset(&color_attachments[1].clear_value, 0, sizeof(VkClearValue));
    
    // Depth
    DynamicRenderTarget depth_attachment;
    depth_attachment.image_view = depth_view;
    depth_attachment.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.clear_value.depthStencil.depth = 1.0f;
    
    DynamicRenderingInfo render_info = {
        .color_attachments = color_attachments,
        .color_attachment_count = 2,
        .depth_attachment = &depth_attachment,
        .render_area = {{0, 0}, {renderer->width, renderer->height}},
        .layer_count = 1,
        .use_local_read = dynamic_rendering_has_local_read()  // TBDR optimization
    };
    
    dynamic_rendering_begin(renderer->cmd_buffer, &render_info);
    
    // Render geometry to G-Buffer
    // On TBDR GPUs (Apple Silicon), G-Buffer data stays in tile memory
    // for the lighting pass (if we use local_read)
    
    dynamic_rendering_end(renderer->cmd_buffer);
    
    LOG_DEBUG("G-Buffer pass complete (TBDR: %s)",
              render_info.use_local_read ? "enabled" : "disabled");
}

// Cleanup
void modern_renderer_shutdown(ModernRenderer* renderer) {
    if (!renderer) return;
    
    // No VkRenderPass or VkFramebuffer to destroy!
    // Just cleanup image views (if owned by renderer)
    
    LOG_INFO("Modern renderer shutdown");
}

#endif // VULKAN_BUILD
