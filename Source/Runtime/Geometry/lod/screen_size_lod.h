/*
 * screen_size_lod.h
 * Screen-size based LOD
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_SCREEN_SIZE_LOD_H
#define GEOMETRY_SCREEN_SIZE_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_screen_size_lod_handle {
    uint32_t id;
} geometry_screen_size_lod_handle_t;

typedef struct geometry_screen_size_lod_desc {
    uint32_t flags;
    void* user_data;
} geometry_screen_size_lod_desc_t;

typedef struct geometry_screen_size_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_screen_size_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_screen_size_lod_init(void);
void geometry_screen_size_lod_shutdown(void);

/* Lifecycle */
int geometry_screen_size_lod_create(geometry_screen_size_lod_handle_t* out_handle, const geometry_screen_size_lod_desc_t* desc);
void geometry_screen_size_lod_destroy(geometry_screen_size_lod_handle_t handle);

/* Operations */
int geometry_screen_size_lod_update(geometry_screen_size_lod_handle_t handle, const void* data, size_t size);
bool geometry_screen_size_lod_is_valid(geometry_screen_size_lod_handle_t handle);
int geometry_screen_size_lod_get_info(geometry_screen_size_lod_handle_t handle, geometry_screen_size_lod_info_t* out_info);
void geometry_screen_size_lod_mark_dirty(geometry_screen_size_lod_handle_t handle);
int geometry_screen_size_lod_process_pending(void);

/* Statistics */
uint32_t geometry_screen_size_lod_get_count(void);
size_t geometry_screen_size_lod_get_memory_usage(void);
void geometry_screen_size_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_SCREEN_SIZE_LOD_H */
