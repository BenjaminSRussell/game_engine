#ifndef MOTION_WARPING_H
#define MOTION_WARPING_H

#include "include/common.h"
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_WARP_TARGETS 8
#define MAX_WARP_WINDOWS 4

typedef struct {
  Vec3 position;
  Quat rotation;
} Transform;

typedef struct {
  char name[64];
  Vec3 location;
  Quat rotation;
  bool active;
} WarpTarget;

typedef struct {
  f32 start_time;
  f32 end_time;
  char target_name[64];
  bool warp_translation;
  bool warp_rotation;
} WarpWindow;

typedef struct {
  WarpTarget targets[MAX_WARP_TARGETS];
  u32 target_count;

  WarpWindow windows[MAX_WARP_WINDOWS];
  u32 window_count;

  bool enabled;
} MotionWarping;

#ifdef __cplusplus
extern "C" {
#endif

MotionWarping *motion_warp_create(void);
void motion_warp_destroy(MotionWarping *warp);

void motion_warp_add_target(MotionWarping *warp, const char *name,
                            Vec3 location, Quat rotation);
void motion_warp_remove_target(MotionWarping *warp, const char *name);
void motion_warp_clear_targets(MotionWarping *warp);

void motion_warp_add_window(MotionWarping *warp, f32 start_time, f32 end_time,
                            const char *target_name, bool warp_translation,
                            bool warp_rotation);

void motion_warp_update(MotionWarping *warp, Transform *root_motion_delta,
                        f32 current_time, f32 dt);

#ifdef __cplusplus
}
#endif

#endif
