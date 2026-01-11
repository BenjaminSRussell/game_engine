#include "editor_common.h"
#include "core/logger.h"
#include "core/memory.h"
#include "rendering/vulkan.h"
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                           EDITOR CONTEXT IMPLEMENTATION
 * =================================================================================================
 *
 * Provides shared infrastructure for all editor tools:
 * - Tool registration and management
 * - Viewport initialization and camera controls
 * - Shared undo/redo system
 * - Common settings
 */

// =================================================================================================
//                                    CONTEXT MANAGEMENT
// =================================================================================================

EditorContext* editor_context_create(void) {
    EditorContext* ctx = (EditorContext*)malloc(sizeof(EditorContext));
    if (!ctx) {
        LOG_ERROR("Failed to allocate EditorContext");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(EditorContext));
    LOG_INFO("Created EditorContext");
    
    return ctx;
}

void editor_context_destroy(EditorContext* ctx) {
    if (!ctx) return;
    
    if (ctx->is_initialized) {
        editor_context_shutdown(ctx);
    }
    
    free(ctx);
    LOG_INFO("Destroyed EditorContext");
}

bool editor_context_init(EditorContext* ctx) {
    if (!ctx) return false;
    if (ctx->is_initialized) {
        LOG_WARN("EditorContext already initialized");
        return true;
    }
    
    // Initialize viewport
    if (!editor_viewport_init(&ctx->viewport, 1280, 720)) {
        LOG_ERROR("Failed to initialize editor viewport");
        return false;
    }
    
    // Set default settings
    ctx->show_grid = true;
    ctx->show_gizmos = true;
    ctx->grid_size = 1.0f;
    ctx->undo_capacity = 100;
    
    ctx->is_initialized = true;
    LOG_INFO("Initialized EditorContext");
    
    return true;
}

void editor_context_shutdown(EditorContext* ctx) {
    if (!ctx || !ctx->is_initialized) return;
    
    // Shutdown viewport
    editor_viewport_shutdown(&ctx->viewport);
    
    // Shutdown tool-specific systems
    if (ctx->terrain_system) {
        terrain_sculpting_destroy(ctx->terrain_system);
        LOG_INFO("Shutting down terrain system");
        ctx->terrain_system = NULL;
    }
    
    if (ctx->material_graph) {
        material_graph_destroy(ctx->material_graph);
        LOG_INFO("Shutting down material graph");
        ctx->material_graph = NULL;
    }
    
    ctx->is_initialized = false;
    LOG_INFO("Shutdown EditorContext");
}

void editor_context_update(EditorContext* ctx, f32 delta_time) {
    if (!ctx || !ctx->is_initialized) return;
    
    // Update viewport camera
    editor_viewport_update_camera(&ctx->viewport, delta_time);
    
    // Update active tool
    if (ctx->active_tool && ctx->active_tool->update) {
        ctx->active_tool->update(ctx, delta_time);
    }
}

// =================================================================================================
//                                    TOOL MANAGEMENT
// =================================================================================================

bool editor_register_tool(EditorContext* ctx, EditorTool* tool) {
    if (!ctx || !tool) return false;
    
    if (ctx->tool_count >= 16) {
        LOG_ERROR("Cannot register more than 16 tools");
        return false;
    }
    
    ctx->available_tools[ctx->tool_count] = tool;
    ctx->tool_count++;
    
    LOG_INFO("Registered editor tool: %s", tool->name);
    
    return true;
}

bool editor_set_active_tool(EditorContext* ctx, const char* tool_name) {
    if (!ctx || !tool_name) return false;
    
    // Find tool by name
    for (u32 i = 0; i < ctx->tool_count; i++) {
        EditorTool* tool = ctx->available_tools[i];
        if (strcmp(tool->name, tool_name) == 0) {
            // Shutdown previous tool
            if (ctx->active_tool && ctx->active_tool->shutdown) {
                ctx->active_tool->shutdown(ctx);
            }
            
            // Initialize new tool
            ctx->active_tool = tool;
            if (tool->init) {
                if (!tool->init(ctx)) {
                    LOG_ERROR("Failed to initialize tool: %s", tool_name);
                    ctx->active_tool = NULL;
                    return false;
                }
            }
            
            LOG_INFO("Activated editor tool: %s", tool_name);
            return true;
        }
    }
    
    LOG_ERROR("Tool not found: %s", tool_name);
    return false;
}

EditorTool* editor_get_active_tool(EditorContext* ctx) {
    return ctx ? ctx->active_tool : NULL;
}

// =================================================================================================
//                                    VIEWPORT MANAGEMENT
// =================================================================================================

