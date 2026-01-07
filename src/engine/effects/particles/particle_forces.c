/*
 * particle_forces.c
 * Particle physics forces
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#include "effects/particles/particle_forces.h"


/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void apply_gravity(const particle_force_desc_t* force, vec3_t* velocity, float dt) {
    // velocity += direction * strength * dt
    velocity->x += force->direction.x * force->strength * dt;
    velocity->y += force->direction.y * force->strength * dt;
    velocity->z += force->direction.z * force->strength * dt;
}

static void apply_wind(const particle_force_desc_t* force, vec3_t* velocity, float dt) {
    // Simple wind: push velocity towards wind vector
    // Force = (Wind - Velocity) * strength
    float fx = (force->direction.x - velocity->x) * force->strength * dt;
    float fy = (force->direction.y - velocity->y) * force->strength * dt;
    float fz = (force->direction.z - velocity->z) * force->strength * dt;
    velocity->x += fx;
    velocity->y += fy;
    velocity->z += fz;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void effects_particle_forces_apply(const particle_force_desc_t* force, vec3_t* position, vec3_t* velocity, float dt) {
    if (!force || !position || !velocity) return;

    switch (force->type) {
        case PARTICLE_FORCE_GRAVITY:
            apply_gravity(force, velocity, dt);
            break;
        case PARTICLE_FORCE_WIND:
            apply_wind(force, velocity, dt);
            break;
        case PARTICLE_FORCE_ATTRACTOR:
            // TODO: Implement attractor
            break;
        default:
            break;
    }
}
