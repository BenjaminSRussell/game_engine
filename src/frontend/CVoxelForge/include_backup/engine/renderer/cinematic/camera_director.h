#ifndef CAMERA_DIRECTOR_H
#define CAMERA_DIRECTOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  float time_seconds;
  float pos_x, pos_y, pos_z;
  float look_x, look_y, look_z;
} CameraKeyframe;

void camera_director_init(void);
void camera_director_shutdown(void);
void camera_director_update(float delta_time);

// Path management
uint64_t camera_director_create_path(const char *path_name);
void camera_director_delete_path(uint64_t path_id);

// Keyframe management
void camera_director_add_keyframe(uint64_t path_id, float time_seconds,
                                  float pos_x, float pos_y, float pos_z,
                                  float look_x, float look_y, float look_z);
void camera_director_clear_keyframes(uint64_t path_id);

// Playback
void camera_director_play_path(uint64_t path_id, bool loop);
void camera_director_pause(void);
void camera_director_resume(void);
void camera_director_stop(void);

// Camera settings
void camera_director_set_fov(float fov_degrees);
float camera_director_get_fov(void);

void camera_director_set_focus_distance(float distance);
float camera_director_get_focus_distance(void);

// Status
bool camera_director_is_playing(void);
float camera_director_get_playback_time(void);

#endif // CAMERA_DIRECTOR_H
