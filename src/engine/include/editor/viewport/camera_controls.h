// include/editor/viewport/camera_controls.h
//
// Purpose: Editor camera controls system
// Provides Maya-style orbit, pan, zoom, and fly camera modes
//
#ifndef EDITOR_VIEWPORT_CAMERA_CONTROLS_H
#define EDITOR_VIEWPORT_CAMERA_CONTROLS_H

#include "include/common.h"
#include "include/math/math.h"
#include "include/platform/input/input.h"

// Forward declarations
typedef struct Camera Camera;

// Camera control modes
typedef enum {
    CAMERA_MODE_ORBIT,      // Orbit around target
    CAMERA_MODE_FLY,        // FPS-style fly camera
    CAMERA_MODE_PAN,        // 2D pan mode
    CAMERA_MODE_DOLLY,      // Dolly zoom
    CAMERA_MODE_COUNT
} CameraMode;

// Camera control state
typedef enum {
    CAMERA_STATE_IDLE,
    CAMERA_STATE_ORBITING,
    CAMERA_STATE_PANNING,
    CAMERA_STATE_ZOOMING,
    CAMERA_STATE_FLYING
} CameraState;

// Maya-style controls
typedef enum {
    CAMERA_BUTTON_NONE,
    CAMERA_BUTTON_LEFT,     // Orbit
    CAMERA_BUTTON_MIDDLE,   // Pan
    CAMERA_BUTTON_RIGHT,    // Zoom/Dolly
    CAMERA_BUTTON_ALT_LEFT, // Alt + Left = Orbit
    CAMERA_BUTTON_ALT_MIDDLE,// Alt + Middle = Pan
    CAMERA_BUTTON_ALT_RIGHT // Alt + Right = Zoom
} CameraButton;

// Camera configuration
typedef struct {
    CameraMode mode;
    f32 orbit_distance;          // Distance from orbit target
    f32 min_distance;            // Minimum orbit distance
    f32 max_distance;            // Maximum orbit distance
    f32 pan_speed;               // Pan movement speed
    f32 zoom_speed;              // Zoom/dolly speed
    f32 orbit_speed;             // Orbit rotation speed
    f32 fly_speed;               // Fly camera movement speed
    f32 fov_speed;               // FOV zoom speed
    f32 smoothing_factor;        // Movement smoothing (0-1)
    bool enable_smoothing;        // Enable spring-damper smoothing
    bool invert_y;               // Invert Y axis for orbit
    Vec3 target;                 // Orbit target point
    Vec2 last_mouse_pos;         // Last mouse position for delta calculation
} CameraControlsConfig;

// Camera controls system
typedef struct {
    CameraControlsConfig config;
    CameraState state;
    CameraButton active_button;
    bool alt_pressed;
    bool shift_pressed;
    bool ctrl_pressed;
    
    // Smoothing state
    Vec3 smooth_position;
    Vec3 smooth_target;
    f32 smooth_distance;
    Vec3 smooth_velocity;
    
    // Input blocking
    bool input_blocked;
    
    // Safe frames
    bool show_safe_frames;
    f32 safe_frame_aspect;       // Target aspect ratio for safe frames
} CameraControls;

// Public API
CameraControls* camera_controls_create(Camera *camera);
void camera_controls_destroy(CameraControls *controls);

// Configuration
void camera_controls_set_mode(CameraControls *controls, CameraMode mode);
void camera_controls_set_target(CameraControls *controls, Vec3 target);
void camera_controls_set_distance(CameraControls *controls, f32 distance);
void camera_controls_set_speed(CameraControls *controls, f32 speed);
void camera_controls_enable_smoothing(CameraControls *controls, bool enable);
void camera_controls_set_safe_frames(CameraControls *controls, bool show, f32 aspect);

// Input handling
bool camera_controls_handle_key_down(CameraControls *controls, KeyCode key, bool shift, bool ctrl, bool alt);
bool camera_controls_handle_key_up(CameraControls *controls, KeyCode key, bool shift, bool ctrl, bool alt);
bool camera_controls_handle_mouse_down(CameraControls *controls, Vec2 mouse_pos, MouseButton button, bool shift, bool ctrl, bool alt);
bool camera_controls_handle_mouse_up(CameraControls *controls, Vec2 mouse_pos, MouseButton button);
bool camera_controls_handle_mouse_move(CameraControls *controls, Vec2 mouse_pos);
bool camera_controls_handle_mouse_wheel(CameraControls *controls, f32 wheel_delta, bool shift, bool ctrl);

// Camera operations
void camera_controls_focus_selected(CameraControls *controls, const Vec3 *selection_bounds, u32 count);
void camera_controls_frame_all(CameraControls *controls, const Vec3 *world_bounds);
void camera_controls_reset(CameraControls *controls);

// Update
void camera_controls_update(CameraControls *controls, Camera *camera, f32 dt);

// Utility
bool camera_controls_is_input_blocked(const CameraControls *controls);
void camera_controls_block_input(CameraControls *controls, bool block);
CameraMode camera_controls_get_mode(const CameraControls *controls);
bool camera_controls_is_orbiting(const CameraControls *controls);
bool camera_controls_is_panning(const CameraControls *controls);
bool camera_controls_is_zooming(const CameraControls *controls);

#endif // EDITOR_VIEWPORT_CAMERA_CONTROLS_H
