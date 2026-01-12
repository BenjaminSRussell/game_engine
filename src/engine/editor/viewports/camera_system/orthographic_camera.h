/**
 * @file orthographic_camera.h
 * @brief Orthographic camera system for 2D and UI rendering
 *
 * Provides orthographic projection for UI, 2D games, and technical rendering
 * Supports zoom, pan, and viewport management
 */

#ifndef RENDER_ORTHOGRAPHIC_CAMERA_H
#define RENDER_ORTHOGRAPHIC_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"

#ifdef __cplusplus
extern "C" {
#endif

// Orthographic camera structure
typedef struct {
    // Projection parameters
    float left, right, bottom, top;
    float near_plane, far_plane;
    
    // Camera transform
    Vec3 position;
    Vec3 target;
    Vec3 up;
    
    // Viewport
    Vec2 viewport_origin;
    Vec2 viewport_size;
    
    // Zoom and pan
    float zoom_level;
    Vec2 pan_offset;
    
    // Camera matrices
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection_matrix;
    
    // Configuration
    bool auto_resize;
    bool maintain_aspect_ratio;
    float aspect_ratio;
    
    // Performance tracking
    uint32_t frame_count;
    float last_update_time;
} OrthographicCamera;

// Core camera functions
int orthographic_camera_create(OrthographicCamera* camera, float left, float right, float bottom, float top, float near_plane, float far_plane);
int orthographic_camera_destroy(OrthographicCamera* camera);

// View and projection management
int orthographic_camera_set_projection(OrthographicCamera* camera, float left, float right, float bottom, float top, float near_plane, float far_plane);
int orthographic_camera_set_view(OrthographicCamera* camera, Vec3 position, Vec3 target, Vec3 up);
int orthographic_camera_set_viewport(OrthographicCamera* camera, Vec2 origin, Vec2 size);

// Zoom and pan controls
int orthographic_camera_set_zoom(OrthographicCamera* camera, float zoom_level);
int orthographic_camera_zoom_in(OrthographicCamera* camera, float zoom_factor);
int orthographic_camera_zoom_out(OrthographicCamera* camera, float zoom_factor);
int orthographic_camera_pan(OrthographicCamera* camera, Vec2 delta);
int orthographic_camera_center(OrthographicCamera* camera);

// Matrix computation
int orthographic_camera_update_matrices(OrthographicCamera* camera);
int orthographic_camera_get_view_matrix(const OrthographicCamera* camera, Mat4* out_matrix);
int orthographic_camera_get_projection_matrix(const OrthographicCamera* camera, Mat4* out_matrix);
int orthographic_camera_get_view_projection_matrix(const OrthographicCamera* // Get combined view-projection matrix
camera, Mat4* out_matrix);

// Viewport and screen conversion
int orthographic_camera_world_to_screen(const OrthographicCamera* camera, Vec3 world_pos, Vec2* out_screen_pos);
int orthographic_camera_screen_to_world(const OrthographicCamera* camera, Vec2 screen_pos, Vec3* out_world_pos);
int orthographic_camera_ray_from_screen(const OrthographicCamera* camera, Vec2 screen_pos, Vec3* out_origin, Vec3* out_direction);

// Utility functions
int orthographic_camera_get_bounds(const OrthographicCamera* camera, Vec2* out_min, Vec2* out_max);
int orthographic_camera_get_center(const OrthographicCamera* camera, Vec2* out_center);
float orthographic_camera_get_width(const OrthographicCamera* camera);
float orthographic_camera_get_height(const OrthographicCamera* camera);
float orthographic_get_aspect_ratio(const OrthographicCamera* camera);

// Configuration functions
int orthographic_camera_set_auto_resize(OrthographicCamera* camera, bool enabled);
int orthographic_camera_set_maintain_aspect_ratio(OrthographicCamera* camera, bool enabled, float aspect_ratio);
int orthographic_camera_set_background_color(OrthographicCamera* camera, Vec4 color);

// Animation and interpolation
int orthographic_camera_animate_to(OrthographicCamera* camera, Vec3 target_position, Vec3 target_target, float duration);
int orthographic_camera_animate_zoom(OrthographicCamera* camera, float target_zoom, float duration);
int orthographic_camera_animate_pan(OrthographicCamera* camera, Vec2 target_pan, float duration);

// Validation and debugging
bool orthographic_camera_is_valid(const OrthographicCamera* camera);
int orthographic_camera_print_info(const OrthographicCamera* camera);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_ORTHOGRAPHIC_CAMERA_H */
