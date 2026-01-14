#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "core/types.h"
#include "effects/particles/particle_types.h"
#include "effects/particles/particle_emitter.h"
#include "effects/particles/particle_simulation.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration for particle system
typedef struct particle_system_config {
    u32 max_particles;
    bool use_gpu;
} particle_system_config_t;

// Opaque handle to particle system
typedef struct ParticleSystem ParticleSystem;

// Initialize particle system
// Returns true on success, false on failure
bool particle_system_init(const particle_system_config_t* config);

// Shutdown particle system
void particle_system_shutdown(void);

// Get the global particle system instance
ParticleSystem* particle_system_get(void);

// Update particle system
void particle_system_update(float delta_time);

// Render particle system
// Pass view and projection matrices as void* to avoid coupling with specific math library here
// (Implementation will cast them to Mat4*)
void particle_system_render(const void* view_matrix, const void* proj_matrix);

// Create an emitter attached to the system
ParticleEmitter* particle_system_create_emitter(const emitter_params_t* params);

// Destroy an emitter
void particle_system_destroy_emitter(ParticleEmitter* emitter);

// Get particle system statistics
particle_stats_t particle_system_get_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* PARTICLE_SYSTEM_H */
