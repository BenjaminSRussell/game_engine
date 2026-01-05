/*
 * particle_buffer.h
 * GPU particle buffer
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_BUFFER_H
#define EFFECTS_PARTICLE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_buffer_handle {
    uint32_t id;
} effects_particle_buffer_handle_t;

typedef struct effects_particle_buffer_desc {
    uint32_t flags;
    void* user_data;
} effects_particle_buffer_desc_t;

typedef struct effects_particle_buffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_particle_buffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_particle_buffer_init(void);
void effects_particle_buffer_shutdown(void);

/* Lifecycle */
int effects_particle_buffer_create(effects_particle_buffer_handle_t* out_handle, const effects_particle_buffer_desc_t* desc);
void effects_particle_buffer_destroy(effects_particle_buffer_handle_t handle);

/* Operations */
int effects_particle_buffer_update(effects_particle_buffer_handle_t handle, const void* data, size_t size);
bool effects_particle_buffer_is_valid(effects_particle_buffer_handle_t handle);
int effects_particle_buffer_get_info(effects_particle_buffer_handle_t handle, effects_particle_buffer_info_t* out_info);
void effects_particle_buffer_mark_dirty(effects_particle_buffer_handle_t handle);
int effects_particle_buffer_process_pending(void);

/* Statistics */
uint32_t effects_particle_buffer_get_count(void);
size_t effects_particle_buffer_get_memory_usage(void);
void effects_particle_buffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_BUFFER_H */
