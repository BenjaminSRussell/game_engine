// src/engine/animation/motion_warping.c
#include "include/animation/motion_warping.h"
#include "include/core/logger.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

MotionWarping *motion_warp_create(void) {
  MotionWarping *warp = (MotionWarping *)calloc(1, sizeof(MotionWarping));
  if (!warp)
    return NULL;

  warp->enabled = true;
  warp->target_count = 0;
  warp->window_count = 0;

  return warp;
}

void motion_warp_destroy(MotionWarping *warp) {
  if (warp)
    free(warp);
}

void motion_warp_add_target(MotionWarping *warp, const char *name,
                            Vec3 location, Quat rotation) {
  if (!warp || warp->target_count >= MAX_WARP_TARGETS)
    return;

  // Check if target already exists, update it
  for (u32 i = 0; i < warp->target_count; i++) {
    if (strcmp(warp->targets[i].name, name) == 0) {
      warp->targets[i].location = location;
      warp->targets[i].rotation = rotation;
      warp->targets[i].active = true;
      return;
    }
  }

  // Add new target
  WarpTarget *target = &warp->targets[warp->target_count++];
  strncpy(target->name, name, sizeof(target->name) - 1);
  target->location = location;
  target->rotation = rotation;
  target->active = true;
}

void motion_warp_remove_target(MotionWarping *warp, const char *name) {
  if (!warp)
    return;

  for (u32 i = 0; i < warp->target_count; i++) {
    if (strcmp(warp->targets[i].name, name) == 0) {
      warp->targets[i].active = false;
      return;
    }
  }
}

void motion_warp_clear_targets(MotionWarping *warp) {
  if (!warp)
    return;
  warp->target_count = 0;
}

void motion_warp_add_window(MotionWarping *warp, f32 start_time, f32 end_time,
                            const char *target_name, bool warp_translation,
                            bool warp_rotation) {
  if (!warp || warp->window_count >= MAX_WARP_WINDOWS)
    return;

  WarpWindow *window = &warp->windows[warp->window_count++];
  window->start_time = start_time;
  window->end_time = end_time;
  strncpy(window->target_name, target_name, sizeof(window->target_name) - 1);
  window->warp_translation = warp_translation;
  window->warp_rotation = warp_rotation;
}

void motion_warp_update(MotionWarping *warp, Transform *root_motion_delta,
                        f32 current_time, f32 dt) {
  if (!warp || !root_motion_delta || !warp->enabled)
    return;

  // Check if we're in any active warp window
  for (u32 i = 0; i < warp->window_count; i++) {
    WarpWindow *window = &warp->windows[i];

    if (current_time >= window->start_time &&
        current_time <= window->end_time) {
      // Find the target
      WarpTarget *target = NULL;
      for (u32 j = 0; j < warp->target_count; j++) {
        if (strcmp(warp->targets[j].name, window->target_name) == 0 &&
            warp->targets[j].active) {
          target = &warp->targets[j];
          break;
        }
      }

      if (!target)
        continue;

      // Calculate progress through window (0 to 1)
      f32 window_duration = window->end_time - window->start_time;

      // Warp translation
      if (window->warp_translation) {
        // Calculate how much we need to move to reach target by end of window
        // This is a simplified version - full implementation would track
        // accumulated motion
        Vec3 warp_offset = vec3_mul(target->location, dt / window_duration);
        root_motion_delta->position =
            vec3_add(root_motion_delta->position, warp_offset);
      }

      // Warp rotation
      if (window->warp_rotation) {
        // Interpolate towards target rotation
        f32 rotation_alpha = dt / (window->end_time - current_time);
        if (rotation_alpha > 1.0f)
          rotation_alpha = 1.0f;

        root_motion_delta->rotation = quat_slerp(
            root_motion_delta->rotation, target->rotation, rotation_alpha);
      }
    }
  }
}
