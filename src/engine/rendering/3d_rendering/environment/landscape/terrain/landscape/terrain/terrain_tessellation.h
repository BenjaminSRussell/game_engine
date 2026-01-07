/*
 * terrain_tessellation.h
 * GPU tessellation
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_TERRAIN_TESSELLATION_H
#define LANDSCAPE_TERRAIN_TESSELLATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_tessellation_handle {
    uint32_t id;
} landscape_terrain_tessellation_handle_t;

typedef struct landscape_terrain_tessellation_desc {
    uint32_t flags;
    void* user_data;
} landscape_terrain_tessellation_desc_t;

typedef struct landscape_terrain_tessellation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_terrain_tessellation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_tessellation_init(void);
void landscape_terrain_tessellation_shutdown(void);

/* Lifecycle */
int landscape_terrain_tessellation_create(landscape_terrain_tessellation_handle_t* out_handle, const landscape_terrain_tessellation_desc_t* desc);
void landscape_terrain_tessellation_destroy(landscape_terrain_tessellation_handle_t handle);

/* Operations */
int landscape_terrain_tessellation_update(landscape_terrain_tessellation_handle_t handle, const void* data, size_t size);
bool landscape_terrain_tessellation_is_valid(landscape_terrain_tessellation_handle_t handle);
int landscape_terrain_tessellation_get_info(landscape_terrain_tessellation_handle_t handle, landscape_terrain_tessellation_info_t* out_info);
void landscape_terrain_tessellation_mark_dirty(landscape_terrain_tessellation_handle_t handle);
int landscape_terrain_tessellation_process_pending(void);

/* Statistics */
uint32_t landscape_terrain_tessellation_get_count(void);
size_t landscape_terrain_tessellation_get_memory_usage(void);
void landscape_terrain_tessellation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_TESSELLATION_H */
