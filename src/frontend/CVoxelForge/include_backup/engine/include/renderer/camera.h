// include/render/camera.h
//
// Purpose: Defines the `Camera` structure and provides the API for controlling
// the viewer's perspective in the 3D world. This header is fundamental for
// rendering, as it enables the generation of view and projection matrices
// that transform world coordinates into screen coordinates.
//
// Public APIs:
// - `Camera`: A structure representing the camera's state, including its
//   `position` in 3D space, `front`, `up`, and `right` vectors for orientation,
//   `yaw` and `pitch` for rotational angles, and projection parameters (`fov`,
//   `near_plane`, `far_plane`).
// - `camera_init`: Initializes a `Camera` instance at a given position and
//   with initial yaw and pitch angles.
// - `camera_get_view_matrix`: Computes and returns the view matrix based on the
//   camera's current position and orientation.
// - `camera_get_projection_matrix`: Computes and returns the projection matrix
//   (typically a perspective projection) using the camera's FOV, aspect ratio,
//   near, and far planes.
// - `camera_get_forward`: Returns the camera's forward direction vector.
//
// Ownership: `Camera` instances are value types that hold their internal state.
// Functions typically operate on `Camera` objects passed by pointer, modifying
// their state or returning computed matrices/vectors.
//
// Invariants:
// - A `Camera` must be initialized with `camera_init` before its matrices
//   or vectors are computed.
// - `yaw` and `pitch` angles are typically in degrees, but usage might vary
//   depending on internal implementation (e.g., in `camera_get_view_matrix`).
// - `aspect` ratio should be correctly provided to
// `camera_get_projection_matrix`.
// - `Vec3` and `Mat4` structures (from `vec3.h` and `mat4.h`) are assumed to be
// correctly defined.
//
#ifndef CAMERA_H
#define CAMERA_H

#include <common.h>
#include <math/mat4.h>
#include <math/vec3.h>

// Camera structure (matches vulkan.h definition)
struct Camera {
  Vec3 position;
  Vec3 front;
  Vec3 up;
  Vec3 right;
  f32 yaw;
  f32 pitch;
  f32 fov;
  f32 near_plane;
  f32 far_plane;

  // Camera shake effect
  f32 shake_duration;
  f32 shake_intensity;
  f32 shake_time;
  Vec3 shake_offset;
};

typedef struct Camera Camera;

// Initialize camera
void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch);

// Set camera position
void camera_set_position(Camera *camera, Vec3 position);

// Get view matrix
Mat4 camera_get_view_matrix(Camera *camera);

// Get projection matrix
Mat4 camera_get_projection_matrix(Camera *camera, f32 aspect);

// Get forward vector
Vec3 camera_get_forward(Camera *camera);

// Camera shake functions
void camera_add_shake(Camera *camera, f32 intensity, f32 duration);
void camera_update_shake(Camera *camera, f32 delta_time);

#endif // CAMERA_H
