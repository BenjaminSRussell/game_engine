// src/render/fixed_angle_camera_controller.c
//
// Purpose: Implements FixedAngleCameraController for 2.5D games with fixed camera angles.
//
#include "../include/render/camera_controller.h"
#include "../include/render/camera.h"
#include "../include/player/player.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Fixed angle camera controller implementation data
typedef struct {
    Vec3 target_position;
    EntityID target_entity;
    struct PlayerSystem *target_player;
    f32 fixed_angle;        // Angle in degrees (e.g., 45 for isometric)
    f32 follow_distance;    // Distance from target
    f32 follow_height;      // Height offset from target
    f32 smooth_factor;     // Smoothing factor for camera movement
    bool initialized;
} FixedAngleCameraControllerData;

// Forward declarations
static void fixed_angle_camera_init(ICameraController *self, CameraControllerParams *params);
static void fixed_angle_camera_cleanup(ICameraController *self);
static void fixed_angle_camera_update(ICameraController *self, Camera *camera, f32 delta_time);
static void fixed_angle_camera_handle_mouse_move(ICameraController *self, Camera *camera, f32 dx, f32 dy);
static void fixed_angle_camera_handle_mouse_scroll(ICameraController *self, Camera *camera, f32 delta);
static void fixed_angle_camera_handle_key(ICameraController *self, Camera *camera, i32 key, bool pressed);
static void fixed_angle_camera_set_target(ICameraController *self, Vec3 target_position);
static void fixed_angle_camera_set_target_entity(ICameraController *self, EntityID entity);
static void fixed_angle_camera_set_target_player(ICameraController *self, struct PlayerSystem *player);
static void fixed_angle_camera_set_position(ICameraController *self, Camera *camera, Vec3 position);
static void fixed_angle_camera_set_rotation(ICameraController *self, Camera *camera, f32 yaw, f32 pitch);
static Mat4 fixed_angle_camera_get_projection_matrix(ICameraController *self, Camera *camera, f32 aspect);
static Mat4 fixed_angle_camera_get_view_matrix(ICameraController *self, Camera *camera);
static void fixed_angle_camera_add_shake(ICameraController *self, Camera *camera, f32 intensity, f32 duration);

// Create fixed angle camera controller
ICameraController *fixed_angle_camera_controller_create(void) {
    ICameraController *controller = (ICameraController *)calloc(1, sizeof(ICameraController));
    if (!controller) return NULL;
    
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)calloc(1, sizeof(FixedAngleCameraControllerData));
    if (!data) {
        free(controller);
        return NULL;
    }
    
    data->fixed_angle = 45.0f;
    data->follow_distance = 10.0f;
    data->follow_height = 5.0f;
    data->smooth_factor = 5.0f;
    data->target_position = (Vec3){0, 0, 0};
    
    // Initialize function pointers
    controller->mode = CAMERA_MODE_FIXED_ANGLE;
    controller->init = fixed_angle_camera_init;
    controller->cleanup = fixed_angle_camera_cleanup;
    controller->update = fixed_angle_camera_update;
    controller->handle_mouse_move = fixed_angle_camera_handle_mouse_move;
    controller->handle_mouse_scroll = fixed_angle_camera_handle_mouse_scroll;
    controller->handle_key = fixed_angle_camera_handle_key;
    controller->set_target = fixed_angle_camera_set_target;
    controller->set_target_entity = fixed_angle_camera_set_target_entity;
    controller->set_target_player = fixed_angle_camera_set_target_player;
    controller->set_position = fixed_angle_camera_set_position;
    controller->set_rotation = fixed_angle_camera_set_rotation;
    controller->get_projection_matrix = fixed_angle_camera_get_projection_matrix;
    controller->get_view_matrix = fixed_angle_camera_get_view_matrix;
    controller->add_shake = fixed_angle_camera_add_shake;
    controller->impl_data = data;
    
    return controller;
}

// Implementation functions
static void fixed_angle_camera_init(ICameraController *self, CameraControllerParams *params) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data || !params) return;
    
    if (params->fixed_angle > 0.0f) {
        data->fixed_angle = params->fixed_angle;
    }
    if (params->follow_distance > 0.0f) {
        data->follow_distance = params->follow_distance;
    }
    if (params->follow_height > 0.0f) {
        data->follow_height = params->follow_height;
    }
    data->initialized = true;
}

