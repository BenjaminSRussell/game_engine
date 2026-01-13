/*
 * cubemap_filtering.h
 * Cubemap filtering
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_CUBEMAP_FILTERING_H
#define TEXTURE_CUBEMAP_FILTERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_cubemap_filtering_handle {
    uint32_t id;
} texture_cubemap_filtering_handle_t;

typedef struct texture_cubemap_filtering_desc {
    uint32_t flags;
    void* user_data;
} texture_cubemap_filtering_desc_t;

typedef struct texture_cubemap_filtering_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_cubemap_filtering_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_cubemap_filtering_init(void);
void texture_cubemap_filtering_shutdown(void);

/* Lifecycle */
int texture_cubemap_filtering_create(texture_cubemap_filtering_handle_t* out_handle, const texture_cubemap_filtering_desc_t* desc);
void texture_cubemap_filtering_destroy(texture_cubemap_filtering_handle_t handle);

/* Operations */
int texture_cubemap_filtering_update(texture_cubemap_filtering_handle_t handle, const void* data, size_t size);
bool texture_cubemap_filtering_is_valid(texture_cubemap_filtering_handle_t handle);
int texture_cubemap_filtering_get_info(texture_cubemap_filtering_handle_t handle, texture_cubemap_filtering_info_t* out_info);
void texture_cubemap_filtering_mark_dirty(texture_cubemap_filtering_handle_t handle);
int texture_cubemap_filtering_process_pending(void);

/* Statistics */
uint32_t texture_cubemap_filtering_get_count(void);
size_t texture_cubemap_filtering_get_memory_usage(void);
void texture_cubemap_filtering_debug_print(void);

/* Serialization */
int texture_cubemap_filtering_serialize(texture_cubemap_filtering_handle_t handle, void* buffer, size_t buffer_size, size_t* out_size);
int texture_cubemap_filtering_deserialize(const void* buffer, size_t buffer_size, texture_cubemap_filtering_handle_t* out_handle);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_CUBEMAP_FILTERING_H */
