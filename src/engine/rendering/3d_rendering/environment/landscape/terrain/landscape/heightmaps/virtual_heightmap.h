/*
 * virtual_heightmap.h
 * Virtual heightmap
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_VIRTUAL_HEIGHTMAP_H
#define LANDSCAPE_VIRTUAL_HEIGHTMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_virtual_heightmap_handle {
    uint32_t id;
} landscape_virtual_heightmap_handle_t;

typedef struct landscape_virtual_heightmap_desc {
    uint32_t flags;
    void* user_data;
} landscape_virtual_heightmap_desc_t;

typedef struct landscape_virtual_heightmap_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_virtual_heightmap_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_virtual_heightmap_init(void);
void landscape_virtual_heightmap_shutdown(void);

/* Lifecycle */
int landscape_virtual_heightmap_create(landscape_virtual_heightmap_handle_t* out_handle, const landscape_virtual_heightmap_desc_t* desc);
void landscape_virtual_heightmap_destroy(landscape_virtual_heightmap_handle_t handle);

/* Operations */
int landscape_virtual_heightmap_update(landscape_virtual_heightmap_handle_t handle, const void* data, size_t size);
bool landscape_virtual_heightmap_is_valid(landscape_virtual_heightmap_handle_t handle);
int landscape_virtual_heightmap_get_info(landscape_virtual_heightmap_handle_t handle, landscape_virtual_heightmap_info_t* out_info);
void landscape_virtual_heightmap_mark_dirty(landscape_virtual_heightmap_handle_t handle);
int landscape_virtual_heightmap_process_pending(void);

/* Statistics */
uint32_t landscape_virtual_heightmap_get_count(void);
size_t landscape_virtual_heightmap_get_memory_usage(void);
void landscape_virtual_heightmap_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_VIRTUAL_HEIGHTMAP_H */
