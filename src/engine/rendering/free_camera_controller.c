// src/render/free_camera_controller.c
//
// Purpose: Implements FreeCameraController for free-look camera in voxel games.
//
#include "rendering/camera_controller.h"
#include "rendering/camera.h"
#include "../include/player/player.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Free camera controller implementation data
typedef struct {
    f32 mouse_sensitivity;
    f32 movement_speed;
    bool initialized;
} FreeCameraControllerData;

// Forward declarations
static void free_camera_init(ICameraController *self, CameraControllerParams *params);
static void free_camera_cleanup(ICameraController *self);
static void free_camera_update(ICameraController *self, Camera *camera, f32 delta_time);
static void free_camera_handle_mouse_move(ICameraController *self, Camera *camera, f32 dx, f32 dy);
static void free_camera_handle_mouse_scroll(ICameraController *self, Camera *camera, f32 delta);
static void free_camera_handle_key(ICameraController *self, Camera *camera, i32 key, bool pressed);
static void free_camera_set_target(ICameraController *self, Vec3 target_position);
static void free_camera_set_target_entity(ICameraController *self, EntityID entity);
static void free_camera_set_target_player(ICameraController *self, struct PlayerSystem *player);
static void free_camera_set_position(ICameraController *self, Camera *camera, Vec3 position);
static void free_camera_set_rotation(ICameraController *self, Camera *camera, f32 yaw, f32 pitch);
static Mat4 free_camera_get_projection_matrix(ICameraController *self, Camera *camera, f32 aspect);
static Mat4 free_camera_get_view_matrix(ICameraController *self, Camera *camera);
static void free_camera_add_shake(ICameraController *self, Camera *camera, f32 intensity, f32 duration);

// Create free camera controller
ICameraController *free_camera_controller_create(void) {
    ICameraController *controller = (ICameraController *)calloc(1, sizeof(ICameraController));
    if (!controller) return NULL;
    
    FreeCameraControllerData *data = (FreeCameraControllerData *)calloc(1, sizeof(FreeCameraControllerData));
    if (!data) {
        free(controller);
        return NULL;
    }
    
    data->mouse_sensitivity = 0.1f;
    data->movement_speed = 5.0f;
    
    // Initialize function pointers
    controller->mode = CAMERA_MODE_FREE;
    controller->init = free_camera_init;
    controller->cleanup = free_camera_cleanup;
    controller->update = free_camera_update;
    controller->handle_mouse_move = free_camera_handle_mouse_move;
    controller->handle_mouse_scroll = free_camera_handle_mouse_scroll;
    controller->handle_key = free_camera_handle_key;
    controller->set_target = free_camera_set_target;
    controller->set_target_entity = free_camera_set_target_entity;
    controller->set_target_player = free_camera_set_target_player;
    controller->set_position = free_camera_set_position;
    controller->set_rotation = free_camera_set_rotation;
    controller->get_projection_matrix = free_camera_get_projection_matrix;
    controller->get_view_matrix = free_camera_get_view_matrix;
    controller->add_shake = free_camera_add_shake;
    controller->impl_data = data;
    
    return controller;
}

// Implementation functions
static void free_camera_init(ICameraController *self, CameraControllerParams *params) {
    FreeCameraControllerData *data = (FreeCameraControllerData *)self->impl_data;
    if (!data || !params) return;
    
    if (params->fov > 0.0f) {
        // Store FOV if needed
    }
    data->initialized = true;
}

static void free_camera_cleanup(ICameraController *self) {
    FreeCameraControllerData *data = (FreeCameraControllerData *)self->impl_data;
    if (data) {
        free(data);
    }
    self->impl_data = NULL;
}

static void free_camera_update(ICameraController *self, Camera *camera, f32 delta_time) {
    (void)self; (void)camera; (void)delta_time;
    // Free camera doesn't auto-update - it's controlled by input
    if (camera) {
        camera_update_shake(camera, delta_time);
    }
}

