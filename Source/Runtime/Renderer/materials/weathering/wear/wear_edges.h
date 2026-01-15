/*
 * wear_edges.h
 * Edge wear API
 *
 * Part of the Wear & Tear subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WEAR_TEAR_WEAR_EDGES_H
#define WEAR_TEAR_WEAR_EDGES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wear_params {
    float amount;           // 0-1 overall wear
    float intensity;        // How much it affects albedo/roughness
    float contrast;         // Sharpness of wear
} wear_params_t;

/**
 * Calculate edge wear mask based on curvature and noise.
 */
float wear_calculate_edge_mask(const wear_params_t* params, float curvature, vec3_t world_pos);

/**
 * Apply wear to material properties.
 */
void wear_apply_to_properties(float mask, float* albedo_factor, float* roughness);

#ifdef __cplusplus
}
#endif

#endif /* WEAR_TEAR_WEAR_EDGES_H */
