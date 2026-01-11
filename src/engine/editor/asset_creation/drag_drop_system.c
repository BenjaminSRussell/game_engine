// Drag and Drop System implementation
#include "editor/asset_creation/drag_drop_system.h"
#include <core/logger.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global drag and drop context
static DragDropContext g_context = {0};
static bool g_initialized = false;

// MARK: - Drag and Drop System Management

bool drag_drop_init(void) {
    if (g_initialized) {
        return true;
    }
    
    memset(&g_context, 0, sizeof(DragDropContext));
    g_context.drag_threshold = 5.0f; // 5 pixel threshold
    g_context.show_ghost = true;
    
    g_initialized = true;
    LOG_INFO("Drag and drop system initialized");
    return true;
}

void drag_drop_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    // Clean up current drag data
    if (g_context.current_drag.data) {
        drag_drop_free_data(&g_context.current_drag);
    }
    
    // Clean up ghost entity
    if (g_context.ghost_entity.id != 0) {
        // TODO: Destroy ghost entity
        g_context.ghost_entity.id = 0;
    }
    
    drag_drop_clear_targets();
    
    g_initialized = false;
    LOG_INFO("Drag and drop system shutdown");
}

void drag_drop_update(f32 delta_time) {
    if (!g_initialized) {
        return;
    }
    
    // Update drag state
    if (g_context.is_dragging) {
        // Update ghost position if dragging
        if (g_context.show_ghost && g_context.ghost_entity.id != 0) {
            // TODO: Update ghost entity position based on current mouse position
        }
        
        // Check for potential targets
        drag_drop_find_potential_targets();
    }
}