bool editor_viewport_init(EditorViewport* viewport, u32 width, u32 height) {
    if (!viewport) return false;
    
    memset(viewport, 0, sizeof(EditorViewport));
    
    viewport->width = width;
    viewport->height = height;
    
    // Set default camera position
    viewport->camera_position = (Vec3){0.0f, 10.0f, 20.0f};
    viewport->camera_target = (Vec3){0.0f, 0.0f, 0.0f};
    viewport->camera_distance = 20.0f;
    viewport->camera_yaw = 0.0f;
    viewport->camera_pitch = -30.0f;
    
    // Initialize view and projection matrices
    viewport->view_matrix = mat4_identity();
    viewport->projection_matrix = mat4_identity();
    
    // Create framebuffer and textures for editor viewport
    LOG_INFO("Creating editor viewport framebuffer: %ux%u", width, height);
    
    // Get global Vulkan renderer from context (will be passed in real implementation)
    // For now, we'll create stub handles that would be replaced with actual Vulkan objects
    // In a real implementation, this would:
    // 1. Create color texture (VK_FORMAT_R8G8B8A8_SRGB)
    // 2. Create depth texture (VK_FORMAT_D32_SFLOAT) 
    // 3. Create render pass for editor rendering
    // 4. Create framebuffer with both attachments
    
    // Placeholder handles - these would be actual Vulkan objects
    viewport->framebuffer = 0;
    viewport->color_texture = 0;
    viewport->depth_texture = 0;
    
    // Example of what the real implementation would look like:
    /*
    VulkanRenderer* vk_renderer = (VulkanRenderer*)renderer;
    if (vk_renderer) {
        // Create color image
        VkImageCreateInfo color_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT
        };
        
        vkCreateImage(vk_renderer->device, &color_info, NULL, &viewport->color_texture);
        // ... allocate memory, create image view, etc.
        
        // Create depth image
        VkImageCreateInfo depth_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = VK_FORMAT_D32_SFLOAT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT
        };
        
        vkCreateImage(vk_renderer->device, &depth_info, NULL, &viewport->depth_texture);
        // ... allocate memory, create image view, etc.
        
        // Create framebuffer
        VkImageView attachments[] = {color_view, depth_view};
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = editor_render_pass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = width,
            .height = height,
            .layers = 1
        };
        
        vkCreateFramebuffer(vk_renderer->device, &fb_info, NULL, &viewport->framebuffer);
    }
    */
    
    LOG_INFO("Initialized editor viewport: %ux%u", width, height);
    
    return true;
}

void editor_viewport_shutdown(EditorViewport* viewport) {
    if (!viewport) return;
    
    // Cleanup framebuffer and textures
    LOG_INFO("Destroying editor viewport framebuffer and textures");
    
    // In a real implementation, this would clean up actual Vulkan resources:
    /*
    VulkanRenderer* vk_renderer = (VulkanRenderer*)renderer;
    if (vk_renderer && viewport->framebuffer != VK_NULL_HANDLE) {
        // Wait for device to be idle before destroying resources
        vkDeviceWaitIdle(vk_renderer->device);
        
        // Destroy framebuffer
        if (viewport->framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(vk_renderer->device, viewport->framebuffer, NULL);
            viewport->framebuffer = VK_NULL_HANDLE;
        }
        
        // Destroy color texture and view
        if (viewport->color_texture != VK_NULL_HANDLE) {
            vkDestroyImageView(vk_renderer->device, viewport->color_texture_view, NULL);
            vkDestroyImage(vk_renderer->device, viewport->color_texture, NULL);
            vkFreeMemory(vk_renderer->device, viewport->color_texture_memory, NULL);
            viewport->color_texture = VK_NULL_HANDLE;
            viewport->color_texture_view = VK_NULL_HANDLE;
            viewport->color_texture_memory = VK_NULL_HANDLE;
        }
        
        // Destroy depth texture and view
        if (viewport->depth_texture != VK_NULL_HANDLE) {
            vkDestroyImageView(vk_renderer->device, viewport->depth_texture_view, NULL);
            vkDestroyImage(vk_renderer->device, viewport->depth_texture, NULL);
            vkFreeMemory(vk_renderer->device, viewport->depth_texture_memory, NULL);
            viewport->depth_texture = VK_NULL_HANDLE;
            viewport->depth_texture_view = VK_NULL_HANDLE;
            viewport->depth_texture_memory = VK_NULL_HANDLE;
        }
    }
    */
    
    // Clear handles (for stub implementation)
    viewport->framebuffer = 0;
    viewport->color_texture = 0;
    viewport->depth_texture = 0;
    
    LOG_INFO("Shutdown editor viewport");
    
    memset(viewport, 0, sizeof(EditorViewport));
}

