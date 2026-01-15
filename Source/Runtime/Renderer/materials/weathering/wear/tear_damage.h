/*
 * tear_damage.h
 * Damage patterns API
 *
 * Part of the Wear & Tear subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WEAR_TEAR_TEAR_DAMAGE_H
#define WEAR_TEAR_TEAR_DAMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum damage_type {
    DAMAGE_TYPE_SCRATCH,
    DAMAGE_TYPE_DENT,
    DAMAGE_TYPE_CRACK,
    DAMAGE_TYPE_COUNT
} damage_type_t;

typedef struct damage_params {
    damage_type_t type;
    float seed;
    float scale;
    float depth;
} damage_params_t;

/**
 * Generate a damage mask (displacement/intensity) for a given point.
 */
float damage_generate_pattern(const damage_params_t* params, vec3_t world_pos);

#ifdef __cplusplus
}
#endif

#endif /* WEAR_TEAR_TEAR_DAMAGE_H */
