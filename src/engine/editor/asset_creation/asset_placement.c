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
        asset_placement_render_preview();
    }
    
    // Render grid if enabled
    if (g_context.grid_snapping_enabled) {
        // TODO: Render grid visualization
    }
    
    // Render collision preview if enabled
    if (g_context.collision_check_enabled) {
        // Render collision bounds preview
        asset_placement_render_collision_preview();
    }
}

AssetPlacementContext* asset_placement_get_context(void) {
    return &g_context;
}

// MARK: - Preview Rendering

void asset_placement_render_preview(void) {
    if (!g_context.preview_visible || g_context.preview_entity.id == 0) {
        return;
    }
    
    // Get preview entity position and bounds
    Vec3 preview_pos = entity_get_position(g_context.preview_entity.id);
    Vec3 min_bounds, max_bounds;
    if (!entity_get_bounds(g_context.preview_entity.id, &min_bounds, &max_bounds)) {
        return;
    }
    
    // Calculate world bounds
    Vec3 world_min = vec3_add(preview_pos, min_bounds);
    Vec3 world_max = vec3_add(preview_pos, max_bounds);
    
    // Set preview material properties
    Material preview_material = {0};
    preview_material.albedo = (Vec4){0.7f, 0.7f, 1.0f, 0.6f}; // Semi-transparent blue-white
    preview_material.metallic = 0.2f;
    preview_material.roughness = 0.7f;
    preview_material.emissive = (Vec3){0.1f, 0.1f, 0.3f}; // Subtle blue glow
    
    // Apply preview material override
    entity_set_material_override(g_context.preview_entity.id, &preview_material);
    
    // Enable depth testing but disable depth writing for proper transparency
    entity_set_depth_write_enabled(g_context.preview_entity.id, false);
    
    // Add pulsing effect
    f32 pulse = sinf(g_context.preview_time * 2.0f) * 0.1f + 0.9f;
    preview_material.albedo.w = pulse * 0.6f; // Pulsing transparency
    entity_set_material_override(g_context.preview_entity.id, &preview_material);
    
    // Render outline for better visibility
    Vec3 outline_color = (Vec3){0.3f, 0.5f, 1.0f}; // Light blue
    f32 outline_width = 2.0f;
    entity_set_outline(g_context.preview_entity.id, true, outline_color, outline_width);
    
    // Render placement indicator at base
    Vec3 base_center = (Vec3){preview_pos.x, world_min.y, preview_pos.z};
    
    // Draw placement crosshair
    f32 crosshair_size = 0.3f;
    Vec3 cross_color = (Vec3){0.2f, 0.8f, 1.0f}; // Cyan
    f32 cross_width = 2.0f;
    
    debug_renderer_draw_line(
        (Vec3){base_center.x - crosshair_size, base_center.y + 0.01f, base_center.z},
        (Vec3){base_center.x + crosshair_size, base_center.y + 0.01f, base_center.z},
        cross_color, cross_width
    );
    debug_renderer_draw_line(
        (Vec3){base_center.x, base_center.y + 0.01f, base_center.z - crosshair_size},
        (Vec3){base_center.x, base_center.y + 0.01f, base_center.z + crosshair_size},
        cross_color, cross_width
    );
    
    // Render height indicator line
    Vec3 top_center = (Vec3){preview_pos.x, world_max.y, preview_pos.z};
    debug_renderer_draw_line(
        base_center,
        top_center,
        (Vec3){0.5f, 0.5f, 0.5f}, 1.0f
    );
    
    // Render rotation indicator if rotation is active
    if (g_context.rotation_active) {
        f32 rotation_radius = 0.5f;
        u32 segments = 32;
        f32 angle_step = 2.0f * PI / segments;
        
        // Draw rotation circle
        for (u32 i = 0; i < segments; i++) {
            f32 angle1 = i * angle_step;
            f32 angle2 = (i + 1) * angle_step;
            
            Vec3 p1 = {
                base_center.x + cosf(angle1) * rotation_radius,
                base_center.y + 0.02f,
                base_center.z + sinf(angle1) * rotation_radius
            };
            Vec3 p2 = {
                base_center.x + cosf(angle2) * rotation_radius,
                base_center.y + 0.02f,
                base_center.z + sinf(angle2) * rotation_radius
            };
            
            debug_renderer_draw_line(p1, p2, (Vec3){1.0f, 1.0f, 0.0f}, 2.0f); // Yellow
        }
        
        // Draw rotation direction indicator
        Vec3 rotation_dir = (Vec3){cosf(g_context.current_rotation), 0.0f, sinf(g_context.current_rotation)};
        debug_renderer_draw_line(
            base_center,
            vec3_add(base_center, vec3_scale(rotation_dir, rotation_radius * 1.2f)),
            (Vec3){1.0f, 0.5f, 0.0f}, 3.0f // Orange
        );
    }
    
    // Render scale indicator if scaling is active
    if (g_context.scale_active) {
        f32 scale_indicator_size = 0.2f;
        Vec3 scale_color = (Vec3){1.0f, 0.0f, 1.0f}; // Magenta
        
        // Draw scale handles at corners
        Vec3 corners[8] = {
            {world_min.x, world_min.y, world_min.z}, {world_max.x, world_min.y, world_min.z},
            {world_min.x, world_max.y, world_min.z}, {world_max.x, world_max.y, world_min.z},
            {world_min.x, world_min.y, world_max.z}, {world_max.x, world_min.y, world_max.z},
            {world_min.x, world_max.y, world_max.z}, {world_max.x, world_max.y, world_max.z}
        };
        
        for (u32 i = 0; i < 8; i++) {
            // Draw small cube at each corner
            Vec3 corner = corners[i];
            Vec3 corner_size = (Vec3){scale_indicator_size, scale_indicator_size, scale_indicator_size};
            
            // Draw corner as wireframe cube
            Vec3 corner_corners[8] = {
                {corner.x - corner_size.x, corner.y - corner_size.y, corner.z - corner_size.z},
                {corner.x + corner_size.x, corner.y - corner_size.y, corner.z - corner_size.z},
                {corner.x - corner_size.x, corner.y + corner_size.y, corner.z - corner_size.z},
                {corner.x + corner_size.x, corner.y + corner_size.y, corner.z - corner_size.z},
                {corner.x - corner_size.x, corner.y - corner_size.y, corner.z + corner_size.z},
                {corner.x + corner_size.x, corner.y - corner_size.y, corner.z + corner_size.z},
                {corner.x - corner_size.x, corner.y + corner_size.y, corner.z + corner_size.z},
                {corner.x + corner_size.x, corner.y + corner_size.y, corner.z + corner_size.z}
            };
            
            // Draw edges of corner cube
            debug_renderer_draw_line(corner_corners[0], corner_corners[1], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[1], corner_corners[3], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[3], corner_corners[2], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[2], corner_corners[0], scale_color, 1.0f);
            
            debug_renderer_draw_line(corner_corners[4], corner_corners[5], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[5], corner_corners[7], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[7], corner_corners[6], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[6], corner_corners[4], scale_color, 1.0f);
            
            debug_renderer_draw_line(corner_corners[0], corner_corners[4], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[1], corner_corners[5], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[2], corner_corners[6], scale_color, 1.0f);
            debug_renderer_draw_line(corner_corners[3], corner_corners[7], scale_color, 1.0f);
        }
    }
    
    // Render surface alignment indicator if surface snapping is active
    if (g_context.surface_snapping_enabled && g_context.snap_position_valid) {
        Vec3 snap_normal = g_context.snap_normal;
        
        // Draw normal vector
        f32 normal_length = 1.0f;
        Vec3 normal_end = vec3_add(base_center, vec3_scale(snap_normal, normal_length));
        debug_renderer_draw_line(base_center, normal_end, (Vec3){0.0f, 1.0f, 0.0f}, 2.0f); // Green
        
        // Draw surface plane indicator
        f32 plane_size = 0.4f;
        Vec3 plane_right = vec3_cross(snap_normal, (Vec3){0.0f, 1.0f, 0.0f});
        if (vec3_length(plane_right) < 0.1f) {
            plane_right = vec3_cross(snap_normal, (Vec3){0.0f, 0.0f, 1.0f});
        }
        plane_right = vec3_normalize(plane_right);
        Vec3 plane_forward = vec3_cross(snap_normal, plane_right);
        
        // Draw plane as a square
        Vec3 plane_corners[4] = {
            vec3_subtract(vec3_subtract(base_center, vec3_scale(plane_right, plane_size)), vec3_scale(plane_forward, plane_size)),
            vec3_add(vec3_subtract(base_center, vec3_scale(plane_right, plane_size)), vec3_scale(plane_forward, plane_size)),
            vec3_add(vec3_add(base_center, vec3_scale(plane_right, plane_size)), vec3_scale(plane_forward, plane_size)),
            vec3_subtract(vec3_add(base_center, vec3_scale(plane_right, plane_size)), vec3_scale(plane_forward, plane_size))
        };
        
        debug_renderer_draw_line(plane_corners[0], plane_corners[1], (Vec3){0.0f, 0.8f, 0.0f}, 1.0f);
        debug_renderer_draw_line(plane_corners[1], plane_corners[2], (Vec3){0.0f, 0.8f, 0.0f}, 1.0f);
        debug_renderer_draw_line(plane_corners[2], plane_corners[3], (Vec3){0.0f, 0.8f, 0.0f}, 1.0f);
        debug_renderer_draw_line(plane_corners[3], plane_corners[0], (Vec3){0.0f, 0.8f, 0.0f}, 1.0f);
    }
    
    // Render asset type indicator
    if (g_context.current_type != ASSET_TYPE_UNKNOWN) {
        Vec3 label_pos = (Vec3){preview_pos.x, world_max.y + 0.3f, preview_pos.z};
        
        // Create temporary text entity for asset type label
        const char* type_names[] = {
            "Unknown", "Static", "Dynamic", "Character", "Vehicle", "Weapon", "Tool", "Prop"
        };
        
        if (g_context.current_type < sizeof(type_names) / sizeof(type_names[0])) {
            // Draw label background
            f32 label_size = 0.2f;
            Vec3 label_color = (Vec3){0.2f, 0.2f, 0.2f};
            
            debug_renderer_draw_line(
                (Vec3){label_pos.x - label_size, label_pos.y, label_pos.z - label_size},
                (Vec3){label_pos.x + label_size, label_pos.y, label_pos.z - label_size},
                label_color, 1.0f
            );
            debug_renderer_draw_line(
                (Vec3){label_pos.x + label_size, label_pos.y, label_pos.z - label_size},
                (Vec3){label_pos.x + label_size, label_pos.y, label_pos.z + label_size},
                label_color, 1.0f
            );
            debug_renderer_draw_line(
                (Vec3){label_pos.x + label_size, label_pos.y, label_pos.z + label_size},
                (Vec3){label_pos.x - label_size, label_pos.y, label_pos.z + label_size},
                label_color, 1.0f
            );
            debug_renderer_draw_line(
                (Vec3){label_pos.x - label_size, label_pos.y, label_pos.z + label_size},
                (Vec3){label_pos.x - label_size, label_pos.y, label_pos.z - label_size},
                label_color, 1.0f
            );
        }
    }
    
    LOG_TRACE("Rendered asset placement preview at position (%.2f, %.2f, %.2f)", 
             preview_pos.x, preview_pos.y, preview_pos.z);
}

