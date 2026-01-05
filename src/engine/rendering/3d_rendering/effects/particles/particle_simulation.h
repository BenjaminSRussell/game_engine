/*
 * particle_simulation.h
 * CPU particle simulation
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_SIMULATION_H
#define EFFECTS_PARTICLE_SIMULATION_H

#include "particle_emitter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_simulation_handle {
    uint32_t id;
} effects_particle_simulation_handle_t;

typedef struct effects_particle_simulation_desc {
    uint32_t max_particles;
    bool enable_gpu_simulation;
    uint32_t flags;
    void* user_data;
} effects_particle_simulation_desc_t;

typedef struct effects_particle_simulation_info {
    uint32_t id;
    uint32_t active_particle_count;
    uint32_t max_particles;
    bool using_gpu;
    bool initialized;
} effects_particle_simulation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_particle_simulation_init(void);
void effects_particle_simulation_shutdown(void);

/* Lifecycle */
int effects_particle_simulation_create(effects_particle_simulation_handle_t* out_handle, const effects_particle_simulation_desc_t* desc);
void effects_particle_simulation_destroy(effects_particle_simulation_handle_t handle);

/* Operations */
int effects_particle_simulation_update(effects_particle_simulation_handle_t handle, float dt);
int effects_particle_simulation_spawn(effects_particle_simulation_handle_t handle, const emitter_params_t* emitter, float dt);
bool effects_particle_simulation_is_valid(effects_particle_simulation_handle_t handle);
int effects_particle_simulation_get_info(effects_particle_simulation_handle_t handle, effects_particle_simulation_info_t* out_info);
void effects_particle_simulation_mark_dirty(effects_particle_simulation_handle_t handle);
int effects_particle_simulation_process_pending(void);

/* Data Access */
const particle_t* effects_particle_simulation_get_particles(effects_particle_simulation_handle_t handle, uint32_t* out_count);

/* Statistics */
uint32_t effects_particle_simulation_get_count(void);
size_t effects_particle_simulation_get_memory_usage(void);
void effects_particle_simulation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_SIMULATION_H */