void drag_drop_render(void) {
    if (!g_initialized) {
        return;
    }
    
    // Render ghost entity if dragging
    if (g_context.is_dragging && g_context.show_ghost && g_context.ghost_entity.id != 0) {
        // Render ghost entity with transparency
        entity_set_visibility(g_context.ghost_entity.id, true);
        
        // Set ghost material properties for transparency
        Material ghost_material = {0};
        ghost_material.albedo = (Vec4){0.8f, 0.8f, 1.0f, 0.5f}; // Semi-transparent blue-white
        ghost_material.metallic = 0.1f;
        ghost_material.roughness = 0.8f;
        ghost_material.emissive = (Vec3){0.1f, 0.1f, 0.2f}; // Slight glow
        
        // Apply ghost material override
        entity_set_material_override(g_context.ghost_entity.id, &ghost_material);
        
        // Enable depth testing but disable depth writing for proper transparency
        entity_set_depth_write_enabled(g_context.ghost_entity.id, false);
        
        // Add subtle pulsing effect
        f32 pulse = sinf(g_context.drag_time * 3.0f) * 0.1f + 0.9f;
        ghost_material.albedo.w = pulse * 0.5f; // Pulsing transparency
        entity_set_material_override(g_context.ghost_entity.id, &ghost_material);
        
        // Render outline for better visibility
        Vec3 outline_color = (Vec3){0.2f, 0.6f, 1.0f};
        f32 outline_width = 2.0f;
        entity_set_outline(g_context.ghost_entity.id, true, outline_color, outline_width);
        
        LOG_TRACE("Rendering ghost entity at position (%.2f, %.2f, %.2f)", 
                 g_context.ghost_position.x, g_context.ghost_position.y, g_context.ghost_position.z);
    } else if (g_context.ghost_entity.id != 0) {
        // Hide ghost entity when not dragging
        entity_set_visibility(g_context.ghost_entity.id, false);
        entity_set_outline(g_context.ghost_entity.id, false, (Vec3){0,0,0}, 0.0f);
    }
    
    // Render drop target highlights
    if (g_context.current_target) {
        // Highlight current drop target
        DropTarget* target = g_context.current_target;
        
        // Create highlight effect based on target type
        if (target->entity_id != 0) {
            // Highlight entity with glowing effect
            Material highlight_material = {0};
            highlight_material.albedo = (Vec4){0.2f, 1.0f, 0.2f, 0.3f}; // Green with transparency
            highlight_material.emissive = (Vec3){0.1f, 0.5f, 0.1f}; // Green glow
            highlight_material.metallic = 0.0f;
            highlight_material.roughness = 0.9f;
            
            entity_set_material_override(target->entity_id, &highlight_material);
            entity_set_outline(target->entity_id, true, (Vec3){0.0f, 1.0f, 0.0f}, 3.0f);
            
            // Add pulsing effect
            f32 pulse = sinf(g_context.drag_time * 4.0f) * 0.2f + 0.8f;
            highlight_material.albedo.w = pulse * 0.3f;
            entity_set_material_override(target->entity_id, &highlight_material);
        }
        
        // Render target bounds visualization
        if (target->bounds_valid) {
            // Draw bounding box around target area
            Vec3 min = target->bounds_min;
            Vec3 max = target->bounds_max;
            
            // Create 8 corners of the bounding box
            Vec3 corners[8] = {
                {min.x, min.y, min.z}, {max.x, min.y, min.z},
                {min.x, max.y, min.z}, {max.x, max.y, min.z},
                {min.x, min.y, max.z}, {max.x, min.y, max.z},
                {min.x, max.y, max.z}, {max.x, max.y, max.z}
            };
            
            // Draw edges of the bounding box
            Vec3 line_color = (Vec3){0.0f, 1.0f, 0.0f}; // Green
            f32 line_width = 2.0f;
            
            // Bottom face
            debug_renderer_draw_line(corners[0], corners[1], line_color, line_width);
            debug_renderer_draw_line(corners[1], corners[3], line_color, line_width);
            debug_renderer_draw_line(corners[3], corners[2], line_color, line_width);
            debug_renderer_draw_line(corners[2], corners[0], line_color, line_width);
            
            // Top face
            debug_renderer_draw_line(corners[4], corners[5], line_color, line_width);
            debug_renderer_draw_line(corners[5], corners[7], line_color, line_width);
            debug_renderer_draw_line(corners[7], corners[6], line_color, line_width);
            debug_renderer_draw_line(corners[6], corners[4], line_color, line_width);
            
            // Vertical edges
            debug_renderer_draw_line(corners[0], corners[4], line_color, line_width);
            debug_renderer_draw_line(corners[1], corners[5], line_color, line_width);
            debug_renderer_draw_line(corners[2], corners[6], line_color, line_width);
            debug_renderer_draw_line(corners[3], corners[7], line_color, line_width);
        }
        
        // Render target icon or indicator
        if (target->icon_texture_id != 0) {
            // Render drop target icon at target position
            Vec3 icon_pos = target->world_position;
            icon_pos.y += target->bounds_max.y + 0.5f; // Position above target
            
            // Create temporary icon entity
            u32 icon_entity = entity_create();
            if (icon_entity != 0) {
                entity_add_mesh_component(icon_entity, MESH_PLANE_ID);
                entity_set_material(icon_entity, target->icon_texture_id);
                entity_set_position(icon_entity, icon_pos);
                entity_set_scale(icon_entity, (Vec3){0.5f, 0.5f, 0.5f});
                entity_set_billboard(icon_entity, true); // Always face camera
                
                // Make icon semi-transparent and glowing
                Material icon_material = {0};
                icon_material.albedo = (Vec4){1.0f, 1.0f, 1.0f, 0.7f};
                icon_material.emissive = (Vec3){0.3f, 0.8f, 0.3f};
                entity_set_material_override(icon_entity, &icon_material);
                
                // Render and immediately destroy (temporary)
                entity_render(icon_entity);
                entity_destroy(icon_entity);
            }
        }
        
        LOG_TRACE("Highlighting drop target at position (%.2f, %.2f, %.2f)", 
                 target->world_position.x, target->world_position.y, target->world_position.z);
    }
    
    // Render potential target highlights (dimmed)
    for (u32 i = 0; i < g_context.target_count; i++) {
        DropTarget* potential_target = &g_context.potential_targets[i];
        if (potential_target == g_context.current_target) continue; // Skip current target
        
        if (potential_target->entity_id != 0) {
            // Dimmed highlight for potential targets
            Material dim_material = {0};
            dim_material.albedo = (Vec4){0.5f, 0.5f, 1.0f, 0.1f}; // Very dim blue
            dim_material.emissive = (Vec3){0.05f, 0.05f, 0.2f};
            
            entity_set_material_override(potential_target->entity_id, &dim_material);
            entity_set_outline(potential_target->entity_id, true, (Vec3){0.3f, 0.3f, 1.0f}, 1.0f);
        }
    }
}

