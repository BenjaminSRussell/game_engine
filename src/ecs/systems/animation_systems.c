#include "ecs/systems/animation_systems.h"
#include "animation/animation_allocator.h"
#include "ecs/components/animation_components.h"
#include "engine/include/core/logger.h"
#include "engine/include/math/math_all.h"
#include <stdio.h>

// Mock clip duration for validation
#define MOCK_CLIP_DURATION 2.0f

void animation_update_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  f32 dt = ctx->delta_time;

  ComponentType types[] = {g_animator_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    AnimatorComponent *anim = (AnimatorComponent *)comps[0];

    if (!anim->is_playing)
      continue;

    // Advance time
    anim->time += dt * anim->speed;

    // Handle looping (Mock logic)
    if (anim->time > MOCK_CLIP_DURATION) {
      if (anim->loop) {
        anim->time -= MOCK_CLIP_DURATION;
      } else {
        anim->time = MOCK_CLIP_DURATION;
        anim->is_playing = false; // Stop
      }
    }

    // Update Blending
    if (anim->next_clip_id != 0) {
      anim->blend_timer += dt;
      if (anim->blend_timer >= anim->blend_duration) {
        // Finish blend
        anim->current_clip_id = anim->next_clip_id;
        anim->next_clip_id = 0;
        anim->blend_weight = 0.0f;
        anim->time = 0.0f; // Reset time for new clip
      } else {
        // Calculate weight (linear)
        anim->blend_weight = anim->blend_timer / anim->blend_duration;
      }
    }
  }

  ecs_query_destroy(ctx->world, query);
}

void animation_skinning_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Reset blend arena frame allocator
  animation_allocator_reset_blend_arena();

  // Iterate Animator + Skeleton
  ComponentType types[] = {g_animator_component_id, g_skeleton_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 2};

  Query *query = ecs_query_create(ctx->world, &desc);
  Entity entity;
  void *comps[2];

  while (ecs_query_next(query, &entity, comps)) {
    AnimatorComponent *anim = (AnimatorComponent *)comps[0];
    SkeletonComponent *skel = (SkeletonComponent *)comps[1];

    // Mock Pose Evaluation & Skinning
    // In real code:
    // 1. Sample(anim->current_clip, anim->time, &poseA)
    // 2. Sample(anim->next_clip, &poseB)
    // 3. Blend(poseA, poseB, anim->blend_weight, &finalPose)
    // 4. ComputeMatrices(finalPose, skel->bone_matrices)

    // Simulate matrix update
    for (u32 i = 0; i < skel->bone_count; i++) {
      // Identity mock
      // skel->bone_matrices[i] = MAT4_IDENTITY;
    }
  }

  ecs_query_destroy(ctx->world, query);
}

void register_animation_systems(World *world) {
  if (!world) {
    LOG_ERROR("[Animation] Cannot register systems: null world");
    return;
  }

  // Update System (70)
  QueryDesc up_q = {0};
  System *update =
      ecs_system_create(world, "AnimUpdate", animation_update_system, &up_q);
  ecs_system_set_priority(world, update, 70);

  // Skinning System (80)
  QueryDesc skin_q = {0};
  System *skin = ecs_system_create(world, "AnimSkinning",
                                   animation_skinning_system, &skin_q);
  ecs_system_set_priority(world, skin, 80);

  LOG_INFO("[Animation] Registered systems:");
  LOG_INFO("  AnimUpdate (priority 70)");
  LOG_INFO("  AnimSkinning (priority 80)");
}