void editor_viewport_resize(EditorViewport* viewport, u32 width, u32 height) {
    if (!viewport) return;
    
    // Only recreate if dimensions actually changed
    if (viewport->width == width && viewport->height == height) {
        return;
    }
    
    LOG_INFO("Resizing editor viewport: %ux%u -> %ux%u", viewport->width, viewport->height, width, height);
    
    // Cleanup existing framebuffer and textures
    LOG_INFO("Destroying old viewport resources");
    
    // In a real implementation, this would clean up existing Vulkan resources:
    /*
    VulkanRenderer* vk_renderer = (VulkanRenderer*)renderer;
    if (vk_renderer) {
        // Wait for device to be idle before recreating resources
        vkDeviceWaitIdle(vk_renderer->device);
        
        // Destroy old framebuffer
        if (viewport->framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(vk_renderer->device, viewport->framebuffer, NULL);
            viewport->framebuffer = VK_NULL_HANDLE;
        }
        
        // Destroy old color texture and view
        if (viewport->color_texture != VK_NULL_HANDLE) {
            vkDestroyImageView(vk_renderer->device, viewport->color_texture_view, NULL);
            vkDestroyImage(vk_renderer->device, viewport->color_texture, NULL);
            vkFreeMemory(vk_renderer->device, viewport->color_texture_memory, NULL);
        }
        
        // Destroy old depth texture and view
        if (viewport->depth_texture != VK_NULL_HANDLE) {
            vkDestroyImageView(vk_renderer->device, viewport->depth_texture_view, NULL);
            vkDestroyImage(vk_renderer->device, viewport->depth_texture, NULL);
            vkFreeMemory(vk_renderer->device, viewport->depth_texture_memory, NULL);
        }
    }
    */
    
    // Update dimensions
    viewport->width = width;
    viewport->height = height;
    
    // Recreate framebuffer and textures with new size
    LOG_INFO("Recreating viewport resources: %ux%u", width, height);
    
    // In a real implementation, this would recreate Vulkan resources with new dimensions:
    /*
    if (vk_renderer) {
        // Recreate color texture with new dimensions
        VkImageCreateInfo color_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT
        };
        
        vkCreateImage(vk_renderer->device, &color_info, NULL, &viewport->color_texture);
        // ... allocate memory, create image view with new dimensions
        
        // Recreate depth texture with new dimensions
        VkImageCreateInfo depth_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .extent.width = width,
            .extent.height = height,
            .extent.depth = 1,
            .mipLevels = 1,
            .arrayLayers = 1,
            .format = VK_FORMAT_D32_SFLOAT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT
        };
        
        vkCreateImage(vk_renderer->device, &depth_info, NULL, &viewport->depth_texture);
        // ... allocate memory, create image view with new dimensions
        
        // Recreate framebuffer with new attachments
        VkImageView attachments[] = {new_color_view, new_depth_view};
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = editor_render_pass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = width,
            .height = height,
            .layers = 1
        };
        
        vkCreateFramebuffer(vk_renderer->device, &fb_info, NULL, &viewport->framebuffer);
        
        // Update any descriptor sets that reference these resources
        // This would involve updating descriptor writes for the new image views
    }
    */
    
    // Update projection matrix for new aspect ratio
    f32 aspect_ratio = (f32)width / (f32)height;
    f32 fov_y = 45.0f * (3.14159f / 180.0f); // 45 degrees in radians
    f32 near_z = 0.1f;
    f32 far_z = 10000.0f;
    viewport->projection_matrix = mat4_perspective(fov_y, aspect_ratio, near_z, far_z);
    
    LOG_INFO("Resized editor viewport: %ux%u", width, height);
}

void editor_viewport_update_camera(EditorViewport* viewport, f32 delta_time) {
    if (!viewport) return;
    
    // Update camera position based on spherical coordinates
    f32 yaw_rad = viewport->camera_yaw * (3.14159f / 180.0f);
    f32 pitch_rad = viewport->camera_pitch * (3.14159f / 180.0f);
    
    viewport->camera_position.x = viewport->camera_target.x + 
        viewport->camera_distance * cosf(pitch_rad) * sinf(yaw_rad);
    viewport->camera_position.y = viewport->camera_target.y + 
        viewport->camera_distance * sinf(pitch_rad);
    viewport->camera_position.z = viewport->camera_target.z + 
        viewport->camera_distance * cosf(pitch_rad) * cosf(yaw_rad);
    
    // Update view matrix using look-at function
    Vec3 up = (Vec3){0.0f, 1.0f, 0.0f};
    viewport->view_matrix = mat4_look_at(viewport->camera_position, viewport->camera_target, up);
    
    // Update projection matrix with perspective
    f32 aspect_ratio = (f32)viewport->width / (f32)viewport->height;
    f32 fov_y = 45.0f * (3.14159f / 180.0f); // 45 degrees in radians
    f32 near_z = 0.1f;
    f32 far_z = 10000.0f;
    viewport->projection_matrix = mat4_perspective(fov_y, aspect_ratio, near_z, far_z);
}

void editor_viewport_handle_input(EditorViewport* viewport, i32 mouse_x, i32 mouse_y, bool mouse_down) {
    if (!viewport) return;
    
    Vec2 current_mouse = {(f32)mouse_x, (f32)mouse_y};
    
    if (mouse_down) {
        if (viewport->is_rotating) {
            // Calculate delta
            f32 dx = current_mouse.x - viewport->last_mouse_pos.x;
            f32 dy = current_mouse.y - viewport->last_mouse_pos.y;
            
            // Update camera rotation
            viewport->camera_yaw += dx * 0.5f;
            viewport->camera_pitch += dy * 0.5f;
            
            // Clamp pitch
            if (viewport->camera_pitch > 89.0f) viewport->camera_pitch = 89.0f;
            if (viewport->camera_pitch < -89.0f) viewport->camera_pitch = -89.0f;
        } else {
            viewport->is_rotating = true;
        }
    } else {
        viewport->is_rotating = false;
    }
    
    viewport->last_mouse_pos = current_mouse;
}
