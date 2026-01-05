/*
 * froxel_grid.h
 * Froxel volume allocation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_FROXEL_GRID_H
#define LIGHTING_FROXEL_GRID_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_froxel_grid_handle {
    uint32_t id;
} lighting_froxel_grid_handle_t;

typedef struct lighting_froxel_grid_desc {
    uint32_t flags;
    void* user_data;
} lighting_froxel_grid_desc_t;

typedef struct lighting_froxel_grid_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_froxel_grid_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_froxel_grid_init(void);
void lighting_froxel_grid_shutdown(void);

/* Lifecycle */
int lighting_froxel_grid_create(lighting_froxel_grid_handle_t* out_handle, const lighting_froxel_grid_desc_t* desc);
void lighting_froxel_grid_destroy(lighting_froxel_grid_handle_t handle);

/* Operations */
int lighting_froxel_grid_update(lighting_froxel_grid_handle_t handle, const void* data, size_t size);
bool lighting_froxel_grid_is_valid(lighting_froxel_grid_handle_t handle);
int lighting_froxel_grid_get_info(lighting_froxel_grid_handle_t handle, lighting_froxel_grid_info_t* out_info);
void lighting_froxel_grid_mark_dirty(lighting_froxel_grid_handle_t handle);
int lighting_froxel_grid_process_pending(void);

/* Statistics */
uint32_t lighting_froxel_grid_get_count(void);
size_t lighting_froxel_grid_get_memory_usage(void);
void lighting_froxel_grid_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_FROXEL_GRID_H */
