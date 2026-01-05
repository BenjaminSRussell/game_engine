/**
 * =================================================================================================
 *                              ANIMATION STATE MACHINE
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Complete animation state machine with blending, layers, and IK.
 *
 * =================================================================================================
 */

#ifndef ANIMATION_STATE_MACHINE_H
#define ANIMATION_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    ANIMATION CLIP
 * =================================================================================================
 */

typedef struct AnimationKeyframe {
  float time;
  float value[4]; // Position (3) or rotation (4) or scale (3)
  float tangent_in[4];
  float tangent_out[4];
} AnimationKeyframe;

typedef struct AnimationChannel {
  char target_path[64];
  int32_t bone_index;
  uint32_t property_type; // Position, rotation, scale, custom
  AnimationKeyframe *keyframes;
  uint32_t keyframe_count;
} AnimationChannel;

typedef struct AnimationClip {
  uint32_t id;
  char name[64];
  char path[256];
  float duration;
  float sample_rate;
  bool is_looping;
  bool is_additive;
  AnimationChannel *channels;
  uint32_t channel_count;

  // Events
  struct {
    float time;
    char name[32];
    char param[64];
  } events[16];
  uint32_t event_count;

  // Root motion
  bool has_root_motion;
  float root_motion_delta[3];
  float root_rotation_delta;
} AnimationClip;

AnimationClip *animation_clip_load(const char *path);
void animation_clip_unload(AnimationClip *clip);
void animation_clip_sample(AnimationClip *clip, float time, void *pose);
void animation_clip_sample_channel(AnimationClip *clip, int channel, float time,
                                   float *value);
void animation_keyframe_interpolate(const AnimationKeyframe *k1,
                                    const AnimationKeyframe *k2, float t,
                                    float *out);
void animation_extract_root_motion(AnimationClip *clip, float t1, float t2,
                                   float *delta_pos, float *delta_rot);
void animation_events_get(AnimationClip *clip, float t1, float t2,
                          void **events, int *count);
AnimationClip *animation_clip_import_fbx(const char *path);
AnimationClip *animation_clip_import_gltf(const char *path);

/* =================================================================================================
 *                                    ANIMATION STATE
 * =================================================================================================
 */

typedef enum AnimationStateType {
  STATE_SINGLE_CLIP,
  STATE_BLEND_TREE,
  STATE_SUB_STATE_MACHINE,
} AnimationStateType;

typedef struct AnimationState {
  uint32_t id;
  char name[64];
  AnimationStateType type;

  // For single clip
  uint32_t clip_id;
  float speed;
  bool mirror;

  // For blend tree
  uint32_t blend_tree_id;

  // For sub-state machine
  uint32_t sub_machine_id;

  // Timing
  bool loop;
  float cycle_offset;

  // Foot IK
  bool use_foot_ik;

  // Position in editor
  float editor_position[2];
} AnimationState;

AnimationState *animation_state_create(const char *name);
void animation_state_destroy(AnimationState *state);
void animation_state_evaluate(AnimationState *state, float time, void *pose);
float animation_state_get_duration(AnimationState *state);

/* =================================================================================================
 *                                    TRANSITION
 * =================================================================================================
 */

typedef enum TransitionInterruptionType {
  INTERRUPTION_NONE,
  INTERRUPTION_CURRENT,
  INTERRUPTION_NEXT,
  INTERRUPTION_BOTH,
} TransitionInterruptionType;

typedef struct TransitionCondition {
  char parameter_name[32];
  uint32_t comparison; // ==, !=, <, >, <=, >=
  float threshold;
  bool use_trigger;
} TransitionCondition;

typedef struct AnimationTransition {
  uint32_t id;
  uint32_t from_state;
  uint32_t to_state;

  TransitionCondition *conditions;
  uint32_t condition_count;

  float duration;
  float offset;
  TransitionInterruptionType interruption;
  bool has_exit_time;
  float exit_time;
  bool has_fixed_duration;
  bool ordered_interruption;
} AnimationTransition;

