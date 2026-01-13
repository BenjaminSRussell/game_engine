/*
 * river_rendering.h
 * River surface rendering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_RIVER_RENDERING_H
#define WATER_RIVER_RENDERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_river_rendering_handle {
    uint32_t id;
} water_river_rendering_handle_t;

typedef struct water_river_rendering_desc {
    uint32_t flags;
    void* user_data;
} water_river_rendering_desc_t;

typedef struct water_river_rendering_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_river_rendering_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_river_rendering_init(void);
void water_river_rendering_shutdown(void);

/* Lifecycle */
int water_river_rendering_create(water_river_rendering_handle_t* out_handle, const water_river_rendering_desc_t* desc);
void water_river_rendering_destroy(water_river_rendering_handle_t handle);

/* Operations */
int water_river_rendering_update(water_river_rendering_handle_t handle, const void* data, size_t size);
bool water_river_rendering_is_valid(water_river_rendering_handle_t handle);
int water_river_rendering_get_info(water_river_rendering_handle_t handle, water_river_rendering_info_t* out_info);
void water_river_rendering_mark_dirty(water_river_rendering_handle_t handle);
int water_river_rendering_process_pending(void);

/* Statistics */
uint32_t water_river_rendering_get_count(void);
size_t water_river_rendering_get_memory_usage(void);
void water_river_rendering_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_RIVER_RENDERING_H */
