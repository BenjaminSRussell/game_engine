// Grid Renderer Header
#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include "grid_settings.h"

// Initialize/shutdown
void grid_renderer_init(void);
void grid_renderer_shutdown(void);

// Settings management
void grid_renderer_set_settings(const GridSettings *settings);
void grid_renderer_get_settings(GridSettings *out_settings);

// Grid controls
void grid_renderer_toggle_grid(void);
void grid_renderer_set_grid_spacing(float spacing);

// Snap controls
void grid_renderer_toggle_position_snap(void);
void grid_renderer_toggle_rotation_snap(void);
void grid_renderer_toggle_scale_snap(void);

// Rendering
void grid_renderer_render(const float camera_position[3]);

// Snapping utilities
void grid_renderer_apply_position_snap(float position[3]);
void grid_renderer_apply_rotation_snap(float rotation[3]);
void grid_renderer_apply_scale_snap(float scale[3]);

#endif // GRID_RENDERER_H