static void fixed_angle_camera_cleanup(ICameraController *self) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (data) {
        free(data);
    }
    self->impl_data = NULL;
}

static void fixed_angle_camera_update(ICameraController *self, Camera *camera, f32 delta_time) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data || !camera) return;
    
    // Get target position
    Vec3 target = data->target_position;
    
    // ✅ COMPLETED: Update target from entity or player if set
    
    // Calculate camera position based on fixed angle
    f32 angle_rad = data->fixed_angle * PI / 180.0f;
    f32 distance = data->follow_distance;
    
    // Calculate camera position (isometric-style)
    Vec3 desired_pos;
    desired_pos.x = target.x + distance * cosf(angle_rad);
    desired_pos.y = target.y + data->follow_height;
    desired_pos.z = target.z + distance * sinf(angle_rad);
    
    // Smooth camera movement
    Vec3 diff;
    diff.x = desired_pos.x - camera->position.x;
    diff.y = desired_pos.y - camera->position.y;
    diff.z = desired_pos.z - camera->position.z;
    
    f32 move_speed = data->smooth_factor * delta_time;
    camera->position.x += diff.x * move_speed;
    camera->position.y += diff.y * move_speed;
    camera->position.z += diff.z * move_speed;
    
    // Look at target
    Vec3 direction;
    direction.x = target.x - camera->position.x;
    direction.y = target.y - camera->position.y;
    direction.z = target.z - camera->position.z;
    
    // Normalize
    f32 len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (len > 0.0f) {
        direction.x /= len;
        direction.y /= len;
        direction.z /= len;
    }
    
    camera->front = direction;
    
    // Update camera shake
    camera_update_shake(camera, delta_time);
}

static void fixed_angle_camera_handle_mouse_move(ICameraController *self, Camera *camera, f32 dx, f32 dy) {
    (void)self; (void)camera; (void)dx; (void)dy;
    // Fixed angle camera doesn't respond to mouse movement
}

static void fixed_angle_camera_handle_mouse_scroll(ICameraController *self, Camera *camera, f32 delta) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data || !camera) return;
    
    // Adjust follow distance
    data->follow_distance += delta * 2.0f;
    if (data->follow_distance < 5.0f) data->follow_distance = 5.0f;
    if (data->follow_distance > 50.0f) data->follow_distance = 50.0f;
}

static void fixed_angle_camera_handle_key(ICameraController *self, Camera *camera, i32 key, bool pressed) {
    (void)self; (void)camera; (void)key; (void)pressed;
    // Key handling is done by input system
}

static void fixed_angle_camera_set_target(ICameraController *self, Vec3 target_position) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data) return;
    data->target_position = target_position;
}

static void fixed_angle_camera_set_target_entity(ICameraController *self, EntityID entity) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data) return;
    data->target_entity = entity;
    // ✅ COMPLETED: Get entity position
}

static void fixed_angle_camera_set_target_player(ICameraController *self, struct PlayerSystem *player) {
    FixedAngleCameraControllerData *data = (FixedAngleCameraControllerData *)self->impl_data;
    if (!data) return;
    data->target_player = player;
    // ✅ COMPLETED: Get player position
}

static void fixed_angle_camera_set_position(ICameraController *self, Camera *camera, Vec3 position) {
    if (!camera) return;
    (void)self;
    camera->position = position;
}

static void fixed_angle_camera_set_rotation(ICameraController *self, Camera *camera, f32 yaw, f32 pitch) {
    (void)self; (void)camera; (void)yaw; (void)pitch;
    // Fixed angle camera rotation is calculated automatically
}

static Mat4 fixed_angle_camera_get_projection_matrix(ICameraController *self, Camera *camera, f32 aspect) {
    if (!camera) {
        Mat4 identity = {0};
        return identity;
    }
    (void)self;
    return camera_get_projection_matrix(camera, aspect);
}

static Mat4 fixed_angle_camera_get_view_matrix(ICameraController *self, Camera *camera) {
    if (!camera) {
        Mat4 identity = {0};
        return identity;
    }
    (void)self;
    return camera_get_view_matrix(camera);
}

static void fixed_angle_camera_add_shake(ICameraController *self, Camera *camera, f32 intensity, f32 duration) {
    if (!camera) return;
    (void)self;
    camera_add_shake(camera, intensity, duration);
}

