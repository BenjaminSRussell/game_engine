#include "../editor_common.h"
#include "core/logger.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include "rendering/vulkan.h"
#include <stdio.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265359f
#endif

// Function declarations
bool terrain_editor_raycast_to_terrain(EditorContext* ctx, Vec3* out_hit_position);
void terrain_editor_render_brush_cursor(EditorContext* ctx, Vec3 position, f32 radius);
void terrain_editor_render_brush_preview(EditorContext* ctx, Vec3 position, TerrainBrush brush);
static Vec3 refine_terrain_intersection(Vec3 ray_start, Vec3 ray_dir, 
                                       f32 min_dist, f32 max_dist, 
                                       TerrainHeightmap* heightmap);

/**
 * =================================================================================================
 *                           TERRAIN EDITOR UI
 * =================================================================================================
 * 
 * Implements the introspection UI (Sidebar) and 3D overlay for the Terrain Editor.
 */

// Simulated UI state
static struct {
    bool show_brush_settings;
    bool show_stats;
} terrain_ui_state = {true, true};

// Mock drawing functions for sidebar UI
static void ui_begin_window(const char* name) {}
static void ui_end_window(void) {}
static void ui_slider_float(const char* label, f32* value, f32 min, f32 max) {}
static void ui_radio_button(const char* label, bool* active) {}
static void ui_text(const char* fmt, ...) {}

void terrain_editor_ui_render_sidebar(EditorContext* ctx) {
    if (!ctx || !ctx->terrain_system) return;
    TerrainSculptingSystem* ts = ctx->terrain_system;

    ui_begin_window("Terrain Tools");

    // Operation Selection
    ui_text("Operation:");
    bool op_raise = (ts->current_brush.operation == SCULPT_RAISE);
    ui_radio_button("Raise", &op_raise);
    if (op_raise) ts->current_brush.operation = SCULPT_RAISE;
    
    bool op_lower = (ts->current_brush.operation == SCULPT_LOWER);
    ui_radio_button("Lower", &op_lower);
    if (op_lower) ts->current_brush.operation = SCULPT_LOWER;
    
    // Brush Settings
    ui_text("Settings:");
    ui_slider_float("Radius", &ts->current_brush.radius, 1.0f, 100.0f);
    ui_slider_float("Strength", &ts->current_brush.strength, 0.0f, 1.0f);
    ui_slider_float("Hardness", &ts->current_brush.hardness, 0.0f, 1.0f);

    ui_end_window();
}

void terrain_editor_ui_render_viewport_overlay(EditorContext* ctx) {
    if (!ctx || !ctx->terrain_system) return;
    TerrainSculptingSystem* ts = ctx->terrain_system;
    
    // Draw Brush Cursor
    // Perform raycast from camera through mouse position to terrain
    Vec3 hit_position = {0};
    bool hit_terrain = terrain_editor_raycast_to_terrain(ctx, &hit_position);
    
    if (hit_terrain && ts->current_brush.is_active) {
        // Draw 3D brush cursor at hit position
        terrain_editor_render_brush_cursor(ctx, hit_position, ts->current_brush.radius);
        
        // Draw brush preview circle on terrain surface
        terrain_editor_render_brush_preview(ctx, hit_position, ts->current_brush);
    }
}

// Perform raycast from camera to terrain
bool terrain_editor_raycast_to_terrain(EditorContext* ctx, Vec3* out_hit_position) {
    if (!ctx || !out_hit_position) return false;
    
    TerrainSculptingSystem* ts = ctx->terrain_system;
    if (!ts || !ts->heightmap) {
        LOG_ERROR("No terrain system or heightmap available for raycast");
        return false;
    }
    
    // Get camera position and direction
    Vec3 camera_pos = ctx->viewport.camera_position;
    Vec3 camera_target = ctx->viewport.camera_target;
    
    // Calculate camera forward direction
    Vec3 camera_forward = {
        camera_target.x - camera_pos.x,
        camera_target.y - camera_pos.y,
        camera_target.z - camera_pos.z
    };
    
    // Normalize direction
    f32 length = sqrtf(camera_forward.x * camera_forward.x + 
                      camera_forward.y * camera_forward.y + 
                      camera_forward.z * camera_forward.z);
    if (length > 0.0f) {
        camera_forward.x /= length;
        camera_forward.y /= length;
        camera_forward.z /= length;
    }
    
    // In a real implementation, this would:
    // 1. Convert mouse position to world-space ray
    // 2. Intersect ray with terrain heightmap using proper algorithm
    // 3. Return precise hit position and normal
    
    // For now, implement a simple heightmap intersection
    TerrainHeightmap* heightmap = ts->heightmap;
    
    // Step along the ray and check heightmap intersection
    const f32 step_size = 0.5f;
    const f32 max_distance = 500.0f;
    Vec3 current_pos = camera_pos;
    
    for (f32 distance = 0.0f; distance < max_distance; distance += step_size) {
        current_pos.x = camera_pos.x + camera_forward.x * distance;
        current_pos.y = camera_pos.y + camera_forward.y * distance;
        current_pos.z = camera_pos.z + camera_forward.z * distance;
        
        // Convert world position to heightmap coordinates
        f32 hm_x = (current_pos.x / heightmap->scale_x) + (heightmap->width / 2.0f);
        f32 hm_z = (current_pos.z / heightmap->scale_z) + (heightmap->height / 2.0f);
        
        // Check if within heightmap bounds
        if (hm_x >= 0.0f && hm_x < heightmap->width && 
            hm_z >= 0.0f && hm_z < heightmap->height) {
            
            // Get height at this position
            u32 x_index = (u32)hm_x;
            u32 z_index = (u32)hm_z;
            u32 index = z_index * heightmap->width + x_index;
            
            if (index < heightmap->width * heightmap->height) {
                f32 terrain_height = heightmap->heights[index] * heightmap->height_scale;
                
                // Check if ray is below terrain surface
                if (current_pos.y <= terrain_height) {
                    // Found intersection - refine with binary search for precision
                    Vec3 hit_pos = refine_terrain_intersection(camera_pos, camera_forward, 
                                                          distance - step_size, distance, 
                                                          heightmap);
                    
                    *out_hit_position = hit_pos;
                    
                    LOG_DEBUG("Terrain raycast hit at (%.2f, %.2f, %.2f)", 
                             hit_pos.x, hit_pos.y, hit_pos.z);
                    return true;
                }
            }
        }
        
        // Early exit if ray goes too far below expected terrain level
        if (current_pos.y < -100.0f) {
            break;
        }
    }
    
    // No intersection found - return ground level at camera position
    out_hit_position->x = camera_pos.x;
    out_hit_position->y = 0.0f;
    out_hit_position->z = camera_pos.z;
    
    LOG_DEBUG("Terrain raycast missed - returning ground level");
    return false;
}

