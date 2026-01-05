/*
 * terrain_culling.h
 * Terrain chunk culling
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_TERRAIN_CULLING_H
#define LANDSCAPE_TERRAIN_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_culling_handle {
    uint32_t id;
} landscape_terrain_culling_handle_t;

typedef struct landscape_terrain_culling_desc {
    uint32_t flags;
    void* user_data;
} landscape_terrain_culling_desc_t;

typedef struct landscape_terrain_culling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_terrain_culling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_culling_init(void);
void landscape_terrain_culling_shutdown(void);

/* Lifecycle */
int landscape_terrain_culling_create(landscape_terrain_culling_handle_t* out_handle, const landscape_terrain_culling_desc_t* desc);
void landscape_terrain_culling_destroy(landscape_terrain_culling_handle_t handle);

/* Operations */
int landscape_terrain_culling_update(landscape_terrain_culling_handle_t handle, const void* data, size_t size);
bool landscape_terrain_culling_is_valid(landscape_terrain_culling_handle_t handle);
int landscape_terrain_culling_get_info(landscape_terrain_culling_handle_t handle, landscape_terrain_culling_info_t* out_info);
void landscape_terrain_culling_mark_dirty(landscape_terrain_culling_handle_t handle);
int landscape_terrain_culling_process_pending(void);

/* Statistics */
uint32_t landscape_terrain_culling_get_count(void);
size_t landscape_terrain_culling_get_memory_usage(void);
void landscape_terrain_culling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_CULLING_H */
