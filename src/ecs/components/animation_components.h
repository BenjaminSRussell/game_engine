#ifndef ANIMATION_COMPONENTS_H
#define ANIMATION_COMPONENTS_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * ECS Animation Components
 *
 * - AnimatorComponent: Controls playback (clip, time, speed)
 * - SkeletonComponent: Stores bone hierarchy and matrices for rendering
 */

// ============================================================================
// ANIMATOR COMPONENT
// ============================================================================

typedef struct {
  u32 skeleton_id;     // Reference to skeleton resource
  u32 current_clip_id; // Playing clip
  u32 next_clip_id;    // Cross-fade target

  f32 time;  // Current playback time
  f32 speed; // Playback speed multiplier (default 1.0)
  bool loop; // Loop current clip?

  f32 blend_weight;   // 0.0 (current) -> 1.0 (next)
  f32 blend_duration; // Total time for cross-fade
  f32 blend_timer;    // Current transition time

  bool is_playing;
} AnimatorComponent;

// ============================================================================
// SKELETON COMPONENT
// ============================================================================

#define MAX_BONES 64

typedef struct {
  u32 bone_count;
  Mat4 bone_matrices[MAX_BONES]; // Final storage for Renderer

  // Internal state (allocator handles)
  void *local_pose;  // Ptr to Pose in Pool
  void *global_pose; // Ptr to Pose in Pool
} SkeletonComponent;

// ============================================================================
// REGISTRATION & HELPERS
// ============================================================================

extern u32 g_animator_component_id;
extern u32 g_skeleton_component_id;

void register_animation_components(void *world);

AnimatorComponent animator_create(u32 skeleton_id);
SkeletonComponent skeleton_create(u32 bone_count);

/**
 * Play animation clip
 */
void animator_play(AnimatorComponent *animator, u32 clip_id, bool loop);

/**
 * Cross-fade to new clip
 */
void animator_cross_fade(AnimatorComponent *animator, u32 clip_id, f32 duration,
                         bool loop);

#endif // ANIMATION_COMPONENTS_H
