// Asset Placement System implementation
#include "editor/asset_creation/asset_placement.h"
#include <core/logger.h>
#include <ecs/ecs.h>
#include <ecs/components/transform.h>
#include <math/vec3.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global asset placement context
static AssetPlacementContext g_context = {0};
static bool g_initialized = false;

// MARK: - Asset Placement System Management

bool asset_placement_init(void) {
    if (g_initialized) {
        return true;
    }
    
    memset(&g_context, 0, sizeof(AssetPlacementContext));
    g_context.grid_size = 1.0f;
    g_context.grid_snapping_enabled = true;
    g_context.collision_check_enabled = true;
    
    g_initialized = true;
    LOG_INFO("Asset placement system initialized");
    return true;
}

void asset_placement_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    asset_placement_reset_context();
    g_initialized = false;
    LOG_INFO("Asset placement system shutdown");
}

void asset_placement_update(float delta_time) {
    if (!g_initialized) {
        return;
    }
    
    // Update preview entity position
    if (g_context.preview_visible && g_context.preview_entity.id != 0) {
        // Update preview entity transform
        TransformComponent* transform = (TransformComponent*)ecs_get_component(
            NULL, g_context.preview_entity, TRANSFORM_COMPONENT_ID);
        
        if (transform) {
            transform->position = g_context.preview_position;
            
            // Apply grid snapping if enabled
            if (g_context.grid_snapping_enabled) {
                asset_placement_snap_to_grid(&transform->position, g_context.grid_size);
            }
        }
    }
}

void asset_placement_render(void) {
    if (!g_initialized) {
        return;
    }
    
    // Render preview entity
    if (g_context.preview_visible && g_context.preview_entity.id != 0) {
        // Preview entity will be rendered by the main renderer
    }
    
    // Render grid if enabled
    if (g_context.grid_snapping_enabled) {
        // TODO: Render grid visualization
    }
    
    // Render collision preview if enabled
    if (g_context.collision_check_enabled) {
        // TODO: Render collision bounds preview
    }
}

AssetPlacementContext* asset_placement_get_context(void) {
    return &g_context;
}

// MARK: - Grid Visualization

