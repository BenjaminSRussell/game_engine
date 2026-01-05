/*
 * format_conversion.h
 * Format conversion
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FORMAT_CONVERSION_H
#define TEXTURE_FORMAT_CONVERSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_format_conversion_handle {
    uint32_t id;
} texture_format_conversion_handle_t;

typedef struct texture_format_conversion_desc {
    uint32_t flags;
    void* user_data;
} texture_format_conversion_desc_t;

typedef struct texture_format_conversion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_format_conversion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_format_conversion_init(void);
void texture_format_conversion_shutdown(void);

/* Lifecycle */
int texture_format_conversion_create(texture_format_conversion_handle_t* out_handle, const texture_format_conversion_desc_t* desc);
void texture_format_conversion_destroy(texture_format_conversion_handle_t handle);

/* Operations */
int texture_format_conversion_update(texture_format_conversion_handle_t handle, const void* data, size_t size);
bool texture_format_conversion_is_valid(texture_format_conversion_handle_t handle);
int texture_format_conversion_get_info(texture_format_conversion_handle_t handle, texture_format_conversion_info_t* out_info);
void texture_format_conversion_mark_dirty(texture_format_conversion_handle_t handle);
int texture_format_conversion_process_pending(void);

/* Statistics */
uint32_t texture_format_conversion_get_count(void);
size_t texture_format_conversion_get_memory_usage(void);
void texture_format_conversion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FORMAT_CONVERSION_H */