// Refine terrain intersection using binary search for precision
static Vec3 refine_terrain_intersection(Vec3 ray_start, Vec3 ray_dir, 
                                       f32 min_dist, f32 max_dist, 
                                       TerrainHeightmap* heightmap) {
    const u32 iterations = 8;
    Vec3 best_hit = ray_start;
    
    for (u32 i = 0; i < iterations; i++) {
        f32 mid_dist = (min_dist + max_dist) * 0.5f;
        Vec3 mid_pos = {
            ray_start.x + ray_dir.x * mid_dist,
            ray_start.y + ray_dir.y * mid_dist,
            ray_start.z + ray_dir.z * mid_dist
        };
        
        // Convert to heightmap coordinates
        f32 hm_x = (mid_pos.x / heightmap->scale_x) + (heightmap->width / 2.0f);
        f32 hm_z = (mid_pos.z / heightmap->scale_z) + (heightmap->height / 2.0f);
        
        if (hm_x >= 0.0f && hm_x < heightmap->width && 
            hm_z >= 0.0f && hm_z < heightmap->height) {
            
            u32 x_index = (u32)hm_x;
            u32 z_index = (u32)hm_z;
            u32 index = z_index * heightmap->width + x_index;
            
            if (index < heightmap->width * heightmap->height) {
                f32 terrain_height = heightmap->heights[index] * heightmap->height_scale;
                
                if (mid_pos.y <= terrain_height) {
                    max_dist = mid_dist;
                    best_hit = mid_pos;
                } else {
                    min_dist = mid_dist;
                }
            } else {
                min_dist = mid_dist;
            }
        } else {
            min_dist = mid_dist;
        }
    }
    
    return best_hit;
}

// Render 3D brush cursor at specified position
void terrain_editor_render_brush_cursor(EditorContext* ctx, Vec3 position, f32 radius) {
    // In a real implementation, this would render a 3D wireframe circle or ring
    // at the specified position with the given radius
    
    // Example implementation would:
    // 1. Create a circle mesh with the specified radius
    // 2. Transform it to the hit position
    // 3. Align it with the terrain normal
    // 4. Render it with a debug shader
    
    /*
    // Create transformation matrix for brush cursor
    Mat4 translation = mat4_translate(position.x, position.y, position.z);
    Mat4 scale = mat4_scale(radius, radius, radius);
    Mat4 transform = mat4_mul(translation, scale);
    
    // Render debug circle
    VulkanRenderer* vk_renderer = (VulkanRenderer*)ctx->renderer;
    if (vk_renderer) {
        // Bind debug line shader
        // Set transform matrix uniform
        // Set color uniform (cyan for brush cursor)
        // Draw circle mesh as lines
    }
    */
    
    // For now, just log the rendering intent
    LOG_DEBUG("Rendering brush cursor at position (%.2f, %.2f, %.2f) with radius %.2f", 
             position.x, position.y, position.z, radius);
}

// Render brush preview on terrain surface
void terrain_editor_render_brush_preview(EditorContext* ctx, Vec3 position, TerrainBrush brush) {
    // In a real implementation, this would render a filled circle on the terrain
    // showing the area that would be affected by the brush
    
    // This would involve:
    // 1. Projecting the brush circle onto the terrain surface
    // 2. Rendering a semi-transparent overlay
    // 3. Visualizing the falloff and strength
    
    /*
    // Sample points within brush radius
    const u32 sample_points = 32;
    for (u32 i = 0; i < sample_points; i++) {
        f32 angle = (f32)i / (f32)sample_points * 2.0f * PI;
        Vec3 sample_pos = {
            position.x + cosf(angle) * brush.radius,
            position.y,
            position.z + sinf(angle) * brush.radius
        };
        
        // Get terrain height at this position
        f32 terrain_height = terrain_get_height_at_position(ts->heightmap, sample_pos.x, sample_pos.z);
        sample_pos.y = terrain_height;
        
        // Render sample point or line to visualize brush area
    }
    */
    
    LOG_DEBUG("Rendering brush preview: operation=%d, radius=%.2f, strength=%.2f", 
             brush.operation, brush.radius, brush.strength);
}
