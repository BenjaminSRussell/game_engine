// Animation/Control Rig
#ifndef CONTROL_RIG_H
#define CONTROL_RIG_H

#include <common.h>
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_RIG_CONTROLS 128

typedef struct {
  char name[64];
  Vec3 position;
  Quat rotation;
  Vec3 scale;
  u32 parent_index;
  bool is_effector; // IK target
} RigControl;

typedef struct {
  RigControl controls[MAX_RIG_CONTROLS];
  u32 control_count;
  void *skeleton_ref;
} ControlRig;

#ifdef __cplusplus
extern "C" {
#endif

ControlRig *control_rig_create(void *skeleton);
void control_rig_add_control(ControlRig *rig, const char *name, Vec3 pos);
void control_rig_evaluate(ControlRig *rig, void *output_pose);

#ifdef __cplusplus
}
#endif

#endif
