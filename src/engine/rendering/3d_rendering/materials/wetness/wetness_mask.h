/*
 * wetness_mask.h
 * Dynamic wetness API
 *
 * Part of the Wetness System
 * Advanced 3D Rendering Engine
 */

#ifndef WETNESS_SYSTEM_WETNESS_MASK_H
#define WETNESS_SYSTEM_WETNESS_MASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wetness_params {
    float global_intensity;     // 0-1 rain amount
    float drying_factor;        // How fast it dries
    float absorption;           // Porosity of material
} wetness_params_t;

/**
 * Calculate wetness mask for a surface.
 * normal should be world space.
 */
float wetness_calculate_mask(const wetness_params_t* params, vec3_t normal, float cavity, float humidity);

/**
 * Apply wetness to material albedo (darkening).
 */
vec3_t wetness_apply_darkening(vec3_t albedo, float wetness, float absorption);

#ifdef __cplusplus
}
#endif

#endif /* WETNESS_SYSTEM_WETNESS_MASK_H */
