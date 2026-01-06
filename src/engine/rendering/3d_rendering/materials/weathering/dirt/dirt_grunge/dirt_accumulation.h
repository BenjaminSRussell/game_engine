/*
 * dirt_accumulation.h
 * Dirt in crevices API
 *
 * Part of the Dirt & Grunge subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DIRT_GRUNGE_DIRT_ACCUMULATION_H
#define DIRT_GRUNGE_DIRT_ACCUMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dirt_params {
    float intensity;            // Overall dirt amount
    float cavity_bias;          // How much it favors cavities (0-1)
    float gravity_weight;       // How much it is affected by surface orientation
    vec3_t dirt_color;          // Base color of the dirt/dust
} dirt_params_t;

/**
 * Calculate dirt mask based on cavity map and surface normal.
 */
float dirt_calculate_mask(const dirt_params_t* params, float cavity, vec3_t normal, vec3_t world_pos);

/**
 * Apply dirt to an albedo color.
 */
vec3_t dirt_apply_to_albedo(vec3_t albedo, float mask, vec3_t dirt_color);

#ifdef __cplusplus
}
#endif

#endif /* DIRT_GRUNGE_DIRT_ACCUMULATION_H */
