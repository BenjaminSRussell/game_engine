/*
 * terrain_holes.h
 * Terrain hole support
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_TERRAIN_HOLES_H
#define LANDSCAPE_TERRAIN_HOLES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_holes_handle {
    uint32_t id;
} landscape_terrain_holes_handle_t;

typedef struct landscape_terrain_holes_desc {
    uint32_t flags;
    void* user_data;
} landscape_terrain_holes_desc_t;

typedef struct landscape_terrain_holes_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_terrain_holes_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_holes_init(void);
void landscape_terrain_holes_shutdown(void);

/* Lifecycle */
int landscape_terrain_holes_create(landscape_terrain_holes_handle_t* out_handle, const landscape_terrain_holes_desc_t* desc);
void landscape_terrain_holes_destroy(landscape_terrain_holes_handle_t handle);

/* Operations */
int landscape_terrain_holes_update(landscape_terrain_holes_handle_t handle, const void* data, size_t size);
bool landscape_terrain_holes_is_valid(landscape_terrain_holes_handle_t handle);
int landscape_terrain_holes_get_info(landscape_terrain_holes_handle_t handle, landscape_terrain_holes_info_t* out_info);
void landscape_terrain_holes_mark_dirty(landscape_terrain_holes_handle_t handle);
int landscape_terrain_holes_process_pending(void);

/* Statistics */
uint32_t landscape_terrain_holes_get_count(void);
size_t landscape_terrain_holes_get_memory_usage(void);
void landscape_terrain_holes_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_HOLES_H */
