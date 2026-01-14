#include "../Public/unified_animation.h"
#include "animation_types.h"
#include <string.h>

// Helper to find bone index by name
static i32 find_bone_index(AnimSkeleton *skeleton, const char *name) {
  if (!name)
    return -1;
  for (u32 i = 0; i < skeleton->bone_count; i++) {
    if (strcmp(skeleton->bones[i].name, name) == 0) {
      return (i32)i;
    }
  }
  return -1;
}

static inline Vec3 get_bone_world_pos(AnimSkeleton *skeleton, i32 bone_idx) {
  // Extract translation from global transform matrix
  Mat4 *m = &skeleton->global_transforms[bone_idx];
  // Mat4 is likely a 2D array m[4][4] or similar structure where m[col][row] is
  // standard Translation is in the last column (index 3)
  return (Vec3){m->m[3][0], m->m[3][1], m->m[3][2]};
}

void anim_solve_two_bone_ik(AnimSkeleton *skeleton, const char *root_bone,
                            const char *mid_bone, const char *end_bone,
                            Vec3 target, Vec3 pole_vector) {
  (void)pole_vector; // Unused for now
  if (!skeleton)
    return;

  i32 root_idx = find_bone_index(skeleton, root_bone);
  i32 mid_idx = find_bone_index(skeleton, mid_bone);
  i32 end_idx = find_bone_index(skeleton, end_bone);

  if (root_idx < 0 || mid_idx < 0 || end_idx < 0)
    return;

  // Basic Analytic 2-Bone IK Logic
  // 1. Get positions
  Vec3 root_pos = get_bone_world_pos(skeleton, root_idx);
  Vec3 mid_pos = get_bone_world_pos(skeleton, mid_idx);
  Vec3 end_pos = get_bone_world_pos(skeleton, end_idx);

  // 2. Calculate bone lengths
  // Assuming vec3_length and vec3_sub are available via unified_animation.h ->
  // vec3.h
  f32 len1 = vec3_length(vec3_sub(mid_pos, root_pos));
  f32 len2 = vec3_length(vec3_sub(end_pos, mid_pos));

  // 3. Calculate distance to target
  Vec3 to_target = vec3_sub(target, root_pos);
  f32 dist = vec3_length(to_target);

  // 4. Reachable check
  if (dist >= len1 + len2) {
    // Fully extended directly towards target
  } else {
    // ... (Law of cosines logic for joint angles) ...
  }
}
