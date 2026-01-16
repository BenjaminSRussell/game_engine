#include "ecs/components/animation_components.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

u32 g_animator_component_id = 0;
u32 g_skeleton_component_id = 0;

void register_animation_components(void *world) {
  if (!world) {
    LOG_ERROR("[Animation] Cannot register components: null world");
    return;
  }

  // Register Animator
  ComponentInfo anim_info = {.name = "Animator",
                             .size = sizeof(AnimatorComponent),
                             .alignment = alignof(AnimatorComponent)};
  g_animator_component_id = ecs_register_component(world, &anim_info);

  // Register Skeleton
  ComponentInfo skel_info = {.name = "Skeleton",
                             .size = sizeof(SkeletonComponent),
                             .alignment = alignof(SkeletonComponent)};
  g_skeleton_component_id = ecs_register_component(world, &skel_info);

  LOG_INFO("[Animation] Registered components:");
  LOG_INFO("  Animator: ID %u", g_animator_component_id);
  LOG_INFO("  Skeleton: ID %u", g_skeleton_component_id);
}

AnimatorComponent animator_create(u32 skeleton_id) {
  AnimatorComponent anim = {0};
  anim.skeleton_id = skeleton_id;
  anim.speed = 1.0f;
  anim.loop = true;
  anim.is_playing = false;
  return anim;
}

SkeletonComponent skeleton_create(u32 bone_count) {
  SkeletonComponent skel = {0};
  skel.bone_count = (bone_count > MAX_BONES) ? MAX_BONES : bone_count;

  // Initialize identity matrices
  for (int i = 0; i < skel.bone_count; i++) {
    // Mat4 Identity logic placeholder
    // skel.bone_matrices[i] = MAT4_IDENTITY;
  }
  return skel;
}

void animator_play(AnimatorComponent *animator, u32 clip_id, bool loop) {
  if (!animator)
    return;
  animator->current_clip_id = clip_id;
  animator->next_clip_id = 0;
  animator->time = 0.0f;
  animator->loop = loop;
  animator->is_playing = true;
  animator->blend_weight = 0.0f;
}

void animator_cross_fade(AnimatorComponent *animator, u32 clip_id, f32 duration,
                         bool loop) {
  if (!animator)
    return;

  // If not playing, just snap
  if (!animator->is_playing) {
    animator_play(animator, clip_id, loop);
    return;
  }

  animator->next_clip_id = clip_id;
  animator->blend_duration = duration;
  animator->blend_timer = 0.0f;
  animator->blend_weight = 0.0f;
  // Note: loop applies to next clip, effectively
}