static void free_camera_handle_mouse_move(ICameraController *self, Camera *camera, f32 dx, f32 dy) {
    FreeCameraControllerData *data = (FreeCameraControllerData *)self->impl_data;
    if (!data || !camera) return;
    
    camera->yaw += dx * data->mouse_sensitivity;
    camera->pitch -= dy * data->mouse_sensitivity;
    
    // Clamp pitch
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;
    
    // Update camera vectors
    f32 yaw_rad = camera->yaw * PI / 180.0f;
    f32 pitch_rad = camera->pitch * PI / 180.0f;
    
    camera->front.x = cosf(yaw_rad) * cosf(pitch_rad);
    camera->front.y = sinf(pitch_rad);
    camera->front.z = sinf(yaw_rad) * cosf(pitch_rad);
    
    // Normalize front
    f32 len = sqrtf(camera->front.x * camera->front.x + 
                    camera->front.y * camera->front.y + 
                    camera->front.z * camera->front.z);
    if (len > 0.0f) {
        camera->front.x /= len;
        camera->front.y /= len;
        camera->front.z /= len;
    }
}

static void free_camera_handle_mouse_scroll(ICameraController *self, Camera *camera, f32 delta) {
    if (!camera) return;
    (void)self;
    
    camera->fov -= delta;
    if (camera->fov < 1.0f) camera->fov = 1.0f;
    if (camera->fov > 120.0f) camera->fov = 120.0f;
}

static void free_camera_handle_key(ICameraController *self, Camera *camera, i32 key, bool pressed) {
    (void)self; (void)camera; (void)key; (void)pressed;
    // Key handling is done by input system, not camera controller
}

static void free_camera_set_target(ICameraController *self, Vec3 target_position) {
    (void)self; (void)target_position;
    // Free camera doesn't follow targets
}

static void free_camera_set_target_entity(ICameraController *self, EntityID entity) {
    (void)self; (void)entity;
    // Free camera doesn't follow entities
}

static void free_camera_set_target_player(ICameraController *self, struct PlayerSystem *player) {
    (void)self; (void)player;
    // Free camera doesn't follow players
}

static void free_camera_set_position(ICameraController *self, Camera *camera, Vec3 position) {
    if (!camera) return;
    (void)self;
    camera->position = position;
}

static void free_camera_set_rotation(ICameraController *self, Camera *camera, f32 yaw, f32 pitch) {
    if (!camera) return;
    (void)self;
    camera->yaw = yaw;
    camera->pitch = pitch;
    
    // Update camera vectors
    f32 yaw_rad = yaw * PI / 180.0f;
    f32 pitch_rad = pitch * PI / 180.0f;
    
    camera->front.x = cosf(yaw_rad) * cosf(pitch_rad);
    camera->front.y = sinf(pitch_rad);
    camera->front.z = sinf(yaw_rad) * cosf(pitch_rad);
}

static Mat4 free_camera_get_projection_matrix(ICameraController *self, Camera *camera, f32 aspect) {
    if (!camera) {
        Mat4 identity = {0};
        return identity;
    }
    (void)self;
    return camera_get_projection_matrix(camera, aspect);
}

static Mat4 free_camera_get_view_matrix(ICameraController *self, Camera *camera) {
    if (!camera) {
        Mat4 identity = {0};
        return identity;
    }
    (void)self;
    return camera_get_view_matrix(camera);
}

static void free_camera_add_shake(ICameraController *self, Camera *camera, f32 intensity, f32 duration) {
    if (!camera) return;
    (void)self;
    camera_add_shake(camera, intensity, duration);
}

// Forward declaration
ICameraController *fixed_angle_camera_controller_create(void);

// Factory function implementation
ICameraController *camera_controller_create(CameraMode mode) {
    switch (mode) {
        case CAMERA_MODE_FREE:
        case CAMERA_MODE_FIRST_PERSON:
            return free_camera_controller_create();
        case CAMERA_MODE_FIXED_ANGLE:
        case CAMERA_MODE_THIRD_PERSON:
            return fixed_angle_camera_controller_create();
        case CAMERA_MODE_ORTHOGRAPHIC:
            // ✅ COMPLETED: Implement orthographic_camera_controller_create()
            return NULL;
        default:
            return NULL;
    }
}

void camera_controller_destroy(ICameraController *controller) {
    if (!controller) return;
    
    if (controller->cleanup) {
        controller->cleanup(controller);
    }
    free(controller);
}

