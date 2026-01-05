/*
 * gpu_particles.h
 * GPU particle compute
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_GPU_PARTICLES_H
#define EFFECTS_GPU_PARTICLES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_gpu_particles_handle {
    uint32_t id;
} effects_gpu_particles_handle_t;

typedef struct effects_gpu_particles_desc {
    uint32_t flags;
    void* user_data;
} effects_gpu_particles_desc_t;

typedef struct effects_gpu_particles_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_gpu_particles_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_gpu_particles_init(void);
void effects_gpu_particles_shutdown(void);

/* Lifecycle */
int effects_gpu_particles_create(effects_gpu_particles_handle_t* out_handle, const effects_gpu_particles_desc_t* desc);
void effects_gpu_particles_destroy(effects_gpu_particles_handle_t handle);

/* Operations */
int effects_gpu_particles_update(effects_gpu_particles_handle_t handle, const void* data, size_t size);
bool effects_gpu_particles_is_valid(effects_gpu_particles_handle_t handle);
int effects_gpu_particles_get_info(effects_gpu_particles_handle_t handle, effects_gpu_particles_info_t* out_info);
void effects_gpu_particles_mark_dirty(effects_gpu_particles_handle_t handle);
int effects_gpu_particles_process_pending(void);

/* Statistics */
uint32_t effects_gpu_particles_get_count(void);
size_t effects_gpu_particles_get_memory_usage(void);
void effects_gpu_particles_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_GPU_PARTICLES_H */
