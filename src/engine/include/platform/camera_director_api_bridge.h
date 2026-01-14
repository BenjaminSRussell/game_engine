// Camera Director API Bridge
// Exposes cinematic camera control to VoxelForgeStudio

#ifndef CAMERA_DIRECTOR_API_BRIDGE_H
#define CAMERA_DIRECTOR_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Camera Director API
// ============================================================================

/// Create a camera path
/// @param name Path name
/// @return Path ID
uint64_t camera_path_create(const char *name);

/// Destroy a camera path
void camera_path_destroy(uint64_t path_id);

/// Add keyframe to path
void camera_path_add_keyframe(uint64_t path_id, float time, float pos_x,
                              float pos_y, float pos_z, float look_x,
                              float look_y, float look_z);

/// Remove keyframe at time
void camera_path_remove_keyframe(uint64_t path_id, float time);

/// Play camera path
void camera_path_play(uint64_t path_id, bool loop);

/// Stop playback
void camera_path_stop(void);

/// Pause playback
void camera_path_pause(void);

/// Resume playback
void camera_path_resume(void);

/// Check if path is playing
bool camera_path_is_playing(void);

/// Set camera FOV
void camera_set_fov(float fov);

/// Get camera FOV
float camera_get_fov(void);

/// Set focus distance for depth of field
void camera_set_focus_distance(float distance);

/// Get focus distance
float camera_get_focus_distance(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_DIRECTOR_API_BRIDGE_H
