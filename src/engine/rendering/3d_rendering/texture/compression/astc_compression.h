/*
 * astc_compression.h
 * ASTC compression
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ASTC_COMPRESSION_H
#define TEXTURE_ASTC_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_astc_compression_handle {
    uint32_t id;
} texture_astc_compression_handle_t;

typedef struct texture_astc_compression_desc {
    uint32_t flags;
    void* user_data;
} texture_astc_compression_desc_t;

typedef struct texture_astc_compression_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_astc_compression_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_astc_compression_init(void);
void texture_astc_compression_shutdown(void);

/* Lifecycle */
int texture_astc_compression_create(texture_astc_compression_handle_t* out_handle, const texture_astc_compression_desc_t* desc);
void texture_astc_compression_destroy(texture_astc_compression_handle_t handle);

/* Operations */
int texture_astc_compression_update(texture_astc_compression_handle_t handle, const void* data, size_t size);
bool texture_astc_compression_is_valid(texture_astc_compression_handle_t handle);
int texture_astc_compression_get_info(texture_astc_compression_handle_t handle, texture_astc_compression_info_t* out_info);
void texture_astc_compression_mark_dirty(texture_astc_compression_handle_t handle);
int texture_astc_compression_process_pending(void);

/* Statistics */
uint32_t texture_astc_compression_get_count(void);
size_t texture_astc_compression_get_memory_usage(void);
void texture_astc_compression_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ASTC_COMPRESSION_H */
