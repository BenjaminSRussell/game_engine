#ifndef SVG_PARTICLE_SYSTEM_H
#define SVG_PARTICLE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "../../editor/importer/svg_importer.h"

/**
 * SVG Particle System - Renders SVG shapes as particles
 * Minimum viable implementation with essential features:
 * - SVG shape-based particle rendering
 * - Physics simulation (gravity, drag, collision)
 * - Color/opacity animation
 * - Scale animation
 * - Pooled particle allocation
 */

/* ============================================================================
   SVG Particle Emitter Configuration
   ============================================================================ */

typedef struct {
    float x, y, z;                    /* Emitter position */
    float velocity_x, velocity_y, velocity_z;  /* Base velocity */
    float velocity_spread;             /* Random velocity spread */
} SVGParticleSpawnParams;

typedef struct {
    uint32_t max_particles;            /* Maximum particles in pool */
    uint32_t spawn_rate;               /* Particles per second */
    float lifetime_min, lifetime_max;   /* Particle lifetime range */
    float scale_min, scale_max;         /* Scale range */
    float opacity;                      /* Initial opacity */

    /* Physics */
    float gravity;                     /* Gravity strength */
    float drag;                        /* Velocity drag */

    /* SVG Configuration */
    SVGElement *svg_shape;              /* SVG shape to render */
    uint32_t svg_element_count;         /* Number of elements */
} SVGParticleEmitterConfig;

/* ============================================================================
   SVG Particle Instance
   ============================================================================ */

typedef struct {
    /* Transform */
    float x, y, z;
    float vx, vy, vz;
    float scale;
    float rotation;

    /* Lifetime */
    float lifetime;
    float max_lifetime;

    /* Visual */
    float opacity;
    float color[4];  /* RGBA */

    /* Active flag */
    bool active;
} SVGParticle;

/* ============================================================================
   SVG Particle Emitter
   ============================================================================ */

typedef struct {
    SVGParticleEmitterConfig config;

    /* Particle pool */
    SVGParticle *particles;
    uint32_t particle_count;
    uint32_t active_count;

    /* Emission state */
    float emission_accumulator;
    bool is_emitting;

    /* Transform */
    float position[3];
} SVGParticleEmitter;

/* ============================================================================
   SVG Particle System
   ============================================================================ */

typedef struct {
    SVGParticleEmitter *emitters;
    uint32_t emitter_count;
    uint32_t max_emitters;

    /* Frame timing */
    float delta_time;
} SVGParticleSystem;

/* ============================================================================
   Public API
   ============================================================================ */

/**
 * Create a new SVG particle system
 */
SVGParticleSystem* svg_particle_system_create(uint32_t max_emitters);

/**
 * Destroy SVG particle system
 */
void svg_particle_system_destroy(SVGParticleSystem *system);

/**
 * Create a new SVG particle emitter
 */
SVGParticleEmitter* svg_particle_emitter_create(
    SVGParticleEmitterConfig config
);

/**
 * Destroy SVG particle emitter
 */
void svg_particle_emitter_destroy(SVGParticleEmitter *emitter);

/**
 * Add emitter to system
 */
bool svg_particle_system_add_emitter(
    SVGParticleSystem *system,
    SVGParticleEmitter *emitter
);

/**
 * Start emission
 */
void svg_particle_emitter_start(SVGParticleEmitter *emitter);

/**
 * Stop emission
 */
void svg_particle_emitter_stop(SVGParticleEmitter *emitter);

/**
 * Update particles (physics, lifetime)
 */
void svg_particle_system_update(
    SVGParticleSystem *system,
    float delta_time
);

/**
 * Set emitter position
 */
void svg_particle_emitter_set_position(
    SVGParticleEmitter *emitter,
    float x, float y, float z
);

/**
 * Burst emission (spawn particles instantly)
 */
void svg_particle_emitter_burst(
    SVGParticleEmitter *emitter,
    uint32_t count
);

/**
 * Get active particle count
 */
uint32_t svg_particle_emitter_get_active_count(
    const SVGParticleEmitter *emitter
);

/**
 * Get particles for rendering
 */
const SVGParticle* svg_particle_emitter_get_particles(
    const SVGParticleEmitter *emitter,
    uint32_t *out_count
);

#endif /* SVG_PARTICLE_SYSTEM_H */
