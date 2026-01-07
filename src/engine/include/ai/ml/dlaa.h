// include/engine/ml/dlaa.h
//
// Purpose: Deep Learning Anti-Aliasing (DLAA) system
// Uses neural networks for intelligent edge smoothing and aliasing reduction
//

#ifndef DLAA_H
#define DLAA_H

#include "include/common.h"
#include "include/ai/ml/ml_core.h"
#include <stdbool.h>

typedef enum {
    DLAA_MODEL_EDGE_DETECT = 0,
    DLAA_MODEL_TEMPORAL,
    DLAA_MODEL_ADAPTIVE,
    DLAA_MODEL_CUSTOM
} DLAAAModel;

typedef struct {
    MLSystem *ml_system;
    void *dlaa_model;
    DLAAAModel model_type;
    void *input_buffer;
    void *output_buffer;
    void *history_buffer[4];
    f32 sharpness_strength;
    bool temporal_enabled;
    bool initialized;
} DLAAASystem;

DLAAASystem *dlaa_create(MLSystem *ml_system);
void dlaa_destroy(DLAAASystem *dlaa);
bool dlaa_initialize(DLAAASystem *dlaa, DLAAAModel model);
bool dlaa_process_frame(DLAAASystem *dlaa, void *input_frame, void *output_frame);
void dlaa_set_sharpness(DLAAASystem *dlaa, f32 strength);

#endif // DLAA_H