DragDropContext* drag_drop_get_context(void) {
    return &g_context;
}

// MARK: - Drag Operations

bool drag_drop_start_drag(DragData* data, Vec2 position) {
    if (!g_initialized || !data) {
        return false;
    }
    
    // Clean up any existing drag data
    if (g_context.current_drag.data) {
        drag_drop_free_data(&g_context.current_drag);
    }
    
    // Copy drag data
    g_context.current_drag = *data;
    if (data->data && data->data_size > 0) {
        g_context.current_drag.data = malloc(data->data_size);
        if (g_context.current_drag.data) {
            memcpy(g_context.current_drag.data, data->data, data->data_size);
        }
    }
    
    // Set drag state
    g_context.state = DRAG_STATE_DRAGGING;
    g_context.start_position = position;
    g_context.current_position = position;
    g_context.delta_position = (Vec2){0, 0};
    g_context.is_dragging = true;
    g_context.drag_start_time = (f32)clock() / CLOCKS_PER_SEC;
    
    // Create ghost entity
    drag_drop_create_ghost_entity();
    
    LOG_INFO("Started dragging: %s", data->description);
    return true;
}

void drag_drop_update_drag(Vec2 position) {
    if (!g_initialized || !g_context.is_dragging) {
        return;
    }
    
    // Update position and delta
    g_context.delta_position.x = position.x - g_context.current_position.x;
    g_context.delta_position.y = position.y - g_context.current_position.y;
    g_context.current_position = position;
    
    // Check if we've moved beyond threshold
    f32 distance = sqrtf(g_context.delta_position.x * g_context.delta_position.x + 
                        g_context.delta_position.y * g_context.delta_position.y);
    
    if (distance >= g_context.drag_threshold) {
        g_context.state = DRAG_STATE_DRAGGING;
    }
    
    // Update ghost position
    if (g_context.show_ghost) {
        // TODO: Convert screen position to world position for ghost
        g_context.ghost_position = (Vec3){position.x, 0, position.y};
    }
}

bool drag_drop_end_drag(Vec2 position) {
    if (!g_initialized || !g_context.is_dragging) {
        return false;
    }
    
    g_context.current_position = position;
    g_context.state = DRAG_STATE_DROPPING;
    
    bool success = false;
    
    // Check if we have a valid drop target
    if (g_context.current_target && g_context.current_target->can_accept) {
        if (g_context.current_target->can_accept(g_context.current_target->user_data, &g_context.current_drag)) {
            // Perform drop
            if (g_context.current_target->on_drop) {
                g_context.current_target->on_drop(g_context.current_target->user_data, 
                                                 &g_context.current_drag, position);
            }
            success = true;
            LOG_INFO("Successfully dropped: %s", g_context.current_drag.description);
        }
    }
    
    // Clean up
    drag_drop_cancel_drag();
    
    return success;
}

void drag_drop_cancel_drag(void) {
    if (!g_initialized) {
        return;
    }
    
    // Clean up drag data
    if (g_context.current_drag.data) {
        drag_drop_free_data(&g_context.current_drag);
    }
    
    // Clean up ghost entity
    if (g_context.ghost_entity.id != 0) {
        // TODO: Destroy ghost entity
        g_context.ghost_entity.id = 0;
    }
    
    // Notify current target that we're leaving
    if (g_context.current_target && g_context.current_target->on_leave) {
        g_context.current_target->on_leave(g_context.current_target->user_data);
    }
    
    // Reset state
    memset(&g_context.current_drag, 0, sizeof(DragData));
    g_context.state = DRAG_STATE_IDLE;
    g_context.is_dragging = false;
    g_context.current_target = NULL;
    g_context.target_count = 0;
    
    LOG_INFO("Cancelled drag operation");
}

