// src/engine/ml/neural_animation.c
//
// Purpose: Implementation of neural animation synthesis system
//

#include "../../../include/ml/neural_animation.h"
#include "../../../include/core/logger.h"
#include <stdlib.h>
#include <string.h>

NeuralAnimationSystem *neural_animation_create(MLSystem *ml_system) {
    if (!ml_system) return NULL;
    
    NeuralAnimationSystem *system = malloc(sizeof(NeuralAnimationSystem));
    if (!system) return NULL;
    
    memset(system, 0, sizeof(NeuralAnimationSystem));
    system->ml_system = ml_system;
    system->animation_speed = 1.0f;
    
    return system;
}

void neural_animation_destroy(NeuralAnimationSystem *system) {
    if (!system) return;
    
    neural_animation_shutdown(system);
    free(system);
}

bool neural_animation_initialize(NeuralAnimationSystem *system, AnimationModel model) {
    if (!system) return false;
    
    system->model_type = model;
    
    char model_path[256];
    snprintf(model_path, sizeof(model_path), "assets/models/anim_%s.mlmodel", 
             model == ANIM_MODEL_POSE_PREDICTION ? "pose" : "motion");
    
    MLModelMetadata metadata = {
        .name = "animation_model",
        .type = ML_MODEL_TYPE_CUSTOM,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = false,
        .max_batch_size = 1
    };
    
    system->animation_model = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->animation_model) {
        LOG_WARNING("Failed to load animation model");
        return false;
    }
    
    system->initialized = true;
    LOG_INFO("Neural Animation initialized with model: %d", model);
    return true;
}

bool neural_animation_generate_pose(NeuralAnimationSystem *system, void *input_context, void *output_pose) {
    if (!system || !input_context || !output_pose || !system->initialized) {
        return false;
    }
    
    if (!system->animation_model) {
        LOG_ERROR("Animation model not loaded");
        return false;
    }
    
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->animation_model);
    if (!context) return false;
    
    bool success = ml_run_inference(system->ml_system, context);
    
    ml_destroy_inference_context(system->ml_system, context);
    
    if (success) {
        LOG_DEBUG("Neural pose generated successfully");
    }
    
    return success;
}

bool neural_animation_generate_motion(NeuralAnimationSystem *system, void *start_pose, void *end_pose, void *motion_output) {
    if (!system || !start_pose || !end_pose || !motion_output || !system->initialized) {
        return false;
    }
    
    if (!system->animation_model) {
        LOG_ERROR("Animation model not loaded");
        return false;
    }
    
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->animation_model);
    if (!context) return false;
    
    bool success = ml_run_inference(system->ml_system, context);
    
    ml_destroy_inference_context(system->ml_system, context);
    
    if (success) {
        LOG_DEBUG("Neural motion generated successfully");
    }
    
    return success;
}

void neural_animation_set_speed(NeuralAnimationSystem *system, f32 speed) {
    if (!system) return;
    system->animation_speed = fmaxf(0.1f, fminf(5.0f, speed));
}

void neural_animation_shutdown(NeuralAnimationSystem *system) {
    if (!system || !system->initialized) return;
    
    if (system->animation_model) {
        ml_unload_model(system->ml_system, system->animation_model);
        system->animation_model = NULL;
    }
    
    system->initialized = false;
    LOG_INFO("Neural Animation shutdown complete");
}
