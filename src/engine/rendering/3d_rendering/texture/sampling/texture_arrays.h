/*
 * texture_arrays.h
 * Texture array management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_TEXTURE_ARRAYS_H
#define TEXTURE_TEXTURE_ARRAYS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_texture_arrays_handle {
    uint32_t id;
} texture_texture_arrays_handle_t;

typedef struct texture_texture_arrays_desc {
    uint32_t flags;
    void* user_data;
} texture_texture_arrays_desc_t;

typedef struct texture_texture_arrays_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_texture_arrays_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_texture_arrays_init(void);
void texture_texture_arrays_shutdown(void);

/* Lifecycle */
int texture_texture_arrays_create(texture_texture_arrays_handle_t* out_handle, const texture_texture_arrays_desc_t* desc);
void texture_texture_arrays_destroy(texture_texture_arrays_handle_t handle);

/* Operations */
int texture_texture_arrays_update(texture_texture_arrays_handle_t handle, const void* data, size_t size);
bool texture_texture_arrays_is_valid(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_get_info(texture_texture_arrays_handle_t handle, texture_texture_arrays_info_t* out_info);
void texture_texture_arrays_mark_dirty(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_process_pending(void);

/* Statistics */
uint32_t texture_texture_arrays_get_count(void);
size_t texture_texture_arrays_get_memory_usage(void);
void texture_texture_arrays_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_TEXTURE_ARRAYS_H */
