/*
 * texture_lod.h
 * Texture LOD calculation
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_TEXTURE_LOD_H
#define TEXTURE_TEXTURE_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_texture_lod_handle {
    uint32_t id;
} texture_texture_lod_handle_t;

typedef struct texture_texture_lod_desc {
    uint32_t flags;
    void* user_data;
} texture_texture_lod_desc_t;

typedef struct texture_texture_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_texture_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_texture_lod_init(void);
void texture_texture_lod_shutdown(void);

/* Lifecycle */
int texture_texture_lod_create(texture_texture_lod_handle_t* out_handle, const texture_texture_lod_desc_t* desc);
void texture_texture_lod_destroy(texture_texture_lod_handle_t handle);

/* Operations */
int texture_texture_lod_update(texture_texture_lod_handle_t handle, const void* data, size_t size);
bool texture_texture_lod_is_valid(texture_texture_lod_handle_t handle);
int texture_texture_lod_get_info(texture_texture_lod_handle_t handle, texture_texture_lod_info_t* out_info);
void texture_texture_lod_mark_dirty(texture_texture_lod_handle_t handle);
int texture_texture_lod_process_pending(void);

/* Statistics */
uint32_t texture_texture_lod_get_count(void);
size_t texture_texture_lod_get_memory_usage(void);
void texture_texture_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif // TEXTURE_TEXTURE_LOD_H
