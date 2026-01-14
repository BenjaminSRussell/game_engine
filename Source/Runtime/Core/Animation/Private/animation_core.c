#include "../Public/unified_animation.h"
#include "animation_types.h"
#include <stdlib.h>
#include <string.h>

AnimSystem *anim_system_create(AnimConfig config) {
  AnimSystem *system = UNIFIED_ALLOC(sizeof(AnimSystem));
  if (!system) {
    LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate Animation System");
    return NULL;
  }

  system->config = config;
  LOG_INFO(LOG_CAT_ANIMATION, "Animation System created");
  return system;
}

void anim_system_destroy(AnimSystem *system) {
  if (!system)
    return;
  UNIFIED_FREE(system);
  LOG_INFO(LOG_CAT_ANIMATION, "Animation System destroyed");
}

void anim_system_update(AnimSystem *system, f32 delta_time) {
  if (!system)
    return;
  // System-wide updates (e.g. global timers) can go here
  (void)delta_time;
}
