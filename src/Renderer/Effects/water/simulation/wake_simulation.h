/*
 * wake_simulation.h
 * Boat wake simulation
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_WAKE_SIMULATION_H
#define WATER_WAKE_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_wake_simulation_handle {
    uint32_t id;
} water_wake_simulation_handle_t;

typedef struct water_wake_simulation_desc {
    uint32_t flags;
    void* user_data;
} water_wake_simulation_desc_t;

typedef struct water_wake_simulation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_wake_simulation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_wake_simulation_init(void);
void water_wake_simulation_shutdown(void);

/* Lifecycle */
int water_wake_simulation_create(water_wake_simulation_handle_t* out_handle, const water_wake_simulation_desc_t* desc);
void water_wake_simulation_destroy(water_wake_simulation_handle_t handle);

/* Operations */
int water_wake_simulation_update(water_wake_simulation_handle_t handle, const void* data, size_t size);
bool water_wake_simulation_is_valid(water_wake_simulation_handle_t handle);
int water_wake_simulation_get_info(water_wake_simulation_handle_t handle, water_wake_simulation_info_t* out_info);
void water_wake_simulation_mark_dirty(water_wake_simulation_handle_t handle);
int water_wake_simulation_process_pending(void);

/* Statistics */
uint32_t water_wake_simulation_get_count(void);
size_t water_wake_simulation_get_memory_usage(void);
void water_wake_simulation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WAKE_SIMULATION_H */
