#include "svg_particle_system.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
   SVG Particle System Creation & Destruction
   ============================================================================ */

SVGParticleSystem* svg_particle_system_create(uint32_t max_emitters)
{
    SVGParticleSystem *system = (SVGParticleSystem *)malloc(
        sizeof(SVGParticleSystem)
    );
    if (!system) return NULL;

    system->emitters = (SVGParticleEmitter *)calloc(
        max_emitters,
        sizeof(SVGParticleEmitter)
    );
    if (!system->emitters) {
        free(system);
        return NULL;
    }

    system->emitter_count = 0;
    system->max_emitters = max_emitters;
    system->delta_time = 0.0f;

    return system;
}

void svg_particle_system_destroy(SVGParticleSystem *system)
{
    if (!system) return;

    for (uint32_t i = 0; i < system->emitter_count; i++) {
        svg_particle_emitter_destroy(&system->emitters[i]);
    }

    free(system->emitters);
    free(system);
}

/* ============================================================================
   SVG Particle Emitter Creation & Destruction
   ============================================================================ */

SVGParticleEmitter* svg_particle_emitter_create(
    SVGParticleEmitterConfig config
)
{
    SVGParticleEmitter *emitter = (SVGParticleEmitter *)malloc(
        sizeof(SVGParticleEmitter)
    );
    if (!emitter) return NULL;

    emitter->particles = (SVGParticle *)calloc(
        config.max_particles,
        sizeof(SVGParticle)
    );
    if (!emitter->particles) {
        free(emitter);
        return NULL;
    }

    emitter->config = config;
    emitter->particle_count = 0;
    emitter->active_count = 0;
    emitter->emission_accumulator = 0.0f;
    emitter->is_emitting = false;
    emitter->position[0] = 0.0f;
    emitter->position[1] = 0.0f;
    emitter->position[2] = 0.0f;

    return emitter;
}

void svg_particle_emitter_destroy(SVGParticleEmitter *emitter)
{
    if (!emitter) return;
    free(emitter->particles);
    free(emitter);
}

/* ============================================================================
   System Management
   ============================================================================ */

bool svg_particle_system_add_emitter(
    SVGParticleSystem *system,
    SVGParticleEmitter *emitter
)
{
    if (!system || !emitter) return false;
    if (system->emitter_count >= system->max_emitters) return false;

    system->emitters[system->emitter_count++] = *emitter;
    return true;
}

void svg_particle_emitter_set_position(
    SVGParticleEmitter *emitter,
    float x, float y, float z
)
{
    if (!emitter) return;
    emitter->position[0] = x;
    emitter->position[1] = y;
    emitter->position[2] = z;
}

/* ============================================================================
   Particle Spawning
   ============================================================================ */

static float _random_range(float min, float max)
{
    float r = (float)rand() / RAND_MAX;
    return min + r * (max - min);
}

static void _spawn_particle(
    SVGParticleEmitter *emitter,
    SVGParticle *particle,
    SVGParticleSpawnParams spawn_params
)
{
    particle->x = spawn_params.x;
    particle->y = spawn_params.y;
    particle->z = spawn_params.z;

    /* Add random spread to velocity */
    float spread = _random_range(-1.0f, 1.0f);
    particle->vx = spawn_params.velocity_x + spread * spawn_params.velocity_spread;
    particle->vy = spawn_params.velocity_y + spread * spawn_params.velocity_spread;
    particle->vz = spawn_params.velocity_z + spread * spawn_params.velocity_spread;

    particle->scale = _random_range(
        emitter->config.scale_min,
        emitter->config.scale_max
    );
    particle->rotation = _random_range(0.0f, 2.0f * 3.14159f);

    particle->lifetime = 0.0f;
    particle->max_lifetime = _random_range(
        emitter->config.lifetime_min,
        emitter->config.lifetime_max
    );

    particle->opacity = emitter->config.opacity;
    particle->color[0] = 1.0f;  /* White by default */
    particle->color[1] = 1.0f;
    particle->color[2] = 1.0f;
    particle->color[3] = emitter->config.opacity;

    particle->active = true;
}

