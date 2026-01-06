/*
 * heightmap_modification.h
 * Runtime height modification
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_HEIGHTMAP_MODIFICATION_H
#define LANDSCAPE_HEIGHTMAP_MODIFICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_heightmap_modification_handle {
    uint32_t id;
} landscape_heightmap_modification_handle_t;

typedef struct landscape_heightmap_modification_desc {
    uint32_t flags;
    void* user_data;
} landscape_heightmap_modification_desc_t;

typedef struct landscape_heightmap_modification_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_heightmap_modification_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_heightmap_modification_init(void);
void landscape_heightmap_modification_shutdown(void);

/* Lifecycle */
int landscape_heightmap_modification_create(landscape_heightmap_modification_handle_t* out_handle, const landscape_heightmap_modification_desc_t* desc);
void landscape_heightmap_modification_destroy(landscape_heightmap_modification_handle_t handle);

/* Operations */
int landscape_heightmap_modification_update(landscape_heightmap_modification_handle_t handle, const void* data, size_t size);
bool landscape_heightmap_modification_is_valid(landscape_heightmap_modification_handle_t handle);
int landscape_heightmap_modification_get_info(landscape_heightmap_modification_handle_t handle, landscape_heightmap_modification_info_t* out_info);
void landscape_heightmap_modification_mark_dirty(landscape_heightmap_modification_handle_t handle);
int landscape_heightmap_modification_process_pending(void);

/* Statistics */
uint32_t landscape_heightmap_modification_get_count(void);
size_t landscape_heightmap_modification_get_memory_usage(void);
void landscape_heightmap_modification_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_HEIGHTMAP_MODIFICATION_H */
