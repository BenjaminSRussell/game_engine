/**
 * =================================================================================================
 *                              FRAMEBUFFER IMPLEMENTATION
 * =================================================================================================
 */

#include "rendering/framebuffer.h"
#include <core/memory.h>
#include <core/logger.h>
#include <string.h>

/* =================================================================================================
 *                                    INTERNAL STRUCTURES
 * =================================================================================================
 */

struct Framebuffer {
    u32 width;
    u32 height;
    
    // Color attachments (MRT support)
    void *color_attachments[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS];
    u32 num_color_attachments;
    
    // Depth/stencil attachment
    void *depth_attachment;
    
    // Backend-specific handle (OpenGL FBO, Vulkan VkFramebuffer, etc.)
    void *backend_handle;
    
    // Validation state
    bool is_valid;
};

/* =================================================================================================
 *                                    IMPLEMENTATION
 * =================================================================================================
 */

Framebuffer* framebuffer_create(u32 width, u32 height) {
    Framebuffer *fb = (Framebuffer*)memory_allocate(sizeof(Framebuffer), MEMORY_TAG_RENDERER);
    
    fb->width = width;
    fb->height = height;
    fb->num_color_attachments = 0;
    fb->depth_attachment = NULL;
    fb->backend_handle = NULL;
    fb->is_valid = false;
    
    // Clear all attachment slots
    memset(fb->color_attachments, 0, sizeof(fb->color_attachments));
    
    LOG_INFO("Framebuffer created: %dx%d", width, height);
    
    return fb;
}

void framebuffer_destroy(Framebuffer *fb) {
    if (!fb) return;
    
    // TODO: Destroy backend-specific handle
    // This will be implemented when we integrate with OpenGL/Vulkan backend
    
    LOG_INFO("Framebuffer destroyed: %dx%d", fb->width, fb->height);
    
    memory_free(fb, sizeof(Framebuffer), MEMORY_TAG_RENDERER);
}

void framebuffer_attach_color(Framebuffer *fb, u32 slot, void *texture) {
    if (!fb) {
        LOG_ERROR("framebuffer_attach_color: NULL framebuffer");
        return;
    }
    
    if (slot >= FRAMEBUFFER_MAX_COLOR_ATTACHMENTS) {
        LOG_ERROR("framebuffer_attach_color: Invalid slot %u (max: %u)", 
                  slot, FRAMEBUFFER_MAX_COLOR_ATTACHMENTS - 1);
        return;
    }
    
    fb->color_attachments[slot] = texture;
    
    // Update attachment count
    if (slot >= fb->num_color_attachments) {
        fb->num_color_attachments = slot + 1;
    }
    
    // Mark as needing validation
    fb->is_valid = false;
    
    LOG_DEBUG("Framebuffer color attachment %u set", slot);
}

void framebuffer_attach_depth(Framebuffer *fb, void *texture) {
    if (!fb) {
        LOG_ERROR("framebuffer_attach_depth: NULL framebuffer");
        return;
    }
    
    fb->depth_attachment = texture;
    
    // Mark as needing validation
    fb->is_valid = false;
    
    LOG_DEBUG("Framebuffer depth attachment set");
}

bool framebuffer_validate(Framebuffer *fb) {
    if (!fb) return false;
    
    // Check that we have at least one color attachment or a depth attachment
    if (fb->num_color_attachments == 0 && fb->depth_attachment == NULL) {
        LOG_ERROR("Framebuffer validation failed: No attachments");
        fb->is_valid = false;
        return false;
    }
    
    // TODO: Additional validation when we integrate with backend:
    // - Check all textures have matching dimensions
    // - Check texture formats are valid for their attachment type
    // - Check backend-specific framebuffer completeness
    
    fb->is_valid = true;
    LOG_DEBUG("Framebuffer validated successfully (%u color attachments)", 
              fb->num_color_attachments);
    
    return true;
}

void framebuffer_bind(Framebuffer *fb) {
    if (!fb) {
        LOG_ERROR("framebuffer_bind: NULL framebuffer");
        return;
    }
    
    if (!fb->is_valid) {
        LOG_WARN("Binding unvalidated framebuffer - call framebuffer_validate() first");
    }
    
    // TODO: Bind backend-specific framebuffer
    // OpenGL: glBindFramebuffer(GL_FRAMEBUFFER, fb->backend_handle);
    // Vulkan: Set in render pass begin
    
    LOG_TRACE("Framebuffer bound");
}

void framebuffer_unbind(void) {
    // TODO: Unbind to default framebuffer
    // OpenGL: glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Vulkan: End render pass
    
    LOG_TRACE("Framebuffer unbound");
}

void framebuffer_clear_color(Framebuffer *fb, f32 r, f32 g, f32 b, f32 a) {
    if (!fb) return;
    
    // TODO: Clear color attachments
    // OpenGL: glClearColor + glClear(GL_COLOR_BUFFER_BIT)
    // Vulkan: Clear values in VkRenderPassBeginInfo
    
    LOG_TRACE("Framebuffer color cleared to (%.2f, %.2f, %.2f, %.2f)", r, g, b, a);
}

void framebuffer_clear_depth(Framebuffer *fb, f32 depth) {
    if (!fb) return;
    
    // TODO: Clear depth attachment
    // OpenGL: glClearDepth + glClear(GL_DEPTH_BUFFER_BIT)
    // Vulkan: Clear values in VkRenderPassBeginInfo
    
    LOG_TRACE("Framebuffer depth cleared to %.2f", depth);
}

u32 framebuffer_get_width(Framebuffer *fb) {
    return fb ? fb->width : 0;
}

u32 framebuffer_get_height(Framebuffer *fb) {
    return fb ? fb->height : 0;
}
