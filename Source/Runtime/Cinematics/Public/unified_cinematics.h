#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct Cutscene Cutscene;
typedef struct CameraPath CameraPath;
typedef struct Timeline Timeline;

// Cutscene system
typedef struct {
  float position[3];
  float look_at[3];
  float fov;
  float duration;
} CameraKeyframe;

Cutscene *cutscene_create(const char *name);
void cutscene_destroy(Cutscene *cutscene);

void cutscene_play(Cutscene *cutscene);
void cutscene_pause(Cutscene *cutscene);
void cutscene_stop(Cutscene *cutscene);
bool cutscene_is_playing(Cutscene *cutscene);

// Camera paths
CameraPath *camera_path_create(void);
void camera_path_add_keyframe(CameraPath *path, const CameraKeyframe *keyframe);
void camera_path_evaluate(CameraPath *path, float time,
                          CameraKeyframe *out_frame);

// Timeline
Timeline *timeline_create(void);
void timeline_add_event(Timeline *timeline, float time,
                        void (*callback)(void *), void *user_data);
void timeline_update(Timeline *timeline, float current_time);
