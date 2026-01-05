/*
 * mtl_texture.h
 * Metal textures
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_TEXTURE_H
#define PLATFORM_MTL_TEXTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_mtl_texture_handle {
    uint32_t id;
} platform_mtl_texture_handle_t;

typedef struct platform_mtl_texture_desc {
    uint32_t flags;
    void* user_data;
} platform_mtl_texture_desc_t;

typedef struct platform_mtl_texture_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} platform_mtl_texture_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int platform_mtl_texture_init(void);
void platform_mtl_texture_shutdown(void);

/* Lifecycle */
int platform_mtl_texture_create(platform_mtl_texture_handle_t* out_handle, const platform_mtl_texture_desc_t* desc);
void platform_mtl_texture_destroy(platform_mtl_texture_handle_t handle);

/* Operations */
int platform_mtl_texture_update(platform_mtl_texture_handle_t handle, const void* data, size_t size);
bool platform_mtl_texture_is_valid(platform_mtl_texture_handle_t handle);
int platform_mtl_texture_get_info(platform_mtl_texture_handle_t handle, platform_mtl_texture_info_t* out_info);
void platform_mtl_texture_mark_dirty(platform_mtl_texture_handle_t handle);
int platform_mtl_texture_process_pending(void);

/* Statistics */
uint32_t platform_mtl_texture_get_count(void);
size_t platform_mtl_texture_get_memory_usage(void);
void platform_mtl_texture_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_TEXTURE_H */
