// include/render/camera_controller.h
//
// Purpose: Defines the abstract CameraController interface that decouples camera
// logic from specific camera implementations. This allows different camera modes:
// - FreeCameraController: Free-look camera for voxel games
// - FixedAngleCameraController: Fixed-angle follow camera for 2.5D games
// - OrthographicCameraController: Orthographic projection for 2.5D
//
// Public APIs:
// - `ICameraController`: Abstract camera controller interface
// - `CameraMode`: Enumeration for different camera modes
// - Concrete implementations for different game styles
//
// Ownership: Camera controllers manage their own state but operate on Camera
// objects passed to them.
//
// Invariants:
// - All function pointers must be non-NULL after initialization
// - Camera must be initialized before use
//
#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <common.h>
#include <math/vec3.h>
#include <math/mat4.h>
#include "camera.h"

// Forward declarations
struct PlayerSystem;

// Camera mode enumeration
typedef enum {
    CAMERA_MODE_FREE,           // Free-look camera (voxel games)
    CAMERA_MODE_FIXED_ANGLE,    // Fixed-angle follow camera (2.5D games)
    CAMERA_MODE_ORTHOGRAPHIC,   // Orthographic projection (2.5D top-down)
    CAMERA_MODE_FIRST_PERSON,   // First-person camera
    CAMERA_MODE_THIRD_PERSON,   // Third-person follow camera
    CAMERA_MODE_COUNT
} CameraMode;

// Camera controller initialization parameters
typedef struct {
    CameraMode mode;
    Vec3 initial_position;
    f32 initial_yaw;
    f32 initial_pitch;
    f32 fov;
    f32 near_plane;
    f32 far_plane;
    
    // Fixed-angle camera parameters
    f32 fixed_angle;        // Angle in degrees (e.g., 45 for isometric)
    f32 follow_distance;   // Distance from target
    f32 follow_height;      // Height offset from target
    
    // Orthographic camera parameters
    f32 ortho_size;         // Size of orthographic view
} CameraControllerParams;

// Abstract camera controller interface
typedef struct ICameraController {
    // Camera mode
    CameraMode mode;
    
    // Lifecycle
    void (*init)(struct ICameraController *self, CameraControllerParams *params);
    void (*cleanup)(struct ICameraController *self);
    
    // Update camera based on input/target
    void (*update)(struct ICameraController *self, Camera *camera, f32 delta_time);
    
    // Input handling
    void (*handle_mouse_move)(struct ICameraController *self, Camera *camera, f32 dx, f32 dy);
    void (*handle_mouse_scroll)(struct ICameraController *self, Camera *camera, f32 delta);
    void (*handle_key)(struct ICameraController *self, Camera *camera, i32 key, bool pressed);
    
    // Target following (for follow cameras)
    void (*set_target)(struct ICameraController *self, Vec3 target_position);
    void (*set_target_entity)(struct ICameraController *self, EntityID entity);
    void (*set_target_player)(struct ICameraController *self, struct PlayerSystem *player);
    
    // Camera positioning
    void (*set_position)(struct ICameraController *self, Camera *camera, Vec3 position);
    void (*set_rotation)(struct ICameraController *self, Camera *camera, f32 yaw, f32 pitch);
    
    // Projection matrix generation
    Mat4 (*get_projection_matrix)(struct ICameraController *self, Camera *camera, f32 aspect);
    Mat4 (*get_view_matrix)(struct ICameraController *self, Camera *camera);
    
    // Camera shake (for effects)
    void (*add_shake)(struct ICameraController *self, Camera *camera, f32 intensity, f32 duration);
    
    // Internal implementation data
    void *impl_data;
} ICameraController;

// Factory function to create camera controller
ICameraController *camera_controller_create(CameraMode mode);
void camera_controller_destroy(ICameraController *controller);

// Convenience macros
#define CAMERA_CONTROLLER_INIT(ctrl, params) ((ctrl)->init((ctrl), (params)))
#define CAMERA_CONTROLLER_UPDATE(ctrl, cam, dt) ((ctrl)->update((ctrl), (cam), (dt)))
#define CAMERA_CONTROLLER_SET_TARGET(ctrl, pos) ((ctrl)->set_target((ctrl), (pos)))
#define CAMERA_CONTROLLER_GET_PROJECTION(ctrl, cam, aspect) ((ctrl)->get_projection_matrix((ctrl), (cam), (aspect)))
#define CAMERA_CONTROLLER_GET_VIEW(ctrl, cam) ((ctrl)->get_view_matrix((ctrl), (cam)))

#endif // CAMERA_CONTROLLER_H

