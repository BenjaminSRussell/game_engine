// include/engine/ml/neural_animation.h
//
// Purpose: Neural animation synthesis system for procedural character animation
// Uses ML to generate realistic character movements and transitions
//

#ifndef NEURAL_ANIMATION_H
#define NEURAL_ANIMATION_H

#include "../../common.h"
#include "ml_core.h"
#include <stdbool.h>

typedef enum {
    ANIM_MODEL_POSE_PREDICTION = 0,
    ANIM_MODEL_MOTION_GENERATION,
    ANIM_MODEL_TRANSITION_SYNTHESIS,
    ANIM_MODEL_STYLE_TRANSFER,
    ANIM_MODEL_CUSTOM
} AnimationModel;

typedef struct {
    MLSystem *ml_system;
    void *animation_model;
    AnimationModel model_type;
    void *pose_data;
    void *motion_data;
    f32 animation_speed;
    bool is_generating;
    bool initialized;
} NeuralAnimationSystem;

NeuralAnimationSystem *neural_animation_create(MLSystem *ml_system);
void neural_animation_destroy(NeuralAnimationSystem *system);
bool neural_animation_initialize(NeuralAnimationSystem *system, AnimationModel model);
bool neural_animation_generate_pose(NeuralAnimationSystem *system, void *input_context, void *output_pose);
bool neural_animation_generate_motion(NeuralAnimationSystem *system, void *start_pose, void *end_pose, void *motion_output);
void neural_animation_set_speed(NeuralAnimationSystem *system, f32 speed);

#endif // NEURAL_ANIMATION_H
