/*
 * wave_propagation.h
 * Wave propagation
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_WAVE_PROPAGATION_H
#define WATER_WAVE_PROPAGATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_wave_propagation_handle {
    uint32_t id;
} water_wave_propagation_handle_t;

typedef struct water_wave_propagation_desc {
    uint32_t flags;
    void* user_data;
} water_wave_propagation_desc_t;

typedef struct water_wave_propagation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_wave_propagation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_wave_propagation_init(void);
void water_wave_propagation_shutdown(void);

/* Lifecycle */
int water_wave_propagation_create(water_wave_propagation_handle_t* out_handle, const water_wave_propagation_desc_t* desc);
void water_wave_propagation_destroy(water_wave_propagation_handle_t handle);

/* Operations */
int water_wave_propagation_update(water_wave_propagation_handle_t handle, const void* data, size_t size);
bool water_wave_propagation_is_valid(water_wave_propagation_handle_t handle);
int water_wave_propagation_get_info(water_wave_propagation_handle_t handle, water_wave_propagation_info_t* out_info);
void water_wave_propagation_mark_dirty(water_wave_propagation_handle_t handle);
int water_wave_propagation_process_pending(void);

/* Statistics */
uint32_t water_wave_propagation_get_count(void);
size_t water_wave_propagation_get_memory_usage(void);
void water_wave_propagation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WAVE_PROPAGATION_H */
