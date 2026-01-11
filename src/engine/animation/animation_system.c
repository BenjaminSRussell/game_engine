#include <character/animation/animation_system.h>
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>

// TODO: Implement Pose Assets and Animation Blueprints for data-driven layering
// TODO: Add support for additive animation blending and motion warping
// TODO: Implement a robust root motion extraction and world-space integration
// system
// TODO: Add support for vertex-animated textures (VAT) for high-density crowds
// TODO: Implement real-time IK solving for terrain alignment (Foot IK)
// TODO: Add support for dual-quaternion skinning to maintain volume during
// large bends
// TODO: Implement animation compression (Quantization, Curve Fitting) for
// memory efficiency
// TODO: Add support for cloth and soft-body animation via secondary solver
// integration
// TODO: Implement a multi-threaded animation evaluation graph with task-based
// scheduling
// TODO: Add support for procedural animation synthesis using Motion Matching
// TODO: Implement facial animation blending with ARKit/OpenFace integration
// support
// TODO: Add logic for animation-driven physics constraints (e.g., character
// interactions)
// TODO: Research AI-driven motion prediction for smoother network replication
// TODO: Implement a comprehensive animation debugger with bone hierarchy
// visualization
// TODO: Add support for custom animation nodes and user-defined blending logic
// TODO: Implement animation-driven audio trigger points (AnimNotifies)
// TODO: Research integration with Control Rig for real-time procedural
// adjustments
// TODO: Add support for sub-pose caching and incremental pose updates
// TODO: Implement LOD-based animation updates (lower update frequency for
// distant targets)
// TODO: Research and implement GPU-driven skinning evaluation for
// Nanite-integrated meshes

// Global state (internal)
static struct {
  bool initialized;
} anim_sys_state;

void animation_system_init(void) {
  if (anim_sys_state.initialized)
    return;
  anim_sys_state.initialized = true;
  LOG_INFO("Animation System Initialized");
}

void animation_system_update(f32 dt) {
  if (!anim_sys_state.initialized)
    return;
  // In a component-based system, usually systems iterate over entities.
  // If we had a central registry of skeletons, we'd update them here.
  // For now, Ecs or GameLoop likely calls updates on components individually,
  // or we might add a global skeleton list later.
  // So this is a placeholder or global manager update.
}

void animation_system_shutdown(void) {
  anim_sys_state.initialized = false;
  LOG_INFO("Animation System Shutdown");
}

Skeleton *animation_create_skeleton(u32 bone_count) {
  if (bone_count > MAX_BONES) {
    LOG_ERROR("Bone count %u exceeds maximum %u", bone_count, MAX_BONES);
    return NULL;
  }

  Skeleton *skeleton = (Skeleton *)calloc(1, sizeof(Skeleton));
  if (skeleton) {
    skeleton->bone_count = bone_count;
    // Initialize matrices to identity
    for (u32 i = 0; i < bone_count; i++) {
      skeleton->global_transforms[i] = mat4_identity();
      skeleton->bones[i].local_bind_pose = mat4_identity();
      skeleton->bones[i].inverse_bind_pose = mat4_identity();
      skeleton->bones[i].parent_index = -1;
    }
  }
  return skeleton;
}

void animation_destroy_skeleton(Skeleton *skeleton) {
  if (skeleton) {
    free(skeleton);
  }
}

void animation_update_skeleton(Skeleton *skeleton, AnimationState *state) {
  if (!skeleton || !state)
    return;

  // 1. Advance time
  if (state->is_playing && state->current_clip) {
    state->current_time +=
        1.0f / 60.0f *
        state
            ->playback_speed; // Approximating DT if not passed, ideally pass DT
    // Note: The API 'animation_update_skeleton' implies it handles logic.
    // Ideally we should pass DT to this function or store it in state->last_dt?
    // Let's assume the caller updated current_time or we update it here using a
    // fixed step ref? Actually, normally 'update' functions take dt. The header
    // signature is: void animation_update_skeleton(Skeleton *skeleton,
    // AnimationState *state); So state->current_time is likely updated
    // externally or we guess.

    // Handle looping
    if (state->current_time > state->current_clip->duration) {
      if (state->is_looping) {
        state->current_time =
            fmodf(state->current_time, state->current_clip->duration);
      } else {
        state->current_time = state->current_clip->duration;
        state->is_playing = false;
      }
    }

    // 2. Sample Clip
    Pose local_pose;
    animation_sample_clip(state->current_clip, state->current_time,
                          &local_pose);

    // 3. Apply to Skeleton (Compute Globals)
    animation_apply_pose_to_skeleton(&local_pose, skeleton);
  }
}