// MARK: - Drop Target Management

void drag_drop_register_target(DropTarget* target) {
    if (!g_initialized || !target || g_context.target_count >= 16) {
        return;
    }
    
    // Check if target is already registered
    for (u32 i = 0; i < g_context.target_count; i++) {
        if (g_context.potential_targets[i] == target) {
            return;
        }
    }
    
    // Add new target
    g_context.potential_targets[g_context.target_count++] = target;
    LOG_DEBUG("Registered drop target");
}

void drag_drop_unregister_target(DropTarget* target) {
    if (!g_initialized || !target) {
        return;
    }
    
    // Find and remove target
    for (u32 i = 0; i < g_context.target_count; i++) {
        if (g_context.potential_targets[i] == target) {
            // Shift remaining targets
            for (u32 j = i; j < g_context.target_count - 1; j++) {
                g_context.potential_targets[j] = g_context.potential_targets[j + 1];
            }
            g_context.target_count--;
            
            if (g_context.current_target == target) {
                g_context.current_target = NULL;
            }
            
            LOG_DEBUG("Unregistered drop target");
            return;
        }
    }
}

void drag_drop_clear_targets(void) {
    if (!g_initialized) {
        return;
    }
    
    g_context.target_count = 0;
    g_context.current_target = NULL;
    memset(g_context.potential_targets, 0, sizeof(g_context.potential_targets));
}

// MARK: - Data Creation Helpers

DragData* drag_drop_create_asset_data(void* asset, const char* asset_name) {
    DragData* data = malloc(sizeof(DragData));
    if (!data) {
        return NULL;
    }
    
    memset(data, 0, sizeof(DragData));
    data->type = DRAG_DATA_TYPE_ASSET;
    data->data = asset;
    data->data_size = sizeof(void*);
    
    if (asset_name) {
        strncpy(data->description, asset_name, sizeof(data->description) - 1);
    }
    
    return data;
}

DragData* drag_drop_create_entity_data(Entity entity, const char* entity_name) {
    DragData* data = malloc(sizeof(DragData));
    if (!data) {
        return NULL;
    }
    
    memset(data, 0, sizeof(DragData));
    data->type = DRAG_DATA_TYPE_ENTITY;
    data->data = malloc(sizeof(Entity));
    if (data->data) {
        *(Entity*)data->data = entity;
        data->data_size = sizeof(Entity);
    }
    
    if (entity_name) {
        strncpy(data->description, entity_name, sizeof(data->description) - 1);
    }
    
    return data;
}

DragData* drag_drop_create_file_data(const char* file_path, const char* file_name) {
    DragData* data = malloc(sizeof(DragData));
    if (!data) {
        return NULL;
    }
    
    memset(data, 0, sizeof(DragData));
    data->type = DRAG_DATA_TYPE_FILE;
    
    if (file_path) {
        size_t path_len = strlen(file_path) + 1;
        data->data = malloc(path_len);
        if (data->data) {
            strcpy(data->data, file_path);
            data->data_size = path_len;
        }
    }
    
    if (file_name) {
        strncpy(data->description, file_name, sizeof(data->description) - 1);
    }
    
    return data;
}

DragData* drag_drop_create_text_data(const char* text, const char* description) {
    DragData* data = malloc(sizeof(DragData));
    if (!data) {
        return NULL;
    }
    
    memset(data, 0, sizeof(DragData));
    data->type = DRAG_DATA_TYPE_TEXT;
    
    if (text) {
        size_t text_len = strlen(text) + 1;
        data->data = malloc(text_len);
        if (data->data) {
            strcpy(data->data, text);
            data->data_size = text_len;
        }
    }
    
    if (description) {
        strncpy(data->description, description, sizeof(data->description) - 1);
    }
    
    return data;
}

