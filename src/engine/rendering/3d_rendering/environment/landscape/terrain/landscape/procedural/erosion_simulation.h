/*
 * erosion_simulation.h
 * Erosion simulation
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_EROSION_SIMULATION_H
#define LANDSCAPE_EROSION_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_erosion_simulation_handle {
    uint32_t id;
} landscape_erosion_simulation_handle_t;

typedef struct landscape_erosion_simulation_desc {
    uint32_t flags;
    void* user_data;
} landscape_erosion_simulation_desc_t;

typedef struct landscape_erosion_simulation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_erosion_simulation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_erosion_simulation_init(void);
void landscape_erosion_simulation_shutdown(void);

/* Lifecycle */
int landscape_erosion_simulation_create(landscape_erosion_simulation_handle_t* out_handle, const landscape_erosion_simulation_desc_t* desc);
void landscape_erosion_simulation_destroy(landscape_erosion_simulation_handle_t handle);

/* Operations */
int landscape_erosion_simulation_update(landscape_erosion_simulation_handle_t handle, const void* data, size_t size);
bool landscape_erosion_simulation_is_valid(landscape_erosion_simulation_handle_t handle);
int landscape_erosion_simulation_get_info(landscape_erosion_simulation_handle_t handle, landscape_erosion_simulation_info_t* out_info);
void landscape_erosion_simulation_mark_dirty(landscape_erosion_simulation_handle_t handle);
int landscape_erosion_simulation_process_pending(void);

/* Statistics */
uint32_t landscape_erosion_simulation_get_count(void);
size_t landscape_erosion_simulation_get_memory_usage(void);
void landscape_erosion_simulation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_EROSION_SIMULATION_H */
