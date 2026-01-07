#include "editor_common.h"
#include "core/logger.h"
#include "core/memory.h"
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
        // This would call terrain_sculpting_destroy() when implemented
        LOG_INFO("Shutting down terrain system");
        ctx->terrain_system = NULL;
    }
    
    if (ctx->material_graph) {
        // This would call material_graph_destroy() when implemented
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
    
    // TODO: Create framebuffer and textures
    // This would require OpenGL/Vulkan calls which aren't available yet
    LOG_INFO("Initialized editor viewport: %ux%u", width, height);
    
    return true;
}

void editor_viewport_shutdown(EditorViewport* viewport) {
    if (!viewport) return;
    
    // TODO: Delete framebuffer and textures
    LOG_INFO("Shutdown editor viewport");
    
    memset(viewport, 0, sizeof(EditorViewport));
}

void editor_viewport_resize(EditorViewport* viewport, u32 width, u32 height) {
    if (!viewport) return;
    
    viewport->width = width;
    viewport->height = height;
    
    // TODO: Recreate framebuffer and textures with new size
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
    
    // TODO: Update view and projection matrices
    // This would use the math library to create proper matrices
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