AnimationTransition *transition_create(uint32_t from, uint32_t to);
void transition_destroy(AnimationTransition *transition);
bool transition_check_conditions(AnimationTransition *transition,
                                 void *parameters);
bool transition_check_exit_time(AnimationTransition *transition,
                                float state_time, float state_duration);
void transition_evaluate(AnimationTransition *transition, float time,
                         float *weight);
void transition_blend_poses(void *pose_from, void *pose_to, float weight,
                            void *pose_out);

/* =================================================================================================
 *                                    BLEND TREE
 * =================================================================================================
 */

typedef enum BlendType {
  BLEND_1D,
  BLEND_2D_SIMPLE,
  BLEND_2D_FREEFORM_DIRECTIONAL,
  BLEND_2D_FREEFORM_CARTESIAN,
  BLEND_ADDITIVE,
} BlendType;

typedef struct BlendNode {
  uint32_t id;
  char name[32];
  uint32_t clip_id;
  float position[2]; // X,Y position for 2D blending
  float speed;
  bool mirror;
} BlendNode;

typedef struct BlendTree {
  uint32_t id;
  char name[64];
  BlendType type;

  BlendNode *nodes;
  uint32_t node_count;

  char parameter_x[32];
  char parameter_y[32];

  float min_threshold;
  float max_threshold;
  bool auto_thresholds;
} BlendTree;

BlendTree *blend_tree_create(const char *name);
void blend_tree_add_node(BlendTree *tree, BlendNode *node);
void blend_tree_remove_node(BlendTree *tree, int index);
void blend_tree_evaluate_1d(BlendTree *tree, float param, void *pose);
void blend_tree_evaluate_2d_simple(BlendTree *tree, float x, float y,
                                   void *pose);
void blend_tree_evaluate_2d_freeform(BlendTree *tree, float x, float y,
                                     void *pose);
void blend_tree_evaluate_additive(BlendTree *tree, void *base_pose, void *pose);
void blend_tree_compute_weights(BlendTree *tree, float x, float y,
                                float *weights);
void blend_tree_sample_clips(BlendTree *tree, float *weights, float time,
                             void *pose);

/* =================================================================================================
 *                                    ANIMATION LAYER
 * =================================================================================================
 */

typedef enum LayerBlendMode {
  LAYER_OVERRIDE,
  LAYER_ADDITIVE,
} LayerBlendMode;

typedef struct AnimationLayer {
  uint32_t id;
  char name[32];
  LayerBlendMode blend_mode;
  float weight;

  uint32_t avatar_mask; // Which bones affected
  bool ik_pass;
  bool sync_with_layer;
  uint32_t sync_layer_index;

  // States for this layer
  uint32_t *state_ids;
  uint32_t state_count;
  uint32_t default_state;
  uint32_t current_state;

  // Transitions
  uint32_t *transition_ids;
  uint32_t transition_count;
} AnimationLayer;

AnimationLayer *animation_layer_create(const char *name);
void animation_layer_destroy(AnimationLayer *layer);
void animation_layer_add_state(AnimationLayer *layer, AnimationState *state);
void animation_layer_add_transition(AnimationLayer *layer,
                                    AnimationTransition *transition);
void animation_layer_evaluate(AnimationLayer *layer, float dt, void *pose);
void animation_layer_blend(void *base_pose, void *layer_pose, float weight,
                           LayerBlendMode mode);
void animation_layer_apply_mask(void *pose, uint32_t mask);

/* =================================================================================================
 *                                    STATE MACHINE
 * =================================================================================================
 */

typedef struct AnimationParameter {
  char name[32];
  uint32_t type; // Float, Int, Bool, Trigger
  union {
    float float_value;
    int32_t int_value;
    bool bool_value;
  } value;
  union {
    float float_default;
    int32_t int_default;
    bool bool_default;
  } default_value;
} AnimationParameter;

