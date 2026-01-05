// Dynamic Rendering Implementation
// Modern rendering without VkRenderPass and VkFramebuffer

#include <renderer/dynamic_rendering.h>
#include <renderer/vulkan_dispatch.h>
#include <core/logger.h>
#include <string.h>

#ifdef VULKAN_BUILD

static bool g_dynamic_rendering_available = false;
static bool g_has_local_read = false;

bool dynamic_rendering_init(VkDevice device) {
    (void)device;
    
    // Check if dispatch table has dynamic rendering loaded
    g_dynamic_rendering_available = vulkan_has_dynamic_rendering();
    
    if (!g_dynamic_rendering_available) {
        LOG_WARN("Dynamic Rendering not available - engine may not function correctly");
        return false;
    }
    
    // Check for TBDR optimization support (VK_KHR_dynamic_rendering_local_read)
    // This would require checking device features - for now, assume unavailable
    g_has_local_read = false;
    
    LOG_INFO("Dynamic Rendering initialized (VK %s)",
             g_vk_dispatch.has_dynamic_rendering_core ? "1.3 core" : "1.2 extension");
    
    return true;
}

void dynamic_rendering_begin(VkCommandBuffer cmd_buffer, const DynamicRenderingInfo* info) {
    if (!g_dynamic_rendering_available || !cmd_buffer || !info) {
        LOG_ERROR("Dynamic rendering not available or invalid parameters");
        return;
    }
    
    // Convert our info to VkRenderingInfo
    VkRenderingAttachmentInfo* color_attachments = NULL;
    if (info->color_attachment_count > 0) {
        color_attachments = (VkRenderingAttachmentInfo*)malloc(
            sizeof(VkRenderingAttachmentInfo) * info->color_attachment_count);
        
        for (u32 i = 0; i < info->color_attachment_count; i++) {
            color_attachments[i] = (VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = NULL,
                .imageView = info->color_attachments[i].image_view,
                .imageLayout = info->color_attachments[i].image_layout,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = info->color_attachments[i].load_op,
                .storeOp = info->color_attachments[i].store_op,
                .clearValue = info->color_attachments[i].clear_value
            };
        }
    }
    
    VkRenderingAttachmentInfo depth_attachment = {0};
    VkRenderingAttachmentInfo* depth_attachment_ptr = NULL;
    if (info->depth_attachment) {
        depth_attachment = (VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = NULL,
            .imageView = info->depth_attachment->image_view,
            .imageLayout = info->depth_attachment->image_layout,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = info->depth_attachment->load_op,
            .storeOp = info->depth_attachment->store_op,
            .clearValue = info->depth_attachment->clear_value
        };
        depth_attachment_ptr = &depth_attachment;
    }
    
    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea = info->render_area,
        .layerCount = info->layer_count > 0 ? info->layer_count : 1,
        .viewMask = info->view_mask,
        .colorAttachmentCount = info->color_attachment_count,
        .pColorAttachments = color_attachments,
        .pDepthAttachment = depth_attachment_ptr,
        .pStencilAttachment = info->stencil_attachment ? depth_attachment_ptr : NULL
    };
    
    // Call the appropriate function via dispatch table
    if (g_vk_dispatch.has_dynamic_rendering_core) {
        g_vk_dispatch.CmdBeginRendering(cmd_buffer, &rendering_info);
    } else if (g_vk_dispatch.has_dynamic_rendering_ext) {
        g_vk_dispatch.CmdBeginRenderingKHR(cmd_buffer, &rendering_info);
    }
    
    // Cleanup
    if (color_attachments) {
        free(color_attachments);
    }
}

void dynamic_rendering_end(VkCommandBuffer cmd_buffer) {
    if (!g_dynamic_rendering_available || !cmd_buffer) {
        return;
    }
    
    // Call the appropriate function via dispatch table
    if (g_vk_dispatch.has_dynamic_rendering_core) {
        g_vk_dispatch.CmdEndRendering(cmd_buffer);
    } else if (g_vk_dispatch.has_dynamic_rendering_ext) {
        g_vk_dispatch.CmdEndRenderingKHR(cmd_buffer);
    }
}

bool dynamic_rendering_is_available(void) {
    return g_dynamic_rendering_available;
}

bool dynamic_rendering_has_local_read(void) {
    return g_has_local_read;
}

// Helper: Simple color-only rendering
DynamicRenderingInfo dynamic_rendering_info_simple(
    VkImageView color_target,
    u32 width, u32 height,
    bool clear_color, float* clear_color_value
) {
    static DynamicRenderTarget color_attachment;
    color_attachment.image_view = color_target;
    color_attachment.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.load_op = clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    
    if (clear_color && clear_color_value) {
        color_attachment.clear_value.color.float32[0] = clear_color_value[0];
        color_attachment.clear_value.color.float32[1] = clear_color_value[1];
        color_attachment.clear_value.color.float32[2] = clear_color_value[2];
        color_attachment.clear_value.color.float32[3] = clear_color_value[3];
    }
    
    DynamicRenderingInfo info = {
        .color_attachments = &color_attachment,
        .color_attachment_count = 1,
        .depth_attachment = NULL,
        .stencil_attachment = NULL,
        .render_area = {{0, 0}, {width, height}},
        .layer_count = 1,
        .view_mask = 0,
        .use_local_read = false
    };
    
    return info;
}

// Helper: Color + depth rendering
DynamicRenderingInfo dynamic_rendering_info_with_depth(
    VkImageView color_target,
    VkImageView depth_target,
    u32 width, u32 height,
    bool clear_color, float* clear_color_value,
    bool clear_depth, float clear_depth_value
) {
    static DynamicRenderTarget color_attachment;
    static DynamicRenderTarget depth_attachment;
    
    color_attachment.image_view = color_target;
    color_attachment.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.load_op = clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    
    if (clear_color && clear_color_value) {
        color_attachment.clear_value.color.float32[0] = clear_color_value[0];
        color_attachment.clear_value.color.float32[1] = clear_color_value[1];
        color_attachment.clear_value.color.float32[2] = clear_color_value[2];
        color_attachment.clear_value.color.float32[3] = clear_color_value[3];
    }
    
    depth_attachment.image_view = depth_target;
    depth_attachment.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.load_op = clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.clear_value.depthStencil.depth = clear_depth_value;
    depth_attachment.clear_value.depthStencil.stencil = 0;
    
    DynamicRenderingInfo info = {
        .color_attachments = &color_attachment,
        .color_attachment_count = 1,
        .depth_attachment = &depth_attachment,
        .stencil_attachment = NULL,
        .render_area = {{0, 0}, {width, height}},
        .layer_count = 1,
        .view_mask = 0,
        .use_local_read = false
    };
    
    return info;
}

#endif // VULKAN_BUILD
