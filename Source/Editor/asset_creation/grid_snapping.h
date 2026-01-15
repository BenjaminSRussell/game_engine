#pragma once

#include <stdbool.h>

typedef struct GridSettings {
    bool enabled;
    bool show_grid;
    float grid_size;        // e.g. 1.0
    float rotation_increment; // degrees, e.g. 15.0
    float scale_increment;    // e.g. 0.25
    
    // Adaptive rendering
    bool adaptive_grid;
    float adaptive_scale;
} GridSettings;

// System management
void grid_snapping_init(void);
GridSettings* grid_snapping_get_settings(void);

// Snapping logic
float grid_snap_value(float val, float increment);
void grid_snap_position(float *pos_x, float *pos_y, float *pos_z);
void grid_snap_rotation(float *rot_x, float *rot_y, float *rot_z);
void grid_snap_scale(float *scale_x, float *scale_y, float *scale_z);

// Rendering (called by editor renderer)
void grid_snapping_render(const float *camera_pos, const float *camera_view, const float *camera_proj);
