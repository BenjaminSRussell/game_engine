/*
 * mipmap_generation.h
 * Mipmap generation compute
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_MIPMAP_GENERATION_H
#define TEXTURE_MIPMAP_GENERATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_mipmap_generation_handle {
    uint32_t id;
} texture_mipmap_generation_handle_t;

typedef struct texture_mipmap_generation_desc {
    uint32_t flags;
    void* user_data;
} texture_mipmap_generation_desc_t;

typedef struct texture_mipmap_generation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_mipmap_generation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_mipmap_generation_init(void);
void texture_mipmap_generation_shutdown(void);

/* Lifecycle */
int texture_mipmap_generation_create(texture_mipmap_generation_handle_t* out_handle, const texture_mipmap_generation_desc_t* desc);
void texture_mipmap_generation_destroy(texture_mipmap_generation_handle_t handle);

/* Operations */
int texture_mipmap_generation_update(texture_mipmap_generation_handle_t handle, const void* data, size_t size);
bool texture_mipmap_generation_is_valid(texture_mipmap_generation_handle_t handle);
int texture_mipmap_generation_get_info(texture_mipmap_generation_handle_t handle, texture_mipmap_generation_info_t* out_info);
void texture_mipmap_generation_mark_dirty(texture_mipmap_generation_handle_t handle);
int texture_mipmap_generation_process_pending(void);

/* Statistics */
uint32_t texture_mipmap_generation_get_count(void);
size_t texture_mipmap_generation_get_memory_usage(void);
void texture_mipmap_generation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_MIPMAP_GENERATION_H */
