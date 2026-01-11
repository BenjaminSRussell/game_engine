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
