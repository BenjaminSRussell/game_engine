// animation/animation_blueprints.h
// Visual animation state machine (Unreal-style Animation Blueprints)
#ifndef ANIMATION_BLUEPRINTS_H
#define ANIMATION_BLUEPRINTS_H

#include "include/animation/animation_system.h"
#include "include/common.h"

#define ANIM_BP_MAX_STATES 64
#define ANIM_BP_MAX_TRANSITIONS 128
#define ANIM_BP_MAX_BLEND_NODES 32

// Animation Blueprint State
typedef struct {
  char name[64];
  AnimationClip *animation;
  f32 playback_speed;
  bool loops;

  // State entry/exit events
  void (*on_enter)(void *context);
  void (*on_exit)(void *context);
} AnimBPState;

// State transition condition
typedef struct {
  u32 from_state_id;
  u32 to_state_id;
  f32 blend_time;

  // Condition function (returns true to transition)
  bool (*condition)(void *context);
  f32 condition_threshold;
} AnimBPTransition;

// Blend node types
typedef enum {
  BLEND_BY_BOOL,
  BLEND_BY_INT,
  BLEND_BY_FLOAT,
  BLEND_ADDITIVE,
  BLEND_LAYERED
} AnimBPBlendType;

typedef struct {
  AnimBPBlendType type;
  AnimationClip *clips[8];
  u32 clip_count;
  f32 blend_alpha;
} AnimBPBlendNode;

// Animation Blueprint (state machine + blend tree)
typedef struct {
  char name[64];

  // State machine
  AnimBPState states[ANIM_BP_MAX_STATES];
  u32 state_count;
  u32 current_state;
  u32 default_state;

  AnimBPTransition transitions[ANIM_BP_MAX_TRANSITIONS];
  u32 transition_count;

  // Blend tree
  AnimBPBlendNode blend_nodes[ANIM_BP_MAX_BLEND_NODES];
  u32 blend_node_count;

  // Runtime data
  f32 current_time;
  f32 transition_blend_alpha;
  bool is_transitioning;
  u32 transition_target;

  // Parameters (exposed to game logic)
  f32 speed;
  f32 direction;
  bool is_jumping;
  bool is_crouching;

} AnimationBlueprint;

#ifdef __cplusplus
extern "C" {
#endif

// Blueprint creation
AnimationBlueprint *anim_bp_create(const char *name);
void anim_bp_destroy(AnimationBlueprint *blueprint);

// State management
u32 anim_bp_add_state(AnimationBlueprint *bp, const char *name,
                      AnimationClip *clip);
void anim_bp_add_transition(AnimationBlueprint *bp, u32 from_state,
                            u32 to_state, bool (*condition)(void *),
                            f32 blend_time);
void anim_bp_set_default_state(AnimationBlueprint *bp, u32 state_id);

// Blend tree
u32 anim_bp_add_blend_node(AnimationBlueprint *bp, AnimBPBlendType type);
void anim_bp_set_blend_clips(AnimationBlueprint *bp, u32 node_id,
                             AnimationClip **clips, u32 count);

// Runtime update
void anim_bp_update(AnimationBlueprint *bp, f32 delta_time, void *context);
void anim_bp_evaluate(AnimationBlueprint *bp, Skeleton *skeleton,
                      Mat4 *out_transforms);

// Parameter setters (for game logic)
void anim_bp_set_float(AnimationBlueprint *bp, const char *param_name,
                       f32 value);
void anim_bp_set_bool(AnimationBlueprint *bp, const char *param_name,
                      bool value);
void anim_bp_trigger_event(AnimationBlueprint *bp, const char *event_name);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLUEPRINTS_H
