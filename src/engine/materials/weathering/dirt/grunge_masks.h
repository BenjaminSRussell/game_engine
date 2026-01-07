/*
 * grunge_masks.h
 * Procedural grunge API
 *
 * Part of the Dirt & Grunge subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DIRT_GRUNGE_GRUNGE_MASKS_H
#define DIRT_GRUNGE_GRUNGE_MASKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum grunge_type {
    GRUNGE_TYPE_STREAKS,    // Vertical streaks from rain/dripping
    GRUNGE_TYPE_SPOTS,      // Small random spots/splatters
    GRUNGE_TYPE_STAIN,      // Large diffused stains
    GRUNGE_TYPE_COUNT
} grunge_type_t;

typedef struct grunge_params {
    grunge_type_t type;
    float intensity;
    float frequency;        // Noise frequency
    float orientation;      // 0-1 (e.g., vertical bias for streaks)
} grunge_params_t;

/**
 * Generate a procedural grunge mask for a given world position and normal.
 */
float grunge_generate_mask(const grunge_params_t* params, vec3_t world_pos, vec3_t normal);

#ifdef __cplusplus
}
#endif

#endif /* DIRT_GRUNGE_GRUNGE_MASKS_H */
