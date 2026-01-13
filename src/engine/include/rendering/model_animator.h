// include/render/model_animator.h
//
// Purpose: Defines a model animation system for skeletal animation playback.
// Handles animation blending, state machines, and bone transformations for
// character models (player, NPCs).
//
// Public APIs:
// - `AnimationClip`: Individual animation data
// - `AnimationState`: Animation playback state
// - `ModelAnimator`: Animation controller
// - `model_animator_init`: Initialize animator
// - `model_animator_play`: Play an animation
// - `model_animator_update`: Update animation state
// - `model_animator_blend`: Blend between animations
//
#ifndef MODEL_ANIMATOR_H
#define MODEL_ANIMATOR_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/quat.h>
#include <math/mat4.h>

#define MAX_BONES 128
#define MAX_ANIMATION_CLIPS 32
#define MAX_BLEND_ANIMATIONS 4

// Animation keyframe for a single bone
typedef struct {
    f32 time;
    Vec3 position;
    Quat rotation;
    Vec3 scale;
} AnimationKeyframe;

// Bone in skeleton
typedef struct {
    char name[64];
    i32 parent_index;        // -1 if root bone
    Mat4 bind_pose;          // Bind pose transform
    Mat4 inverse_bind_pose;  // Inverse bind pose for skinning
} Bone;

// Skeleton definition
typedef struct {
    Bone bones[MAX_BONES];
    u32 bone_count;
} Skeleton;

// Animation track for a single bone
typedef struct {
    u32 bone_index;
    AnimationKeyframe *keyframes;
    u32 keyframe_count;
} AnimationTrack;

// Animation clip
typedef struct {
    char name[64];
    f32 duration;            // Total animation duration in seconds
    f32 ticks_per_second;    // Playback speed
    AnimationTrack *tracks;
    u32 track_count;
    bool loops;              // Whether animation should loop
} AnimationClip;

// Animation state machine states
typedef enum {
    ANIM_STATE_IDLE,
    ANIM_STATE_WALK,
    ANIM_STATE_RUN,
    ANIM_STATE_SPRINT,
    ANIM_STATE_JUMP,
    ANIM_STATE_FALL,
    ANIM_STATE_LAND,
    ANIM_STATE_ATTACK,
    ANIM_STATE_BLOCK,
    ANIM_STATE_HIT,
    ANIM_STATE_DEATH,
    ANIM_STATE_SWIM,
    ANIM_STATE_CLIMB,
    ANIM_STATE_CROUCH,
    ANIM_STATE_COUNT
} AnimationStateEnum;

// Animation blend entry
typedef struct {
    u32 clip_index;
    f32 weight;              // 0.0 to 1.0
    f32 time;                // Current playback time
    f32 speed;               // Playback speed multiplier
} AnimationBlendEntry;

// Model animator
typedef struct {
    Skeleton skeleton;
    AnimationClip clips[MAX_ANIMATION_CLIPS];
    u32 clip_count;

    // Current playback state
    AnimationBlendEntry blend_stack[MAX_BLEND_ANIMATIONS];
    u32 blend_count;

    // Computed bone transforms
    Mat4 bone_transforms[MAX_BONES];     // Final bone transforms
    Mat4 bone_matrices[MAX_BONES];       // Skinning matrices (transform * inverse_bind_pose)

    // State machine
    AnimationStateEnum current_state;
    AnimationStateEnum next_state;
    f32 transition_time;
    f32 transition_duration;
    bool transitioning;

    // Animation events
    void (*on_animation_event)(const char *event_name, void *user_data);
    void *user_data;
} ModelAnimator;

// Initialize model animator
void model_animator_init(ModelAnimator *animator);

// Free model animator resources
void model_animator_free(ModelAnimator *animator);

// Load skeleton from data
bool model_animator_load_skeleton(ModelAnimator *animator, const Bone *bones, u32 bone_count);

// Load animation clip
bool model_animator_load_clip(ModelAnimator *animator, const AnimationClip *clip);

// Find clip by name
i32 model_animator_find_clip(const ModelAnimator *animator, const char *name);

// Play animation by index
void model_animator_play(ModelAnimator *animator, u32 clip_index, f32 blend_time);

// Play animation by name
void model_animator_play_by_name(ModelAnimator *animator, const char *name, f32 blend_time);

// Crossfade to another animation
void model_animator_crossfade(ModelAnimator *animator, u32 clip_index, f32 duration);

// Update animation (call every frame)
void model_animator_update(ModelAnimator *animator, f32 delta_time);

// Set animation state (for state machine)
void model_animator_set_state(ModelAnimator *animator, AnimationStateEnum state);

// Get current bone transform
Mat4 model_animator_get_bone_transform(const ModelAnimator *animator, u32 bone_index);

// Get bone matrix for skinning
Mat4 model_animator_get_bone_matrix(const ModelAnimator *animator, u32 bone_index);

// Sample animation track at time
void model_animator_sample_track(const AnimationTrack *track, f32 time,
                                Vec3 *out_position, Quat *out_rotation, Vec3 *out_scale);

// Blend two transforms
void model_animator_blend_transform(const Mat4 *a, const Mat4 *b, f32 weight, Mat4 *out);

#endif // MODEL_ANIMATOR_H
