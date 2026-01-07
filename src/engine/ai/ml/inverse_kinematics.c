// src/engine/ml/inverse_kinematics.c
//
// Purpose: Implementation of ML-based inverse kinematics system
//

#include "ai/ml/inverse_kinematics.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

void ml_ik_shutdown(MLIKSystem *system);

MLIKSystem *ml_ik_create(MLSystem *ml_system) {
    if (!ml_system) return NULL;
    
    MLIKSystem *system = malloc(sizeof(MLIKSystem));
    if (!system) return NULL;
    
    memset(system, 0, sizeof(MLIKSystem));
    system->ml_system = ml_system;
    system->learning_rate = 0.01f;
    
    return system;
}

void ml_ik_destroy(MLIKSystem *system) {
    if (!system) return;
    
    ml_ik_shutdown(system);
    free(system);
}

bool ml_ik_initialize(MLIKSystem *system, IKModel model) {
    if (!system) return false;
    
    system->model_type = model;
    
    char model_path[256];
    snprintf(model_path, sizeof(model_path), "assets/models/ik_%s.mlmodel", 
             model == IK_MODEL_NEURAL_SOLVER ? "neural" : "fabrik");
    
    MLModelMetadata metadata = {
        .name = "ik_model",
        .type = ML_MODEL_TYPE_CUSTOM,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = false,
        .max_batch_size = 1
    };
    
    system->ik_model = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->ik_model) {
        LOG_WARN("Failed to load IK model");
        return false;
    }
    
    system->initialized = true;
    LOG_INFO("ML IK initialized with model: %d", model);
    return true;
}

bool ml_ik_solve_chain(MLIKSystem *system, IKChain *chain, Vec3 target) {
    if (!system || !chain || !system->initialized) {
        return false;
    }
    
    if (!system->ik_model) {
        LOG_ERROR("IK model not loaded");
        return false;
    }
    
    chain->target_position = target;
    
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->ik_model);
    if (!context) return false;
    
    bool success = ml_run_inference(system->ml_system, context);
    
    ml_destroy_inference_context(system->ml_system, context);
    
    if (success) {
        LOG_DEBUG("IK chain solved successfully");
    }
    
    return success;
}

bool ml_ik_solve_multiple(MLIKSystem *system, IKChain *chains, u32 chain_count) {
    if (!system || !chains || chain_count == 0 || !system->initialized) {
        return false;
    }
    
    bool all_solved = true;
    for (u32 i = 0; i < chain_count; i++) {
        if (!ml_ik_solve_chain(system, &chains[i], chains[i].target_position)) {
            all_solved = false;
        }
    }
    
    return all_solved;
}

void ml_ik_set_tolerance(MLIKSystem *system, f32 tolerance) {
    if (!system) return;
    
    for (u32 i = 0; i < system->chain_count; i++) {
        system->chains[i].tolerance = fmaxf(0.001f, tolerance);
    }
}

void ml_ik_shutdown(MLIKSystem *system) {
    if (!system || !system->initialized) return;
    
    if (system->ik_model) {
        ml_unload_model(system->ml_system, system->ik_model);
        system->ik_model = NULL;
    }
    
    system->initialized = false;
    LOG_INFO("ML IK shutdown complete");
}
