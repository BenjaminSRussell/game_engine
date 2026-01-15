/*
 * particle_sorting.h
 * Depth-sorted particles
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_SORTING_H
#define EFFECTS_PARTICLE_SORTING_H

#include "effects/gpu_particles/particle_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_sorting_handle {
    uint32_t id;
} effects_particle_sorting_handle_t;

typedef struct effects_particle_sorting_desc {
    // Sorting configuration
    bool enable_gpu_sort;
    uint32_t max_particles;
    
    uint32_t flags;
    void* user_data;
} effects_particle_sorting_desc_t;

typedef struct effects_particle_sorting_info {
    uint32_t id;
    bool sorted;
    bool initialized;
} effects_particle_sorting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_particle_sorting_init(void);
void effects_particle_sorting_shutdown(void);

/* Lifecycle */
int effects_particle_sorting_create(effects_particle_sorting_handle_t* out_handle, const effects_particle_sorting_desc_t* desc);
void effects_particle_sorting_destroy(effects_particle_sorting_handle_t handle);

/* Operations */
int effects_particle_sorting_update(effects_particle_sorting_handle_t handle, const void* data, size_t size);
bool effects_particle_sorting_is_valid(effects_particle_sorting_handle_t handle);
int effects_particle_sorting_get_info(effects_particle_sorting_handle_t handle, effects_particle_sorting_info_t* out_info);
void effects_particle_sorting_mark_dirty(effects_particle_sorting_handle_t handle);
int effects_particle_sorting_process_pending(void);

/* Statistics */
uint32_t effects_particle_sorting_get_count(void);
size_t effects_particle_sorting_get_memory_usage(void);
void effects_particle_sorting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_SORTING_H */
