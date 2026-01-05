/**
 * =================================================================================================
 *                              ANIMATION STATE MACHINE IMPLEMENTATION
 *                                Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 */

#include "animation_state_machine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    ANIMATION CLIP
 * =================================================================================================
 */

AnimationClip *animation_clip_load(const char *path) {
  return calloc(1, sizeof(AnimationClip));
}
void animation_clip_unload(AnimationClip *clip) { free(clip); }
void animation_clip_sample(AnimationClip *clip, float time, void *pose) {}
void animation_clip_sample_channel(AnimationClip *clip, int channel, float time,
                                   float *value) {}
void animation_keyframe_interpolate(const AnimationKeyframe *k1,
                                    const AnimationKeyframe *k2, float t,
                                    float *out) {}
void animation_extract_root_motion(AnimationClip *clip, float t1, float t2,
                                   float *delta_pos, float *delta_rot) {}
void animation_events_get(AnimationClip *clip, float t1, float t2,
                          void **events, int *count) {}
AnimationClip *animation_clip_import_fbx(const char *path) {
  return calloc(1, sizeof(AnimationClip));
}
AnimationClip *animation_clip_import_gltf(const char *path) {
  return calloc(1, sizeof(AnimationClip));
}

/* =================================================================================================
 *                                    ANIMATION STATE
 * =================================================================================================
 */

AnimationState *animation_state_create(const char *name) {
  return calloc(1, sizeof(AnimationState));
}
void animation_state_destroy(AnimationState *state) { free(state); }
void animation_state_evaluate(AnimationState *state, float time, void *pose) {}
float animation_state_get_duration(AnimationState *state) { return 0.0f; }

/* =================================================================================================
 *                                    TRANSITION
 * =================================================================================================
 */

AnimationTransition *transition_create(uint32_t from, uint32_t to) {
  return calloc(1, sizeof(AnimationTransition));
}
void transition_destroy(AnimationTransition *transition) { free(transition); }
bool transition_check_conditions(AnimationTransition *transition,
                                 void *parameters) {
  return false;
}
bool transition_check_exit_time(AnimationTransition *transition,
                                float state_time, float state_duration) {
  return false;
}
void transition_evaluate(AnimationTransition *transition, float time,
                         float *weight) {}
void transition_blend_poses(void *pose_from, void *pose_to, float weight,
                            void *pose_out) {}

/* =================================================================================================
 *                                    BLEND TREE
 * =================================================================================================
 */

BlendTree *blend_tree_create(const char *name) {
  return calloc(1, sizeof(BlendTree));
}
void blend_tree_add_node(BlendTree *tree, BlendNode *node) {}
void blend_tree_remove_node(BlendTree *tree, int index) {}
void blend_tree_evaluate_1d(BlendTree *tree, float param, void *pose) {}
void blend_tree_evaluate_2d_simple(BlendTree *tree, float x, float y,
                                   void *pose) {}
void blend_tree_evaluate_2d_freeform(BlendTree *tree, float x, float y,
                                     void *pose) {}
void blend_tree_evaluate_additive(BlendTree *tree, void *base_pose,
                                  void *pose) {}
void blend_tree_compute_weights(BlendTree *tree, float x, float y,
                                float *weights) {}
void blend_tree_sample_clips(BlendTree *tree, float *weights, float time,
                             void *pose) {}

/* =================================================================================================
 *                                    ANIMATION LAYER
 * =================================================================================================
 */

AnimationLayer *animation_layer_create(const char *name) {
  return calloc(1, sizeof(AnimationLayer));
}
void animation_layer_destroy(AnimationLayer *layer) { free(layer); }
void animation_layer_add_state(AnimationLayer *layer, AnimationState *state) {}
void animation_layer_add_transition(AnimationLayer *layer,
                                    AnimationTransition *transition) {}
void animation_layer_evaluate(AnimationLayer *layer, float dt, void *pose) {}
void animation_layer_blend(void *base_pose, void *layer_pose, float weight,
                           LayerBlendMode mode) {}
void animation_layer_apply_mask(void *pose, uint32_t mask) {}

/* =================================================================================================
 *                                    STATE MACHINE
 * =================================================================================================
 */

AnimationStateMachine *state_machine_create(const char *name) {
  return calloc(1, sizeof(AnimationStateMachine));
}
void state_machine_destroy(AnimationStateMachine *sm) { free(sm); }
void state_machine_add_state(AnimationStateMachine *sm, AnimationState *state) {
}
void state_machine_add_transition(AnimationStateMachine *sm,
                                  AnimationTransition *transition) {}
void state_machine_add_parameter(AnimationStateMachine *sm,
                                 AnimationParameter *param) {}
void state_machine_set_parameter(AnimationStateMachine *sm, const char *name,
                                 void *value) {}
void *state_machine_get_parameter(AnimationStateMachine *sm, const char *name) {
  return NULL;
}
void state_machine_set_trigger(AnimationStateMachine *sm, const char *name) {}
void state_machine_reset_triggers(AnimationStateMachine *sm) {}
void state_machine_serialize(AnimationStateMachine *sm, const char *path) {}
void state_machine_deserialize(AnimationStateMachine *sm, const char *path) {}
AnimationStateMachine *state_machine_load(const char *path) { return NULL; }
void state_machine_save(AnimationStateMachine *sm, const char *path) {}

/* =================================================================================================
 *                                    ANIMATION CONTROLLER
 * =================================================================================================
 */

AnimationController *animation_controller_create(uint32_t sm_id) {
  return calloc(1, sizeof(AnimationController));
}
void animation_controller_destroy(AnimationController *controller) {
  free(controller);
}
void animation_controller_update(AnimationController *controller, float dt) {}
void animation_controller_evaluate(AnimationController *controller,
                                   void *pose) {}
void *animation_controller_get_pose(AnimationController *controller) {
  return NULL;
}
void animation_controller_apply_to_skeleton(AnimationController *controller,
                                            void *skeleton) {}
void animation_controller_get_root_motion(AnimationController *controller,
                                          float *pos, float *rot) {}
void animation_controller_crossfade(AnimationController *controller,
                                    const char *state, float duration) {}
void animation_controller_play(AnimationController *controller,
                               const char *state) {}
void animation_controller_blend(AnimationController *controller,
                                float target_weight, float duration) {}
void animation_pose_blend(void *dest, const void *src, float weight) {}
void animation_pose_additive(void *dest, const void *src, float weight) {}
void animation_pose_copy(void *dest, const void *src) {}

/* =================================================================================================
 *                                    IK SYSTEM
 * =================================================================================================
 */

IKChain *ik_chain_create(const char *name) {
  return calloc(1, sizeof(IKChain));
}
void ik_solve_ccd(IKChain *chain, void *pose) {}
void ik_solve_fabrik(IKChain *chain, void *pose) {}
void ik_solve_two_bone(IKChain *chain, void *pose) {}
void ik_foot_placement(void *pose, void *terrain) {}
void ik_aim_constraint(void *pose, int bone, float *target) {}
void ik_look_at(void *pose, int bone, float *target) {}
void ik_ground_adaptation(void *pose, void *terrain) {}