typedef struct AnimationStateMachine {
  uint32_t id;
  char name[64];

  AnimationState *states;
  uint32_t state_count;
  uint32_t state_capacity;

  AnimationTransition *transitions;
  uint32_t transition_count;
  uint32_t transition_capacity;

  AnimationLayer *layers;
  uint32_t layer_count;

  AnimationParameter *parameters;
  uint32_t parameter_count;

  BlendTree *blend_trees;
  uint32_t blend_tree_count;
} AnimationStateMachine;

AnimationStateMachine *state_machine_create(const char *name);
void state_machine_destroy(AnimationStateMachine *sm);
void state_machine_add_state(AnimationStateMachine *sm, AnimationState *state);
void state_machine_add_transition(AnimationStateMachine *sm,
                                  AnimationTransition *transition);
void state_machine_add_parameter(AnimationStateMachine *sm,
                                 AnimationParameter *param);
void state_machine_set_parameter(AnimationStateMachine *sm, const char *name,
                                 void *value);
void *state_machine_get_parameter(AnimationStateMachine *sm, const char *name);
void state_machine_set_trigger(AnimationStateMachine *sm, const char *name);
void state_machine_reset_triggers(AnimationStateMachine *sm);
void state_machine_serialize(AnimationStateMachine *sm, const char *path);
void state_machine_deserialize(AnimationStateMachine *sm, const char *path);
AnimationStateMachine *state_machine_load(const char *path);
void state_machine_save(AnimationStateMachine *sm, const char *path);

/* =================================================================================================
 *                                    ANIMATION CONTROLLER
 * =================================================================================================
 */

typedef struct AnimationPose {
  float *bone_transforms;
  uint32_t bone_count;
  float root_position[3];
  float root_rotation[4];
} AnimationPose;

typedef struct AnimationController {
  uint32_t id;
  uint32_t state_machine_id;

  // Runtime state
  uint32_t current_states[8]; // Per layer
  uint32_t layer_count;

  float state_times[8];
  AnimationTransition *active_transitions[8];
  float transition_times[8];

  // Output
  AnimationPose current_pose;
  AnimationPose previous_pose;

  // Root motion
  float root_motion_delta[3];
  float root_rotation_delta;
  bool apply_root_motion;

  // Events
  void (*on_event)(const char *event, const char *param);
  void (*on_state_enter)(uint32_t state_id);
  void (*on_state_exit)(uint32_t state_id);
} AnimationController;

AnimationController *animation_controller_create(uint32_t sm_id);
void animation_controller_destroy(AnimationController *controller);
void animation_controller_update(AnimationController *controller, float dt);
void animation_controller_evaluate(AnimationController *controller, void *pose);
void *animation_controller_get_pose(AnimationController *controller);
void animation_controller_apply_to_skeleton(AnimationController *controller,
                                            void *skeleton);
void animation_controller_get_root_motion(AnimationController *controller,
                                          float *pos, float *rot);
void animation_controller_crossfade(AnimationController *controller,
                                    const char *state, float duration);
void animation_controller_play(AnimationController *controller,
                               const char *state);
void animation_controller_blend(AnimationController *controller,
                                float target_weight, float duration);
void animation_pose_blend(void *dest, const void *src, float weight);
void animation_pose_additive(void *dest, const void *src, float weight);
void animation_pose_copy(void *dest, const void *src);

/* =================================================================================================
 *                                    IK SYSTEM
 * =================================================================================================
 */

typedef struct IKChain {
  char name[32];
  int32_t *bone_indices;
  uint32_t bone_count;
  int32_t end_effector;
  float *bone_lengths;
  float target[3];
  float pole[3];
  float weight;
  uint32_t iterations;
} IKChain;

IKChain *ik_chain_create(const char *name);
void ik_solve_ccd(IKChain *chain, void *pose);
void ik_solve_fabrik(IKChain *chain, void *pose);
void ik_solve_two_bone(IKChain *chain, void *pose);
void ik_foot_placement(void *pose, void *terrain);
void ik_aim_constraint(void *pose, int bone, float *target);
void ik_look_at(void *pose, int bone, float *target);
void ik_ground_adaptation(void *pose, void *terrain);

#endif // ANIMATION_STATE_MACHINE_H
