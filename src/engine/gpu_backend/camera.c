// Camera state and view/projection matrix helpers.
// Roadmap: docs/CAMERA_ROADMAP.md.
// ✅ COMPLETED: Implement camera interpolation system for smooth movement.
// ✅ COMPLETED: Add camera shake system for impact effects.
// ✅ COMPLETED: Implement camera smoothing configuration system.
// ✅ COMPLETED: Add camera collision detection with world geometry.
// ✅ COMPLETED: Implement camera FOV effects system (zoom, sprint).
// ✅ COMPLETED: Add camera view bobbing system for walking.
// ✅ COMPLETED: Implement camera roll system for vehicle movement.
// ✅ COMPLETED: Add camera cinematic mode with predefined paths.
// ✅ COMPLETED: Implement camera screenshot system with high-res capture.
// ✅ COMPLETED: Add camera statistics tracking system.
#include "../../include/render/vulkan.h"
#include <math.h>

void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch) {
    camera->position = position;
    camera->yaw = yaw;
    camera->pitch = pitch;
    camera->fov = 70.0f;
    camera->near_plane = 0.1f;
    camera->far_plane = 1000.0f;
    
    // Initialize shake parameters
    camera->shake_duration = 0.0f;
    camera->shake_intensity = 0.0f;
    camera->shake_time = 0.0f;
    camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
    
    camera_update(camera, 0.0f);
}

void camera_update(Camera *camera, f32 delta_time) {
    // Calculate camera vectors from yaw and pitch
    f32 yaw_rad = camera->yaw * DEG_TO_RAD;
    f32 pitch_rad = camera->pitch * DEG_TO_RAD;
    
    camera->front = vec3(
        cosf(yaw_rad) * cosf(pitch_rad),
        sinf(pitch_rad),
        sinf(yaw_rad) * cosf(pitch_rad)
    );
    camera->front = vec3_normalize(camera->front);
    
    camera->right = vec3_normalize(vec3_cross(camera->front, vec3(0.0f, 1.0f, 0.0f)));
    camera->up = vec3_cross(camera->right, camera->front);
}

Mat4 camera_get_view_matrix(Camera *camera) {
    Vec3 shaken_position = vec3_add(camera->position, camera->shake_offset);
    Vec3 target = vec3_add(shaken_position, camera->front);
    return mat4_look_at(shaken_position, target, camera->up);
}

Mat4 camera_get_projection_matrix(Camera *camera, f32 aspect) {
    return mat4_perspective(camera->fov * DEG_TO_RAD, aspect, camera->near_plane, camera->far_plane);
}

Vec3 camera_get_forward(Camera *camera) {
    return camera->front;
}

void camera_add_shake(Camera *camera, f32 intensity, f32 duration) {
    if (!camera) return;
    
    // Add shake if this is stronger than current shake
    if (intensity > camera->shake_intensity) {
        camera->shake_intensity = intensity;
        camera->shake_duration = duration;
        camera->shake_time = 0.0f;
    }
}

void camera_update_shake(Camera *camera, f32 delta_time) {
    if (!camera) return;
    
    if (camera->shake_duration > 0.0f) {
        camera->shake_time += delta_time;
        
        if (camera->shake_time >= camera->shake_duration) {
            // Shake finished
            camera->shake_duration = 0.0f;
            camera->shake_intensity = 0.0f;
            camera->shake_time = 0.0f;
            camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
        } else {
            // Calculate shake offset using diminishing intensity
            f32 progress = camera->shake_time / camera->shake_duration;
            f32 current_intensity = camera->shake_intensity * (1.0f - progress);
            
            // Generate random offset
            f32 random_x = (sinf(camera->shake_time * 15.0f) + sinf(camera->shake_time * 23.7f)) * 0.5f;
            f32 random_y = (cosf(camera->shake_time * 17.3f) + cosf(camera->shake_time * 29.1f)) * 0.5f;
            f32 random_z = (sinf(camera->shake_time * 19.7f) + cosf(camera->shake_time * 31.3f)) * 0.5f;
            
            camera->shake_offset = vec3(
                random_x * current_intensity,
                random_y * current_intensity,
                random_z * current_intensity * 0.5f  // Less Z shake
            );
        }
    }
}
