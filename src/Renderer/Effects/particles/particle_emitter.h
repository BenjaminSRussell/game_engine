#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H

#include <stdbool.h>
#include "core/types.h"
#include "effects/particles/particle_types.h"

// Particle emitter structure
typedef struct ParticleEmitter {
    emitter_params_t params;
    float time_since_last_emit;
    bool active;
    u32 particle_count;
    u32 max_particles;
    particle_t* particles;
} ParticleEmitter;

// Create a new particle emitter
ParticleEmitter* particle_emitter_create(const emitter_params_t* params);

// Update particle emitter
void particle_emitter_update(ParticleEmitter* emitter, float delta_time);

// Emit particles from emitter
void particle_emitter_emit(ParticleEmitter* emitter, u32 count);

// Destroy particle emitter
void particle_emitter_destroy(ParticleEmitter* emitter);

#endif /* PARTICLE_EMITTER_H */