DragData* drag_drop_create_custom_data(void* data, size_t size, const char* description) {
    DragData* drag_data = malloc(sizeof(DragData));
    if (!drag_data) {
        return NULL;
    }
    
    memset(drag_data, 0, sizeof(DragData));
    drag_data->type = DRAG_DATA_TYPE_CUSTOM;
    
    if (data && size > 0) {
        drag_data->data = malloc(size);
        if (drag_data->data) {
            memcpy(drag_data->data, data, size);
            drag_data->data_size = size;
        }
    }
    
    if (description) {
        strncpy(drag_data->description, description, sizeof(drag_data->description) - 1);
    }
    
    return drag_data;
}

void drag_drop_free_data(DragData* data) {
    if (!data) {
        return;
    }
    
    if (data->data) {
        free(data->data);
        data->data = NULL;
    }
    
    free(data);
}

// MARK: - Private Helper Functions

static void drag_drop_find_potential_targets(void) {
    if (!g_initialized || !g_context.is_dragging) {
        return;
    }
    
    // Reset current target
    DropTarget* previous_target = g_context.current_target;
    g_context.current_target = NULL;
    
    // Check each registered target
    for (u32 i = 0; i < g_context.target_count; i++) {
        DropTarget* target = g_context.potential_targets[i];
        
        if (target && target->can_accept) {
            if (target->can_accept(target->user_data, &g_context.current_drag)) {
                g_context.current_target = target;
                
                // Notify new target that we're hovering
                if (target->on_hover && target != previous_target) {
                    target->on_hover(target->user_data, &g_context.current_drag, g_context.current_position);
                }
                
                break; // Use first valid target
            }
        }
    }
    
    // Notify previous target that we left
    if (previous_target && previous_target != g_context.current_target && previous_target->on_leave) {
        previous_target->on_leave(previous_target->user_data);
    }
}

static void drag_drop_create_ghost_entity(void) {
    if (!g_initialized || !g_context.current_drag.data) {
        return;
    }
    
    // Create ghost entity based on drag data type
    switch (g_context.current_drag.type) {
        case DRAG_DATA_TYPE_ASSET:
            // Create ghost entity with asset mesh
            {
                AssetDragData* asset_data = (AssetDragData*)g_context.current_drag.data;
                if (asset_data && asset_data->asset_id != 0) {
                    // Create ghost entity with the asset's mesh
                    g_context.ghost_entity.id = entity_create();
                    if (g_context.ghost_entity.id != 0) {
                        // Load asset mesh and assign to ghost entity
                        entity_add_mesh_component(g_context.ghost_entity.id, asset_data->asset_id);
                        entity_set_material(g_context.ghost_entity.id, 0xFFFFFFFF); // Ghost material
                        entity_set_visibility(g_context.ghost_entity.id, false); // Initially hidden
                        entity_set_collision_enabled(g_context.ghost_entity.id, false); // No collision
                        LOG_DEBUG("Created ghost entity for asset %u", asset_data->asset_id);
                    }
                }
            }
            break;
            
        case DRAG_DATA_TYPE_ENTITY:
            // Create ghost entity copy
            {
                EntityDragData* entity_data = (EntityDragData*)g_context.current_drag.data;
                if (entity_data && entity_data->entity_id != 0) {
                    // Create copy of the entity
                    g_context.ghost_entity.id = entity_clone(entity_data->entity_id);
                    if (g_context.ghost_entity.id != 0) {
                        entity_set_visibility(g_context.ghost_entity.id, false); // Initially hidden
                        entity_set_collision_enabled(g_context.ghost_entity.id, false); // No collision
                        entity_set_material(g_context.ghost_entity.id, 0xFFFFFFFF); // Ghost material
                        LOG_DEBUG("Created ghost entity copy of entity %u", entity_data->entity_id);
                    }
                }
            }
            break;
            
        case DRAG_DATA_TYPE_FILE:
            // Create file icon ghost
            {
                FileDragData* file_data = (FileDragData*)g_context.current_drag.data;
                if (file_data && file_data->file_path[0] != '\0') {
                    // Create simple ghost entity with file icon mesh
                    g_context.ghost_entity.id = entity_create();
                    if (g_context.ghost_entity.id != 0) {
                        // Use a simple cube as file icon placeholder
                        entity_add_mesh_component(g_context.ghost_entity.id, MESH_CUBE_ID);
                        entity_set_material(g_context.ghost_entity.id, 0xFFFFFFFF); // Ghost material
                        entity_set_scale(g_context.ghost_entity.id, (Vec3){0.5f, 0.5f, 0.1f}); // File-like proportions
                        entity_set_visibility(g_context.ghost_entity.id, false); // Initially hidden
                        entity_set_collision_enabled(g_context.ghost_entity.id, false); // No collision
                        LOG_DEBUG("Created ghost entity for file '%s'", file_data->file_path);
                    }
                }
            }
            break;
            
        case DRAG_DATA_TYPE_TEXT:
            // Create text ghost
            {
                TextDragData* text_data = (TextDragData*)g_context.current_drag.data;
                if (text_data && text_data->text[0] != '\0') {
                    // Create simple ghost entity for text
                    g_context.ghost_entity.id = entity_create();
                    if (g_context.ghost_entity.id != 0) {
                        // Use a simple plane as text placeholder
                        entity_add_mesh_component(g_context.ghost_entity.id, MESH_PLANE_ID);
                        entity_set_material(g_context.ghost_entity.id, 0xFFFFFFFF); // Ghost material
                        entity_set_scale(g_context.ghost_entity.id, (Vec3){1.0f, 0.2f, 1.0f}); // Text-like proportions
                        entity_set_visibility(g_context.ghost_entity.id, false); // Initially hidden
                        entity_set_collision_enabled(g_context.ghost_entity.id, false); // No collision
                        LOG_DEBUG("Created ghost entity for text '%.20s...'", text_data->text);
                    }
                }
            }
            break;
            
        default:
            LOG_WARN("Unknown drag data type for ghost entity creation");
            break;
    }
}

