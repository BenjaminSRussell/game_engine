/*
 * temporal_reprojection.h
 * Volumetric temporal accumulation
 */

#ifndef LIGHTING_TEMPORAL_REPROJECTION_H
#define LIGHTING_TEMPORAL_REPROJECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <math/mat4.h>
#include "backend/metal/mtl_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct temporal_reprojection {
    metal_texture_t* history_texture;
    float feedback_factor;
    uint32_t frame_index;
    
    // Halton sequence for jittering
    float jitter_x;
    float jitter_y;
} temporal_reprojection_t;

/**
 * Initializes temporal reprojection state.
 */
void temporal_reprojection_init(temporal_reprojection_t* reproj, metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d);

/**
 * Cleanup temporal reprojection resources.
 */
void temporal_reprojection_cleanup(temporal_reprojection_t* reproj);

/**
 * Updates frame jitter and state.
 */
void temporal_reprojection_next_frame(temporal_reprojection_t* reproj);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_TEMPORAL_REPROJECTION_H */
