/*
 * compression_quality.h
 * Compression quality settings
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_COMPRESSION_QUALITY_H
#define TEXTURE_COMPRESSION_QUALITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_compression_quality_handle {
    uint32_t id;
} texture_compression_quality_handle_t;

typedef struct texture_compression_quality_desc {
    uint32_t flags;
    void* user_data;
} texture_compression_quality_desc_t;

typedef struct texture_compression_quality_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_compression_quality_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_compression_quality_init(void);
void texture_compression_quality_shutdown(void);

/* Lifecycle */
int texture_compression_quality_create(texture_compression_quality_handle_t* out_handle, const texture_compression_quality_desc_t* desc);
void texture_compression_quality_destroy(texture_compression_quality_handle_t handle);

/* Operations */
int texture_compression_quality_update(texture_compression_quality_handle_t handle, const void* data, size_t size);
bool texture_compression_quality_is_valid(texture_compression_quality_handle_t handle);
int texture_compression_quality_get_info(texture_compression_quality_handle_t handle, texture_compression_quality_info_t* out_info);
void texture_compression_quality_mark_dirty(texture_compression_quality_handle_t handle);
int texture_compression_quality_process_pending(void);

/* Statistics */
uint32_t texture_compression_quality_get_count(void);
size_t texture_compression_quality_get_memory_usage(void);
void texture_compression_quality_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_COMPRESSION_QUALITY_H */
