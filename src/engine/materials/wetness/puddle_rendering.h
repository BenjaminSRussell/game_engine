/*
 * puddle_rendering.h
 * Puddle reflections API
 *
 * Part of the Wetness System
 * Advanced 3D Rendering Engine
 */

#ifndef WETNESS_SYSTEM_PUDDLE_RENDERING_H
#define WETNESS_SYSTEM_PUDDLE_RENDERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct puddle_params {
    float water_level;      // Height of water table
    float edge_softness;    // 0-1
    float reflection_int;   // Reflection intensity
} puddle_params_t;

/**
 * Calculate puddle mask based on world height and water level.
 */
float puddle_calculate_mask(const puddle_params_t* params, float world_height, float noise);

/**
 * Get puddle normals for ripples.
 */
vec3_t puddle_get_ripple_normal(vec3_t world_pos, float time);

#ifdef __cplusplus
}
#endif

#endif /* WETNESS_SYSTEM_PUDDLE_RENDERING_H */
