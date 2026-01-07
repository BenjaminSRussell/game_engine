// src/engine/ml/dlaa.c
//
// Purpose: Implementation of Deep Learning Anti-Aliasing system
//

#include "../../../include/ml/dlaa.h"
#include "../../../include/core/logger.h"
#include <stdlib.h>
#include <string.h>

DLAAASystem *dlaa_create(MLSystem *ml_system) {
    if (!ml_system) return NULL;
    
    DLAAASystem *dlaa = malloc(sizeof(DLAAASystem));
    if (!dlaa) return NULL;
    
    memset(dlaa, 0, sizeof(DLAAASystem));
    dlaa->ml_system = ml_system;
    dlaa->sharpness_strength = 0.8f;
    
    return dlaa;
}

void dlaa_destroy(DLAAASystem *dlaa) {
    if (!dlaa) return;
    
    dlaa_shutdown(dlaa);
    free(dlaa);
}

bool dlaa_initialize(DLAAASystem *dlaa, DLAAAModel model) {
    if (!dlaa) return false;
    
    dlaa->model_type = model;
    
    char model_path[256];
    snprintf(model_path, sizeof(model_path), "assets/models/dlaa_%s.mlmodel", 
             model == DLAA_MODEL_EDGE_DETECT ? "edge" : "temporal");
    
    MLModelMetadata metadata = {
        .name = "dlaa_model",
        .type = ML_MODEL_TYPE_CUSTOM,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = false,
        .max_batch_size = 1
    };
    
    dlaa->dlaa_model = ml_load_model(dlaa->ml_system, model_path, &metadata);
    if (!dlaa->dlaa_model) {
        LOG_WARNING("Failed to load DLAA model");
        return false;
    }
    
    dlaa->initialized = true;
    LOG_INFO("DLAA initialized with model: %d", model);
    return true;
}

bool dlaa_process_frame(DLAAASystem *dlaa, void *input_frame, void *output_frame) {
    if (!dlaa || !input_frame || !output_frame || !dlaa->initialized) {
        return false;
    }
    
    if (!dlaa->dlaa_model) {
        LOG_ERROR("DLAA model not loaded");
        return false;
    }
    
    MLInferenceContext *context = ml_create_inference_context(dlaa->ml_system, dlaa->dlaa_model);
    if (!context) return false;
    
    bool success = ml_run_inference(dlaa->ml_system, context);
    
    ml_destroy_inference_context(dlaa->ml_system, context);
    
    if (success) {
        LOG_DEBUG("DLAA frame processed successfully");
    }
    
    return success;
}

void dlaa_set_sharpness(DLAAASystem *dlaa, f32 strength) {
    if (!dlaa) return;
    dlaa->sharpness_strength = fmaxf(0.0f, fminf(2.0f, strength));
}

void dlaa_shutdown(DLAAASystem *dlaa) {
    if (!dlaa || !dlaa->initialized) return;
    
    if (dlaa->dlaa_model) {
        ml_unload_model(dlaa->ml_system, dlaa->dlaa_model);
        dlaa->dlaa_model = NULL;
    }
    
    dlaa->initialized = false;
    LOG_INFO("DLAA shutdown complete");
}
