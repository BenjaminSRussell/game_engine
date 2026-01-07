#include "rendering/cinematic/camera_director.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PATHS 32
#define MAX_KEYFRAMES 128

typedef struct {
  uint64_t id;
  char name[64];
  bool active;
  CameraKeyframe keyframes[MAX_KEYFRAMES];
  uint32_t keyframe_count;
  bool loop;
} CameraPath;

static struct {
  CameraPath paths[MAX_PATHS];
  uint32_t path_count;
  uint64_t next_id;

  bool playing;
  uint64_t current_path_id;
  float playback_time;

  float fov;
  float focus_distance;
} director_state;

void camera_director_init(void) {
  memset(&director_state, 0, sizeof(director_state));
  director_state.next_id = 1;
  director_state.fov = 60.0f;
  director_state.focus_distance = 10.0f;
  LOG_INFO("Camera Director Initialized");
}

void camera_director_shutdown(void) { LOG_INFO("Camera Director Shutdown"); }

void camera_director_update(float delta_time) {
  if (!director_state.playing)
    return;

  director_state.playback_time += delta_time;

  // Find current path
  CameraPath *path = NULL;
  for (uint32_t i = 0; i < MAX_PATHS; i++) {
    if (director_state.paths[i].active &&
        director_state.paths[i].id == director_state.current_path_id) {
      path = &director_state.paths[i];
      break;
    }
  }

  if (!path || path->keyframe_count == 0)
    return;

  // Simple looping logic
  float max_time = path->keyframes[path->keyframe_count - 1].time_seconds;
  if (director_state.playback_time > max_time) {
    if (path->loop) {
      director_state.playback_time = 0.0f;
    } else {
      director_state.playing = false;
    }
  }
}

static CameraPath *find_path(uint64_t id) {
  for (uint32_t i = 0; i < MAX_PATHS; i++) {
    if (director_state.paths[i].active && director_state.paths[i].id == id) {
      return &director_state.paths[i];
    }
  }
  return NULL;
}

uint64_t camera_director_create_path(const char *path_name) {
  for (uint32_t i = 0; i < MAX_PATHS; i++) {
    if (!director_state.paths[i].active) {
      uint64_t id = director_state.next_id++;
      director_state.paths[i].id = id;
      director_state.paths[i].active = true;
      director_state.paths[i].keyframe_count = 0;
      director_state.paths[i].loop = false;
      strncpy(director_state.paths[i].name, path_name, 63);
      director_state.paths[i].name[63] = '\0';
      LOG_INFO("Created camera path '%s' (ID: %llu)", path_name, id);
      return id;
    }
  }
  LOG_ERROR("Max camera paths reached");
  return 0;
}

void camera_director_delete_path(uint64_t path_id) {
  CameraPath *path = find_path(path_id);
  if (path) {
    path->active = false;
    LOG_INFO("Deleted camera path %llu", path_id);
  }
}

void camera_director_add_keyframe(uint64_t path_id, float time_seconds,
                                  float pos_x, float pos_y, float pos_z,
                                  float look_x, float look_y, float look_z) {
  CameraPath *path = find_path(path_id);
  if (!path)
    return;

  if (path->keyframe_count >= MAX_KEYFRAMES) {
    LOG_WARN("Max keyframes reached for path %llu", path_id);
    return;
  }

  CameraKeyframe *kf = &path->keyframes[path->keyframe_count++];
  kf->time_seconds = time_seconds;
  kf->pos_x = pos_x;
  kf->pos_y = pos_y;
  kf->pos_z = pos_z;
  kf->look_x = look_x;
  kf->look_y = look_y;
  kf->look_z = look_z;

  LOG_INFO("Added keyframe to path %llu at time %.2fs", path_id, time_seconds);
}

void camera_director_clear_keyframes(uint64_t path_id) {
  CameraPath *path = find_path(path_id);
  if (path) {
    path->keyframe_count = 0;
  }
}

void camera_director_play_path(uint64_t path_id, bool loop) {
  CameraPath *path = find_path(path_id);
  if (!path)
    return;

  director_state.current_path_id = path_id;
  director_state.playing = true;
  director_state.playback_time = 0.0f;
  path->loop = loop;
  LOG_INFO("Playing camera path %llu (loop: %d)", path_id, loop);
}

void camera_director_pause(void) { director_state.playing = false; }

void camera_director_resume(void) { director_state.playing = true; }

void camera_director_stop(void) {
  director_state.playing = false;
  director_state.playback_time = 0.0f;
}

void camera_director_set_fov(float fov_degrees) {
  director_state.fov = fov_degrees;
}

float camera_director_get_fov(void) { return director_state.fov; }

void camera_director_set_focus_distance(float distance) {
  director_state.focus_distance = distance;
}

float camera_director_get_focus_distance(void) {
  return director_state.focus_distance;
}

bool camera_director_is_playing(void) { return director_state.playing; }

float camera_director_get_playback_time(void) {
  return director_state.playback_time;
}