void asset_placement_render_grid(void) {
    if (!g_context.grid_snapping_enabled) {
        return;
    }
    
    // Get camera position for adaptive grid rendering
    Vec3 camera_pos = camera_get_position();
    f32 grid_size = g_context.grid_size;
    
    // Calculate grid range based on camera position
    f32 grid_range = 50.0f; // Render grid within 50 units of camera
    s32 grid_start_x = (s32)floorf((camera_pos.x - grid_range) / grid_size);
    s32 grid_end_x = (s32)ceilf((camera_pos.x + grid_range) / grid_size);
    s32 grid_start_z = (s32)floorf((camera_pos.z - grid_range) / grid_size);
    s32 grid_end_z = (s32)ceilf((camera_pos.z + grid_range) / grid_size);
    
    // Set grid line colors
    Vec3 major_color = (Vec3){0.3f, 0.3f, 0.3f};  // Dark gray for major lines
    Vec3 minor_color = (Vec3){0.2f, 0.2f, 0.2f};  // Lighter gray for minor lines
    Vec3 axis_color = (Vec3){0.1f, 0.3f, 0.1f};   // Green tint for axis lines
    
    f32 major_line_width = 2.0f;
    f32 minor_line_width = 1.0f;
    f32 axis_line_width = 3.0f;
    
    // Render X-axis lines (horizontal lines)
    for (s32 z = grid_start_z; z <= grid_end_z; z++) {
        f32 z_pos = (f32)z * grid_size;
        Vec3 start = {(f32)grid_start_x * grid_size, 0.0f, z_pos};
        Vec3 end = {(f32)grid_end_x * grid_size, 0.0f, z_pos};
        
        Vec3 color = minor_color;
        f32 width = minor_line_width;
        
        // Highlight axis lines (z=0)
        if (z == 0) {
            color = axis_color;
            width = axis_line_width;
        }
        // Highlight major grid lines (every 5 units)
        else if (z % 5 == 0) {
            color = major_color;
            width = major_line_width;
        }
        
        debug_renderer_draw_line(start, end, color, width);
    }
    
    // Render Z-axis lines (vertical lines)
    for (s32 x = grid_start_x; x <= grid_end_x; x++) {
        f32 x_pos = (f32)x * grid_size;
        Vec3 start = {x_pos, 0.0f, (f32)grid_start_z * grid_size};
        Vec3 end = {x_pos, 0.0f, (f32)grid_end_z * grid_size};
        
        Vec3 color = minor_color;
        f32 width = minor_line_width;
        
        // Highlight axis lines (x=0)
        if (x == 0) {
            color = axis_color;
            width = axis_line_width;
        }
        // Highlight major grid lines (every 5 units)
        else if (x % 5 == 0) {
            color = major_color;
            width = major_line_width;
        }
        
        debug_renderer_draw_line(start, end, color, width);
    }
    
    // Render grid origin marker
    Vec3 origin = {0.0f, 0.0f, 0.0f};
    f32 marker_size = 0.5f;
    
    // X-axis marker (red)
    debug_renderer_draw_line(origin, (Vec3){marker_size, 0.0f, 0.0f}, (Vec3){1.0f, 0.0f, 0.0f}, 4.0f);
    
    // Z-axis marker (blue)
    debug_renderer_draw_line(origin, (Vec3){0.0f, 0.0f, marker_size}, (Vec3){0.0f, 0.0f, 1.0f}, 4.0f);
    
    // Y-axis marker (green, pointing up)
    debug_renderer_draw_line(origin, (Vec3){0.0f, marker_size, 0.0f}, (Vec3){0.0f, 1.0f, 0.0f}, 4.0f);
    
    // Render grid labels at major intersections
    if (g_context.show_grid_labels) {
        for (s32 x = grid_start_x; x <= grid_end_x; x += 5) {
            for (s32 z = grid_start_z; z <= grid_end_z; z += 5) {
                Vec3 label_pos = {(f32)x * grid_size, 0.1f, (f32)z * grid_size};
                
                // Create temporary text entity for grid label
                char label_text[32];
                snprintf(label_text, sizeof(label_text), "%d,%d", x, z);
                
                // This would use a text rendering system
                // For now, we'll just draw a small marker at label positions
                Vec3 marker_color = (Vec3){0.6f, 0.6f, 0.6f};
                f32 marker_size_small = 0.1f;
                
                debug_renderer_draw_line(
                    (Vec3){label_pos.x - marker_size_small, label_pos.y, label_pos.z},
                    (Vec3){label_pos.x + marker_size_small, label_pos.y, label_pos.z},
                    marker_color, 1.0f
                );
                debug_renderer_draw_line(
                    (Vec3){label_pos.x, label_pos.y, label_pos.z - marker_size_small},
                    (Vec3){label_pos.x, label_pos.y, label_pos.z + marker_size_small},
                    marker_color, 1.0f
                );
            }
        }
    }
    
    // Render snap position indicator if we have a current snap position
    if (g_context.snap_position_valid) {
        Vec3 snap_pos = g_context.snap_position;
        
        // Draw crosshair at snap position
        f32 crosshair_size = grid_size * 0.5f;
        Vec3 cross_color = (Vec3){1.0f, 1.0f, 0.0f}; // Yellow
        f32 cross_width = 3.0f;
        
        // Horizontal crosshair line
        debug_renderer_draw_line(
            (Vec3){snap_pos.x - crosshair_size, snap_pos.y + 0.01f, snap_pos.z},
            (Vec3){snap_pos.x + crosshair_size, snap_pos.y + 0.01f, snap_pos.z},
            cross_color, cross_width
        );
        
        // Vertical crosshair line
        debug_renderer_draw_line(
            (Vec3){snap_pos.x, snap_pos.y + 0.01f, snap_pos.z - crosshair_size},
            (Vec3){snap_pos.x, snap_pos.y + 0.01f, snap_pos.z + crosshair_size},
            cross_color, cross_width
        );
        
        // Draw snap position bounds (if we have bounds)
        if (g_context.snap_bounds_valid) {
            Vec3 min = g_context.snap_bounds_min;
            Vec3 max = g_context.snap_bounds_max;
            
            // Draw bounds outline
            Vec3 bounds_color = (Vec3){1.0f, 0.5f, 0.0f}; // Orange
            f32 bounds_width = 2.0f;
            
            // Bottom face
            debug_renderer_draw_line((Vec3){min.x, min.y, min.z}, (Vec3){max.x, min.y, min.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, min.y, min.z}, (Vec3){max.x, min.y, max.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, min.y, max.z}, (Vec3){min.x, min.y, max.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){min.x, min.y, max.z}, (Vec3){min.x, min.y, min.z}, bounds_color, bounds_width);
            
            // Top face
            debug_renderer_draw_line((Vec3){min.x, max.y, min.z}, (Vec3){max.x, max.y, min.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, max.y, min.z}, (Vec3){max.x, max.y, max.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, max.y, max.z}, (Vec3){min.x, max.y, max.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){min.x, max.y, max.z}, (Vec3){min.x, max.y, min.z}, bounds_color, bounds_width);
            
            // Vertical edges
            debug_renderer_draw_line((Vec3){min.x, min.y, min.z}, (Vec3){min.x, max.y, min.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, min.y, min.z}, (Vec3){max.x, max.y, min.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){min.x, min.y, max.z}, (Vec3){min.x, max.y, max.z}, bounds_color, bounds_width);
            debug_renderer_draw_line((Vec3){max.x, min.y, max.z}, (Vec3){max.x, max.y, max.z}, bounds_color, bounds_width);
        }
    }
    
    LOG_TRACE("Rendered grid with %dx%d lines, size=%.2f", 
             (grid_end_x - grid_start_x + 1), (grid_end_z - grid_start_z + 1), grid_size);
}