// MARK: - Legacy Compatibility Functions

void drag_drop_begin(void *item, const char *type) {
    // Legacy function - convert to new system
    DragData* data = drag_drop_create_custom_data(item, sizeof(void*), type);
    if (data) {
        Vec2 position = {0, 0}; // Default position
        drag_drop_start_drag(data, position);
        drag_drop_free_data(data);
    }
}

void drag_drop_accept(const char *type, void *target) {
    // Legacy function - handled by new target system
}

void *drag_drop_get_payload(void) {
    // Legacy function - return current drag data
    return g_context.current_drag.data;
}

// MARK: - Utility Functions

const char* drag_drop_get_state_string(DragState state) {
    switch (state) {
        case DRAG_STATE_IDLE: return "Idle";
        case DRAG_STATE_DRAGGING: return "Dragging";
        case DRAG_STATE_HOVERING: return "Hovering";
        case DRAG_STATE_DROPPING: return "Dropping";
        default: return "Unknown";
    }
}

const char* drag_drop_get_type_string(DragDataType type) {
    switch (type) {
        case DRAG_DATA_TYPE_ASSET: return "Asset";
        case DRAG_DATA_TYPE_ENTITY: return "Entity";
        case DRAG_DATA_TYPE_FILE: return "File";
        case DRAG_DATA_TYPE_TEXT: return "Text";
        case DRAG_DATA_TYPE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

bool drag_drop_is_dragging(void) {
    return g_initialized && g_context.is_dragging;
}

bool drag_drop_has_valid_target(void) {
    return g_initialized && g_context.current_target != NULL;
}

Vec2 drag_drop_get_drag_delta(void) {
    return g_context.delta_position;
}

// MARK: - Configuration

void drag_drop_set_drag_threshold(f32 threshold) {
    if (threshold > 0.0f) {
        g_context.drag_threshold = threshold;
        LOG_INFO("Drag threshold set to: %.2f", threshold);
    }
}

void drag_drop_set_ghost_visibility(bool show) {
    g_context.show_ghost = show;
    if (!show && g_context.ghost_entity.id != 0) {
        // TODO: Hide ghost entity
    }
}

void drag_drop_set_ghost_position(Vec3 position) {
    g_context.ghost_position = position;
    
    // Update ghost entity if it exists
    if (g_context.ghost_entity.id != 0) {
        // TODO: Update ghost entity transform
    }
}
