/*
 * virtual_texture.h
 * Virtual texture system
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_VIRTUAL_TEXTURE_H
#define TEXTURE_VIRTUAL_TEXTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_virtual_texture_handle {
    uint32_t id;
} texture_virtual_texture_handle_t;

typedef struct texture_virtual_texture_desc {
    uint32_t flags;
    void* user_data;
} texture_virtual_texture_desc_t;

typedef struct texture_virtual_texture_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_virtual_texture_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_virtual_texture_init(void);
void texture_virtual_texture_shutdown(void);

/* Lifecycle */
int texture_virtual_texture_create(texture_virtual_texture_handle_t* out_handle, const texture_virtual_texture_desc_t* desc);
void texture_virtual_texture_destroy(texture_virtual_texture_handle_t handle);

/* Operations */
int texture_virtual_texture_update(texture_virtual_texture_handle_t handle, const void* data, size_t size);
bool texture_virtual_texture_is_valid(texture_virtual_texture_handle_t handle);
int texture_virtual_texture_get_info(texture_virtual_texture_handle_t handle, texture_virtual_texture_info_t* out_info);
void texture_virtual_texture_mark_dirty(texture_virtual_texture_handle_t handle);
int texture_virtual_texture_process_pending(void);

/* Statistics */
uint32_t texture_virtual_texture_get_count(void);
size_t texture_virtual_texture_get_memory_usage(void);
void texture_virtual_texture_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_VIRTUAL_TEXTURE_H */
