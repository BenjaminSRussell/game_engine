/*
 * particle_forces.h
 * Particle physics forces (Gravity, Wind, Vortex, etc.)
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_FORCES_H
#define EFFECTS_PARTICLE_FORCES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum particle_force_type {
    PARTICLE_FORCE_GRAVITY,
    PARTICLE_FORCE_WIND,
    PARTICLE_FORCE_VORTEX,
    PARTICLE_FORCE_ATTRACTOR,
    PARTICLE_FORCE_TURBULENCE,
    PARTICLE_FORCE_COUNT
} particle_force_type_t;

typedef struct particle_force_desc {
    particle_force_type_t type;
    vec3_t direction; // For gravity/wind
    vec3_t center;    // For vortex/attractor
    float strength;
    float frequency;  // For turbulence
    float radius;     // For attractor
} particle_force_desc_t;

/* ============================================================================
 * API
 * ============================================================================ */

void effects_particle_forces_apply(const particle_force_desc_t* force, vec3_t* position, vec3_t* velocity, float dt);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_FORCES_H */
