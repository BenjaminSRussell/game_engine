/*
 * texture_format.h
 * Texture file format
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ASSET_SYSTEM_TEXTURE_FORMAT_H
#define ASSET_SYSTEM_TEXTURE_FORMAT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_texture_format_handle {
    uint32_t id;
} asset_system_texture_format_handle_t;

typedef struct asset_system_texture_format_desc {
    uint32_t flags;
    void* user_data;
} asset_system_texture_format_desc_t;

typedef struct asset_system_texture_format_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} asset_system_texture_format_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int asset_system_texture_format_init(void);
void asset_system_texture_format_shutdown(void);

/* Lifecycle */
int asset_system_texture_format_create(asset_system_texture_format_handle_t* out_handle, const asset_system_texture_format_desc_t* desc);
void asset_system_texture_format_destroy(asset_system_texture_format_handle_t handle);

/* Operations */
int asset_system_texture_format_update(asset_system_texture_format_handle_t handle, const void* data, size_t size);
bool asset_system_texture_format_is_valid(asset_system_texture_format_handle_t handle);
int asset_system_texture_format_get_info(asset_system_texture_format_handle_t handle, asset_system_texture_format_info_t* out_info);
void asset_system_texture_format_mark_dirty(asset_system_texture_format_handle_t handle);
int asset_system_texture_format_process_pending(void);

/* Statistics */
uint32_t asset_system_texture_format_get_count(void);
size_t asset_system_texture_format_get_memory_usage(void);
void asset_system_texture_format_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_SYSTEM_TEXTURE_FORMAT_H */
