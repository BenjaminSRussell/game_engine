/**
 * =================================================================================================
 *                          GRID SNAPPING SYSTEM
 * =================================================================================================
 */

#include <editor/asset_creation/grid_snapping.h>
#include <math.h>
#include <stdio.h>

static GridSettings g_grid_settings = {
    .enabled = true,
    .show_grid = true,
    .grid_size = 1.0f,
    .rotation_increment = 15.0f,
    .scale_increment = 0.25f,
    .adaptive_grid = true,
    .adaptive_scale = 1.0f
};

void grid_snapping_init(void) {
    // Initialize default settings or load from config
    g_grid_settings.enabled = true;
    g_grid_settings.grid_size = 1.0f;
}

GridSettings* grid_snapping_get_settings(void) {
    return &g_grid_settings;
}

float grid_snap_value(float val, float increment) {
    if (!g_grid_settings.enabled || increment <= 0.0001f) {
        return val;
    }
    return roundf(val / increment) * increment;
}

void grid_snap_position(float *pos_x, float *pos_y, float *pos_z) {
    if (!g_grid_settings.enabled) return;
    float step = g_grid_settings.grid_size;
    if (pos_x) *pos_x = grid_snap_value(*pos_x, step);
    if (pos_y) *pos_y = grid_snap_value(*pos_y, step);
    if (pos_z) *pos_z = grid_snap_value(*pos_z, step);
}

void grid_snap_rotation(float *rot_x, float *rot_y, float *rot_z) {
    if (!g_grid_settings.enabled) return;
    float step = g_grid_settings.rotation_increment;
    if (rot_x) *rot_x = grid_snap_value(*rot_x, step);
    if (rot_y) *rot_y = grid_snap_value(*rot_y, step);
    if (rot_z) *rot_z = grid_snap_value(*rot_z, step);
}

void grid_snap_scale(float *scale_x, float *scale_y, float *scale_z) {
    if (!g_grid_settings.enabled) return;
    float step = g_grid_settings.scale_increment;
    if (scale_x) *scale_x = grid_snap_value(*scale_x, step);
    if (scale_y) *scale_y = grid_snap_value(*scale_y, step);
    if (scale_z) *scale_z = grid_snap_value(*scale_z, step);
}

void grid_snapping_render(const float *camera_pos, const float *camera_view, const float *camera_proj) {
    if (!g_grid_settings.show_grid) return;

    // Logic for adaptive grid rendering
    // In a real implementation this would push line vertices to a debug renderer
    
    // Example logic to determine grid scale based on height/distance
    float dist = 10.0f; 
    if (camera_pos) {
        // e.g. use y height or distance from origin
        dist = fabsf(camera_pos[1]); 
    }
    
    // Logarithmic scaling for adaptive grid
    // float log_dist = log10f(dist);
    // float scale = powf(10.0f, floorf(log_dist));
    // g_grid_settings.adaptive_scale = scale;
    
    // NOTE: This function is a placeholder for the actual render commands
    // render_line(start, end, color);
}