void svg_particle_emitter_start(SVGParticleEmitter *emitter)
{
    if (!emitter) return;
    emitter->is_emitting = true;
}

void svg_particle_emitter_stop(SVGParticleEmitter *emitter)
{
    if (!emitter) return;
    emitter->is_emitting = false;
}

void svg_particle_emitter_burst(
    SVGParticleEmitter *emitter,
    uint32_t count
)
{
    if (!emitter) return;

    count = (count > emitter->config.max_particles)
        ? emitter->config.max_particles : count;

    SVGParticleSpawnParams params = {
        .x = emitter->position[0],
        .y = emitter->position[1],
        .z = emitter->position[2],
        .velocity_x = 0.0f,
        .velocity_y = 0.0f,
        .velocity_z = 0.0f,
        .velocity_spread = 0.5f
    };

    for (uint32_t i = 0; i < count && emitter->particle_count < emitter->config.max_particles; i++) {
        _spawn_particle(emitter, &emitter->particles[emitter->particle_count++], params);
        emitter->active_count++;
    }
}

/* ============================================================================
   Particle Physics & Updates
   ============================================================================ */

static void _update_particle(SVGParticle *particle, float delta_time, float gravity, float drag)
{
    if (!particle->active) return;

    /* Update lifetime */
    particle->lifetime += delta_time;

    /* Check if particle has expired */
    if (particle->lifetime >= particle->max_lifetime) {
        particle->active = false;
        return;
    }

    /* Apply gravity */
    particle->vy -= gravity * delta_time;

    /* Apply drag */
    particle->vx *= (1.0f - drag * delta_time);
    particle->vy *= (1.0f - drag * delta_time);
    particle->vz *= (1.0f - drag * delta_time);

    /* Update position */
    particle->x += particle->vx * delta_time;
    particle->y += particle->vy * delta_time;
    particle->z += particle->vz * delta_time;

    /* Fade opacity over lifetime */
    float life_ratio = particle->lifetime / particle->max_lifetime;
    particle->opacity = particle->color[3] * (1.0f - life_ratio * life_ratio);
}

void svg_particle_system_update(
    SVGParticleSystem *system,
    float delta_time
)
{
    if (!system) return;

    system->delta_time = delta_time;

    for (uint32_t i = 0; i < system->emitter_count; i++) {
        SVGParticleEmitter *emitter = &system->emitters[i];

        /* Handle emission */
        if (emitter->is_emitting) {
            emitter->emission_accumulator += emitter->config.spawn_rate * delta_time;

            uint32_t particles_to_spawn = (uint32_t)emitter->emission_accumulator;
            if (particles_to_spawn > 0) {
                svg_particle_emitter_burst(emitter, particles_to_spawn);
                emitter->emission_accumulator -= particles_to_spawn;
            }
        }

        /* Update particles */
        uint32_t active_count = 0;
        for (uint32_t j = 0; j < emitter->particle_count; j++) {
            _update_particle(
                &emitter->particles[j],
                delta_time,
                emitter->config.gravity,
                emitter->config.drag
            );

            if (emitter->particles[j].active) {
                active_count++;
            }
        }
        emitter->active_count = active_count;
    }
}

/* ============================================================================
   Particle Access
   ============================================================================ */

uint32_t svg_particle_emitter_get_active_count(
    const SVGParticleEmitter *emitter
)
{
    if (!emitter) return 0;
    return emitter->active_count;
}

const SVGParticle* svg_particle_emitter_get_particles(
    const SVGParticleEmitter *emitter,
    uint32_t *out_count
)
{
    if (!emitter || !out_count) return NULL;

    *out_count = emitter->particle_count;
    return emitter->particles;
}
