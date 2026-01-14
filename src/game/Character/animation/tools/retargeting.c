#include <common.h>

void retargeting_init(void) {}

void retargeting_map_skeleton(void *source_skeleton, void *target_skeleton,
                              void *mapping) {
  (void)source_skeleton;
  (void)target_skeleton;
  (void)mapping;
}

void retargeting_apply(void *source_pose, void *mapping, void *target_pose) {
  (void)source_pose;
  (void)mapping;
  (void)target_pose;
}
