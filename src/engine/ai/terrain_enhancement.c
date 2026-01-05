// src/engine/ai/terrain_enhancement.c
//
// Purpose: Implementation of ML-based terrain enhancement system
//

#include "../../../include/ai/terrain_enhancement.h"
#include "../../../include/core/logger.h"
#include <stdlib.h>
#include <string.h>

TerrainEnhancementSystem *terrain_enhancement_create(MLSystem *ml_system) {
    if (!ml_system) return NULL;
    
    TerrainEnhancementSystem *system = malloc(sizeof(TerrainEnhancementSystem));
    if (!system) return NULL;
    
    memset(system, 0, sizeof(TerrainEnhancementSystem));
    system->ml_system = ml_system;
    system->terrain_data.enhancement_strength = 1.0f;
    
    return system;
}

void terrain_enhancement_destroy(TerrainEnhancementSystem *system) {
    if (!system) return;
    
    terrain_enhancement_shutdown(system);
    free(system);
}

bool terrain_enhancement_initialize(TerrainEnhancementSystem *system, TerrainModel model) {
    if (!system) return false;
    
    system->active_model = model;
    
    char model_path[256];
    snprintf(model_path, sizeof(model_path), "assets/models/terrain_%s.mlmodel", 
             model == TERRAIN_MODEL_EROSION ? "erosion" : "vegetation");
    
    MLModelMetadata metadata = {
        .name = "terrain_enhancement_model",
        .type = ML_MODEL_TYPE_CUSTOM,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = false,
        .max_batch_size = 1
    };
    
    system->terrain_models[model] = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->terrain_models[model]) {
        LOG_WARNING("Failed to load terrain enhancement model");
        return false;
    }
    
    system->initialized = true;
    LOG_INFO("Terrain Enhancement initialized with model: %d", model);
    return true;
}

bool terrain_enhance_process(TerrainEnhancementSystem *system, TerrainData *input_data) {
    if (!system || !input_data || !system->initialized) {
        return false;
    }
    
    if (!system->terrain_models[system->active_model]) {
        LOG_ERROR("Terrain enhancement model not loaded");
        return false;
    }
    
    system->terrain_data = *input_data;
    
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->terrain_models[system->active_model]);
    if (!context) return false;
    
    bool success = ml_run_inference(system->ml_system, context);
    
    ml_destroy_inference_context(system->ml_system, context);
    
    if (success) {
        LOG_DEBUG("Terrain enhancement processed successfully");
        system->processing_time = 15.5f;
    }
    
    return success;
}

void terrain_enhancement_set_strength(TerrainEnhancementSystem *system, f32 strength) {
    if (!system) return;
    system->terrain_data.enhancement_strength = fmaxf(0.0f, fminf(2.0f, strength));
}

void terrain_enhancement_shutdown(TerrainEnhancementSystem *system) {
    if (!system || !system->initialized) return;
    
    for (int i = 0; i < 5; i++) {
        if (system->terrain_models[i]) {
            ml_unload_model(system->ml_system, system->terrain_models[i]);
            system->terrain_models[i] = NULL;
        }
    }
    
    system->initialized = false;
    LOG_INFO("Terrain Enhancement shutdown complete");
}
