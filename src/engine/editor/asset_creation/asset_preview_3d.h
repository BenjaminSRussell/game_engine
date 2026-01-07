#pragma once

#include <stdbool.h>

typedef struct AssetPreviewContext {
    float camera_distance;
    float camera_pitch;
    float camera_yaw;
    float auto_rotate_speed;
    bool transparent_background;
    
    // Lighting settings
    float light_intensity;
    float light_dir[3];
    
    // Viewport size
    int width;
    int height;
} AssetPreviewContext;

void asset_preview_init(void);
AssetPreviewContext* asset_preview_get_context(void);

// Configure preview based on asset bounds
void asset_preview_center_camera(const float *bbox_min, const float *bbox_max);

// Rendering (stubbed integration)
void asset_preview_render_scene(void *asset_ptr);
void asset_preview_resize(int width, int height);

// Update simulation (rotation)
void asset_preview_update(float delta_time);