// MARK: - Asset Placement Operations

PlacementResult asset_placement_place(void* asset, Vec3 position) {
    if (!g_initialized || !asset) {
        return PLACEMENT_FAILED_INVALID_ASSET;
    }
    
    // Validate position
    if (!asset_placement_is_valid_position(position, g_context.current_type)) {
        return PLACEMENT_FAILED_INVALID_POSITION;
    }
    
    // Check collision if enabled
    if (g_context.collision_check_enabled && 
        asset_placement_check_collision(position, g_context.current_type)) {
        return PLACEMENT_FAILED_COLLISION;
    }
    
    // Apply grid snapping if enabled
    Vec3 final_position = position;
    if (g_context.grid_snapping_enabled) {
        asset_placement_snap_to_grid(&final_position, g_context.grid_size);
    }
    
    // Create entity for the placed asset
    Entity placed_entity = {0}; // TODO: Create actual entity
    
    // Set up transform component
    TransformComponent* transform = (TransformComponent*)ecs_get_component(
        NULL, placed_entity, TRANSFORM_COMPONENT_ID);
    
    if (transform) {
        transform->position = final_position;
    }
    
    LOG_INFO("Placed asset at position: %.2f, %.2f, %.2f", 
             final_position.x, final_position.y, final_position.z);
    
    return PLACEMENT_SUCCESS;
}

PlacementResult asset_placement_place_at_cursor(void* asset) {
    if (!g_initialized) {
        return PLACEMENT_FAILED_INVALID_POSITION;
    }
    
    return asset_placement_place(asset, g_context.preview_position);
}

void asset_placement_remove_asset(Vec3 position) {
    if (!g_initialized) {
        return;
    }
    
    // Find and remove asset at position
    // TODO: Implement asset removal logic
    LOG_INFO("Removed asset at position: %.2f, %.2f, %.2f", 
             position.x, position.y, position.z);
}

void asset_placement_move_asset(Vec3 from_position, Vec3 to_position) {
    if (!g_initialized) {
        return;
    }
    
    // Remove from old position and place at new position
    asset_placement_remove_asset(from_position);
    
    if (g_context.current_asset) {
        asset_placement_place(g_context.current_asset, to_position);
    }
    
    LOG_INFO("Moved asset from %.2f,%.2f,%.2f to %.2f,%.2f,%.2f", 
             from_position.x, from_position.y, from_position.z,
             to_position.x, to_position.y, to_position.z);
}

// MARK: - Grid and Snapping

void asset_placement_snap_to_grid(Vec3* position, float grid_size) {
    if (!position || grid_size <= 0.0f) {
        return;
    }
    
    position->x = floorf(position->x / grid_size) * grid_size + grid_size * 0.5f;
    position->y = floorf(position->y / grid_size) * grid_size + grid_size * 0.5f;
    position->z = floorf(position->z / grid_size) * grid_size + grid_size * 0.5f;
}

Vec3 asset_placement_get_snapped_position(Vec3 position, float grid_size) {
    Vec3 snapped = position;
    asset_placement_snap_to_grid(&snapped, grid_size);
    return snapped;
}

void asset_placement_set_grid_size(float size) {
    if (size > 0.0f) {
        g_context.grid_size = size;
        LOG_INFO("Grid size set to: %.2f", size);
    }
}

void asset_placement_toggle_grid_snapping(void) {
    g_context.grid_snapping_enabled = !g_context.grid_snapping_enabled;
    LOG_INFO("Grid snapping %s", g_context.grid_snapping_enabled ? "enabled" : "disabled");
}

