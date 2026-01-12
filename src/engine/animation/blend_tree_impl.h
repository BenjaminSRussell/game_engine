#ifndef BLEND_TREE_IMPL_H
#define BLEND_TREE_IMPL_H

#include <stdbool.h>

// Forward declarations
typedef struct AnimState AnimState;
typedef struct AnimationClip AnimationClip;
typedef struct Pose Pose;
typedef struct Vector3 Vector3;

// Creation functions
AnimState *anim_state_create_clip(AnimationClip *clip);
AnimState *anim_state_create_blend_1d();
AnimState *anim_state_create_blend_2d();
AnimState *anim_state_create_state_machine();

// Blend tree operations
void anim_state_add_child(AnimState *state, AnimState *child, float weight, Vector3 blend_point);
void anim_eval_state(AnimState *state, Pose *out_pose, unsigned int bone_count);
void anim_state_update(AnimState *state, float delta_time, unsigned int bone_count);

// State machine operations
void anim_state_machine_transition(AnimState *machine, AnimState *target_state, float duration);
void anim_state_machine_eval(AnimState *machine, Pose *out_pose, unsigned int bone_count);

// Cleanup
void anim_state_destroy(AnimState *state);

#endif // BLEND_TREE_IMPL_H
