#include "platform/camera_director_api_bridge.h"
#include "core/logger.h"
#include "rendering/cinematic/camera_director.h"

uint64_t camera_director_create_path_api(const char *path_name) {
  return camera_director_create_path(path_name);
}

void camera_director_delete_path_api(uint64_t path_id) {
  camera_director_delete_path(path_id);
}

void camera_director_add_keyframe_api(uint64_t path_id, float time_seconds,
                                      float pos_x, float pos_y, float pos_z,
                                      float look_x, float look_y,
                                      float look_z) {
  camera_director_add_keyframe(path_id, time_seconds, pos_x, pos_y, pos_z,
                               look_x, look_y, look_z);
}

void camera_director_clear_keyframes_api(uint64_t path_id) {
  camera_director_clear_keyframes(path_id);
}

void camera_director_play_path_api(uint64_t path_id, bool loop) {
  camera_director_play_path(path_id, loop);
}

void camera_director_pause_api(void) { camera_director_pause(); }

void camera_director_resume_api(void) { camera_director_resume(); }

void camera_director_stop_api(void) { camera_director_stop(); }

void camera_director_set_fov_api(float fov_degrees) {
  camera_director_set_fov(fov_degrees);
}

float camera_director_get_fov_api(void) { return camera_director_get_fov(); }

void camera_director_set_focus_distance_api(float distance) {
  camera_director_set_focus_distance(distance);
}

float camera_director_get_focus_distance_api(void) {
  return camera_director_get_focus_distance();
}

bool camera_director_is_playing_api(void) {
  return camera_director_is_playing();
}

float camera_director_get_playback_time_api(void) {
  return camera_director_get_playback_time();
}