// MARK: - Collision and Validation

bool asset_placement_is_valid_position(Vec3 position, AssetType type) {
    // Check if position is within world bounds
    if (!asset_placement_is_in_bounds(position)) {
        return false;
    }
    
    // Type-specific validation
    switch (type) {
        case ASSET_TYPE_BLOCK:
            // Blocks can only be placed on solid surfaces
            return position.y >= 0.0f;
            
        case ASSET_TYPE_ITEM:
            // Items can be placed anywhere
            return true;
            
        case ASSET_TYPE_MOB:
            // Mobs need ground to stand on
            return position.y >= 0.0f;
            
        case ASSET_TYPE_PROP:
            // Props can be placed anywhere
            return true;
            
        case ASSET_TYPE_LIGHT:
            // Lights can be placed anywhere
            return true;
            
        default:
            return false;
    }
}

bool asset_placement_check_collision(Vec3 position, AssetType type) {
    // Simple collision check - in a real implementation this would
    // check against existing entities and world geometry
    
    // For now, just check if there's already something at this position
    // TODO: Implement proper collision detection
    
    return false; // No collision for now
}

bool asset_placement_is_in_bounds(Vec3 position) {
    // Define world bounds (adjust as needed)
    const float WORLD_MIN_X = -1000.0f;
    const float WORLD_MAX_X = 1000.0f;
    const float WORLD_MIN_Y = -100.0f;
    const float WORLD_MAX_Y = 500.0f;
    const float WORLD_MIN_Z = -1000.0f;
    const float WORLD_MAX_Z = 1000.0f;
    
    return (position.x >= WORLD_MIN_X && position.x <= WORLD_MAX_X &&
            position.y >= WORLD_MIN_Y && position.y <= WORLD_MAX_Y &&
            position.z >= WORLD_MIN_Z && position.z <= WORLD_MAX_Z);
}

void asset_placement_toggle_collision_check(void) {
    g_context.collision_check_enabled = !g_context.collision_check_enabled;
    LOG_INFO("Collision check %s", g_context.collision_check_enabled ? "enabled" : "disabled");
}

// MARK: - Preview System

void asset_placement_set_preview_asset(void* asset, AssetType type) {
    g_context.current_asset = asset;
    g_context.current_type = type;
    
    // Create or update preview entity
    if (asset) {
        // TODO: Create preview entity with asset's mesh
        g_context.preview_visible = true;
    } else {
        asset_placement_show_preview(false);
    }
}

void asset_placement_update_preview_position(Vec3 world_position) {
    g_context.preview_position = world_position;
}

void asset_placement_show_preview(bool show) {
    g_context.preview_visible = show;
    
    if (!show && g_context.preview_entity.id != 0) {
        // Hide or destroy preview entity
        // TODO: Hide preview entity
    }
}

void asset_placement_update_preview(void) {
    if (g_context.preview_visible && g_context.preview_entity.id != 0) {
        // Update preview entity appearance
        // TODO: Update preview mesh/material
    }
}

// MARK: - Asset Management

void asset_placement_set_current_asset(void* asset, AssetType type) {
    g_context.current_asset = asset;
    g_context.current_type = type;
    asset_placement_set_preview_asset(asset, type);
}

void* asset_placement_get_current_asset(void) {
    return g_context.current_asset;
}

AssetType asset_placement_get_current_type(void) {
    return g_context.current_type;
}

// MARK: - Utility Functions

const char* asset_placement_get_result_string(PlacementResult result) {
    switch (result) {
        case PLACEMENT_SUCCESS: return "Success";
        case PLACEMENT_FAILED_INVALID_POSITION: return "Invalid Position";
        case PLACEMENT_FAILED_COLLISION: return "Collision";
        case PLACEMENT_FAILED_OUT_OF_BOUNDS: return "Out of Bounds";
        case PLACEMENT_FAILED_INVALID_ASSET: return "Invalid Asset";
        default: return "Unknown";
    }
}

bool asset_placement_is_in_placement_mode(void) {
    return g_initialized && g_context.current_asset != NULL;
}

void asset_placement_reset_context(void) {
    // Clean up preview entity
    if (g_context.preview_entity.id != 0) {
        // TODO: Destroy preview entity
        g_context.preview_entity.id = 0;
    }
    
    memset(&g_context, 0, sizeof(AssetPlacementContext));
    g_context.grid_size = 1.0f;
    g_context.grid_snapping_enabled = true;
    g_context.collision_check_enabled = true;
}