// MARK: - Collision Preview

void asset_placement_render_collision_preview(void) {
    if (!g_context.collision_check_enabled || !g_context.preview_entity.id != 0) {
        return;
    }
    
    // Get preview entity bounds
    Vec3 min_bounds, max_bounds;
    if (!entity_get_bounds(g_context.preview_entity.id, &min_bounds, &max_bounds)) {
        LOG_WARN("Failed to get preview entity bounds for collision preview");
        return;
    }
    
    // Get preview entity position
    Vec3 preview_pos = entity_get_position(g_context.preview_entity.id);
    
    // Calculate world bounds
    Vec3 world_min = vec3_add(preview_pos, min_bounds);
    Vec3 world_max = vec3_add(preview_pos, max_bounds);
    
    // Perform collision check
    bool has_collision = asset_placement_check_collision_at_position(preview_pos, g_context.current_type);
    
    // Set collision preview colors based on result
    Vec3 collision_color = has_collision ? (Vec3){1.0f, 0.2f, 0.2f} : (Vec3){0.2f, 1.0f, 0.2f}; // Red for collision, green for valid
    Vec3 bounds_color = has_collision ? (Vec3){0.8f, 0.1f, 0.1f} : (Vec3){0.1f, 0.8f, 0.1f};
    f32 line_width = has_collision ? 3.0f : 2.0f;
    
    // Render collision bounds as wireframe box
    Vec3 corners[8] = {
        {world_min.x, world_min.y, world_min.z}, {world_max.x, world_min.y, world_min.z},
        {world_min.x, world_max.y, world_min.z}, {world_max.x, world_max.y, world_min.z},
        {world_min.x, world_min.y, world_max.z}, {world_max.x, world_min.y, world_max.z},
        {world_min.x, world_max.y, world_max.z}, {world_max.x, world_max.y, world_max.z}
    };
    
    // Bottom face
    debug_renderer_draw_line(corners[0], corners[1], bounds_color, line_width);
    debug_renderer_draw_line(corners[1], corners[3], bounds_color, line_width);
    debug_renderer_draw_line(corners[3], corners[2], bounds_color, line_width);
    debug_renderer_draw_line(corners[2], corners[0], bounds_color, line_width);
    
    // Top face
    debug_renderer_draw_line(corners[4], corners[5], bounds_color, line_width);
    debug_renderer_draw_line(corners[5], corners[7], bounds_color, line_width);
    debug_renderer_draw_line(corners[7], corners[6], bounds_color, line_width);
    debug_renderer_draw_line(corners[6], corners[4], bounds_color, line_width);
    
    // Vertical edges
    debug_renderer_draw_line(corners[0], corners[4], bounds_color, line_width);
    debug_renderer_draw_line(corners[1], corners[5], bounds_color, line_width);
    debug_renderer_draw_line(corners[2], corners[6], bounds_color, line_width);
    debug_renderer_draw_line(corners[3], corners[7], bounds_color, line_width);
    
    // Render collision indicator at center
    Vec3 center = vec3_multiply(vec3_add(world_min, world_max), 0.5f);
    
    if (has_collision) {
        // Render red X for collision
        f32 indicator_size = 0.3f;
        debug_renderer_draw_line(
            (Vec3){center.x - indicator_size, center.y + 0.1f, center.z - indicator_size},
            (Vec3){center.x + indicator_size, center.y + 0.1f, center.z + indicator_size},
            collision_color, 4.0f
        );
        debug_renderer_draw_line(
            (Vec3){center.x + indicator_size, center.y + 0.1f, center.z - indicator_size},
            (Vec3){center.x - indicator_size, center.y + 0.1f, center.z + indicator_size},
            collision_color, 4.0f
        );
        
        // Render pulsing effect
        f32 pulse = sinf(g_context.preview_time * 5.0f) * 0.2f + 0.8f;
        Vec3 pulse_color = vec3_scale(collision_color, pulse);
        
        // Draw additional outline with pulsing
        debug_renderer_draw_line(corners[0], corners[1], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[1], corners[3], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[3], corners[2], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[2], corners[0], pulse_color, line_width + 1.0f);
        
        debug_renderer_draw_line(corners[4], corners[5], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[5], corners[7], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[7], corners[6], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[6], corners[4], pulse_color, line_width + 1.0f);
        
        debug_renderer_draw_line(corners[0], corners[4], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[1], corners[5], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[2], corners[6], pulse_color, line_width + 1.0f);
        debug_renderer_draw_line(corners[3], corners[7], pulse_color, line_width + 1.0f);
    } else {
        // Render green checkmark for valid placement
        f32 check_size = 0.2f;
        debug_renderer_draw_line(
            (Vec3){center.x - check_size, center.y + 0.1f, center.z},
            (Vec3){center.x, center.y + 0.1f, center.z + check_size},
            collision_color, 3.0f
        );
        debug_renderer_draw_line(
            (Vec3){center.x, center.y + 0.1f, center.z + check_size},
            (Vec3){center.x + check_size * 1.5f, center.y + 0.1f, center.z - check_size * 0.5f},
            collision_color, 3.0f
        );
    }
    
    // Render collision spheres for detailed collision checking
    if (g_context.show_collision_spheres) {
        u32 sphere_count = entity_get_collision_sphere_count(g_context.preview_entity.id);
        for (u32 i = 0; i < sphere_count; i++) {
            Vec3 sphere_center;
            f32 sphere_radius;
            if (entity_get_collision_sphere(g_context.preview_entity.id, i, &sphere_center, &sphere_radius)) {
                Vec3 world_sphere_center = vec3_add(preview_pos, sphere_center);
                
                // Draw sphere as wireframe using circles
                u32 segments = 16;
                f32 angle_step = 2.0f * PI / segments;
                
                // XY plane circle
                for (u32 j = 0; j < segments; j++) {
                    f32 angle1 = j * angle_step;
                    f32 angle2 = (j + 1) * angle_step;
                    
                    Vec3 p1 = {
                        world_sphere_center.x + cosf(angle1) * sphere_radius,
                        world_sphere_center.y + sinf(angle1) * sphere_radius,
                        world_sphere_center.z
                    };
                    Vec3 p2 = {
                        world_sphere_center.x + cosf(angle2) * sphere_radius,
                        world_sphere_center.y + sinf(angle2) * sphere_radius,
                        world_sphere_center.z
                    };
                    
                    debug_renderer_draw_line(p1, p2, bounds_color, 1.0f);
                }
                
                // XZ plane circle
                for (u32 j = 0; j < segments; j++) {
                    f32 angle1 = j * angle_step;
                    f32 angle2 = (j + 1) * angle_step;
                    
                    Vec3 p1 = {
                        world_sphere_center.x + cosf(angle1) * sphere_radius,
                        world_sphere_center.y,
                        world_sphere_center.z + sinf(angle1) * sphere_radius
                    };
                    Vec3 p2 = {
                        world_sphere_center.x + cosf(angle2) * sphere_radius,
                        world_sphere_center.y,
                        world_sphere_center.z + sinf(angle2) * sphere_radius
                    };
                    
                    debug_renderer_draw_line(p1, p2, bounds_color, 1.0f);
                }
                
                // YZ plane circle
                for (u32 j = 0; j < segments; j++) {
                    f32 angle1 = j * angle_step;
                    f32 angle2 = (j + 1) * angle_step;
                    
                    Vec3 p1 = {
                        world_sphere_center.x,
                        world_sphere_center.y + cosf(angle1) * sphere_radius,
                        world_sphere_center.z + sinf(angle1) * sphere_radius
                    };
                    Vec3 p2 = {
                        world_sphere_center.x,
                        world_sphere_center.y + cosf(angle2) * sphere_radius,
                        world_sphere_center.z + sinf(angle2) * sphere_radius
                    };
                    
                    debug_renderer_draw_line(p1, p2, bounds_color, 1.0f);
                }
            }
        }
    }
    
    // Render ground contact point if valid placement
    if (!has_collision && g_context.show_ground_contact) {
        f32 ground_height = asset_placement_get_ground_height_at_position(preview_pos.x, preview_pos.z);
        if (ground_height != FLT_MAX) {
            Vec3 ground_point = {preview_pos.x, ground_height, preview_pos.z};
            
            // Draw ground contact indicator
            f32 contact_size = 0.15f;
            Vec3 contact_color = (Vec3){0.0f, 0.8f, 0.0f}; // Green
            
            debug_renderer_draw_line(
                (Vec3){ground_point.x - contact_size, ground_point.y + 0.01f, ground_point.z - contact_size},
                (Vec3){ground_point.x + contact_size, ground_point.y + 0.01f, ground_point.z + contact_size},
                contact_color, 2.0f
            );
            debug_renderer_draw_line(
                (Vec3){ground_point.x + contact_size, ground_point.y + 0.01f, ground_point.z - contact_size},
                (Vec3){ground_point.x - contact_size, ground_point.y + 0.01f, ground_point.z + contact_size},
                contact_color, 2.0f
            );
            
            // Draw line from preview entity to ground contact
            debug_renderer_draw_line(
                (Vec3){preview_pos.x, world_min.y, preview_pos.z},
                ground_point,
                (Vec3){0.5f, 0.5f, 0.5f}, 1.0f
            );
        }
    }
    
    LOG_TRACE("Rendered collision preview: %s", has_collision ? "COLLISION" : "VALID");
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
