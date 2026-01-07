/*
 * update_compute.h
 * Particle update compute
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_UPDATE_COMPUTE_H
#define EFFECTS_UPDATE_COMPUTE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_update_compute_handle {
    uint32_t id;
} effects_update_compute_handle_t;

typedef struct effects_update_compute_desc {
    uint32_t flags;
    void* user_data;
} effects_update_compute_desc_t;

typedef struct effects_update_compute_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_update_compute_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_update_compute_init(void);
void effects_update_compute_shutdown(void);

/* Lifecycle */
int effects_update_compute_create(effects_update_compute_handle_t* out_handle, const effects_update_compute_desc_t* desc);
void effects_update_compute_destroy(effects_update_compute_handle_t handle);

/* Operations */
int effects_update_compute_update(effects_update_compute_handle_t handle, const void* data, size_t size);
bool effects_update_compute_is_valid(effects_update_compute_handle_t handle);
int effects_update_compute_get_info(effects_update_compute_handle_t handle, effects_update_compute_info_t* out_info);
void effects_update_compute_mark_dirty(effects_update_compute_handle_t handle);
int effects_update_compute_process_pending(void);

/* Statistics */
uint32_t effects_update_compute_get_count(void);
size_t effects_update_compute_get_memory_usage(void);
void effects_update_compute_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_UPDATE_COMPUTE_H */
