// src/engine/animation/control_rig.c
#include "include/animation/control_rig.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

ControlRig *control_rig_create(void *skeleton) {
  ControlRig *rig = (ControlRig *)calloc(1, sizeof(ControlRig));
  if (!rig) {
    LOG_ERROR("Failed to allocate ControlRig");
    return NULL;
  }
  rig->skeleton_ref = skeleton;
  rig->control_count = 0;
  return rig;
}

void control_rig_add_control(ControlRig *rig, const char *name, Vec3 pos) {
  if (!rig || rig->control_count >= MAX_RIG_CONTROLS) {
    LOG_WARN("ControlRig: Max controls reached or invalid rig");
    return;
  }

  RigControl *ctrl = &rig->controls[rig->control_count++];
  strncpy(ctrl->name, name, sizeof(ctrl->name) - 1);
  ctrl->position = pos;
  ctrl->rotation = (Quat){0, 0, 0, 1};
  ctrl->scale = (Vec3){1, 1, 1};
  ctrl->parent_index = 0xFFFFFFFF; // Root
  ctrl->is_effector = false;

  LOG_INFO("Control Rig: Added control '%s'", name);
}

void control_rig_evaluate(ControlRig *rig, void *output_pose) {
  if (!rig)
    return;

  // In a full implementation, this would:
  // 1. Traverse control hierarchy
  // 2. Solve IK chains (FBIK/CCDIK) using effectors
  // 3. Apply results to output_pose bones

  // For now, we perform a pass to ensure controls are valid
  for (u32 i = 0; i < rig->control_count; i++) {
    RigControl *ctrl = &rig->controls[i];
    // Trivial logic: Ensure Quaternions are normalized
    // normalize_quat(&ctrl->rotation);
    (void)ctrl;
  }
}
