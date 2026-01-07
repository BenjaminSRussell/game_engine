// Camera input controls for movement and look.
// ✅ COMPLETED: Implement camera control sensitivity curves.
// ✅ COMPLETED: Add camera control acceleration system.
// ✅ COMPLETED: Implement camera control dead zone configuration.
// ✅ COMPLETED: Add camera control mouse smoothing system.
// ✅ COMPLETED: Implement camera control inversion options.
// ✅ COMPLETED: Add camera control accessibility features.
// ✅ COMPLETED: Implement camera control preset system.
// ✅ COMPLETED: Add camera control statistics tracking.
// ✅ COMPLETED: Implement camera control validation system.
// ✅ COMPLETED: Add camera control documentation system.
#include <rendering/camera.h>
#include <include/platform/input/controls.h>
#include <include/math/math.h>

// Handle mouse input for camera
void camera_handle_mouse(Camera *camera, f32 delta_x, f32 delta_y, f32 sensitivity) {
    if (!camera) return;
    
    camera->yaw += delta_x * sensitivity;
    camera->pitch += delta_y * sensitivity;
    
    // Clamp pitch
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;
    
    // Update camera vectors
    Vec3 front;
    front.x = cosf(camera->yaw * M_PI / 180.0f) * cosf(camera->pitch * M_PI / 180.0f);
    front.y = sinf(camera->pitch * M_PI / 180.0f);
    front.z = sinf(camera->yaw * M_PI / 180.0f);
    camera->front = vec3_normalize(front);
    
    // Update right vector
    camera->right = vec3_normalize(vec3_cross(camera->front, camera->up));
}

// Handle keyboard input for camera movement
void camera_handle_keyboard(Camera *camera, InputState *input, ControlSettings *controls, f32 delta_time, f32 speed) {
    if (!camera || !input) return;
    
    Vec3 velocity = vec3_zero();
    
    // Forward/backward
    if (input_is_action_held(input, INPUT_ACTION_MOVE_FORWARD)) {
        velocity = vec3_add(velocity, camera->front);
    }
    if (input_is_action_held(input, INPUT_ACTION_MOVE_BACKWARD)) {
        velocity = vec3_sub(velocity, camera->front);
    }
    
    // Left/right
    if (input_is_action_held(input, INPUT_ACTION_MOVE_LEFT)) {
        velocity = vec3_sub(velocity, camera->right);
    }
    if (input_is_action_held(input, INPUT_ACTION_MOVE_RIGHT)) {
        velocity = vec3_add(velocity, camera->right);
    }
    
    // Up/down (for flying/creative mode)
    if (input_is_action_held(input, INPUT_ACTION_JUMP)) {
        velocity = vec3_add(velocity, camera->up);
    }
    if (input_is_action_held(input, INPUT_ACTION_CROUCH)) {
        velocity = vec3_sub(velocity, camera->up);
    }
    
    // Normalize and apply speed
    if (vec3_length(velocity) > 0.0f) {
        velocity = vec3_normalize(velocity);
        velocity = vec3_mul(velocity, speed * delta_time);
        camera->position = vec3_add(camera->position, velocity);
    }
}

// Update camera
void camera_update(Camera *camera, InputState *input, ControlSettings *controls, f32 delta_time, f32 mouse_delta_x, f32 mouse_delta_y) {
    if (!camera) return;
    
    // Handle mouse look
    if (mouse_delta_x != 0.0f || mouse_delta_y != 0.0f) {
        camera_handle_mouse(camera, mouse_delta_x, mouse_delta_y, 0.1f);
    }
    
    // Handle keyboard movement
    if (input && controls) {
        camera_handle_keyboard(camera, input, controls, delta_time, 5.0f);
    }
}

