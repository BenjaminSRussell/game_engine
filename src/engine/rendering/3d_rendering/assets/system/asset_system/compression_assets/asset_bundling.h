/*
 * asset_bundling.h
 * Asset bundling
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ASSET_SYSTEM_ASSET_BUNDLING_H
#define ASSET_SYSTEM_ASSET_BUNDLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_asset_bundling_handle {
    uint32_t id;
} asset_system_asset_bundling_handle_t;

typedef struct asset_system_asset_bundling_desc {
    uint32_t flags;
    void* user_data;
} asset_system_asset_bundling_desc_t;

typedef struct asset_system_asset_bundling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} asset_system_asset_bundling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int asset_system_asset_bundling_init(void);
void asset_system_asset_bundling_shutdown(void);

/* Lifecycle */
int asset_system_asset_bundling_create(asset_system_asset_bundling_handle_t* out_handle, const asset_system_asset_bundling_desc_t* desc);
void asset_system_asset_bundling_destroy(asset_system_asset_bundling_handle_t handle);

/* Operations */
int asset_system_asset_bundling_update(asset_system_asset_bundling_handle_t handle, const void* data, size_t size);
bool asset_system_asset_bundling_is_valid(asset_system_asset_bundling_handle_t handle);
int asset_system_asset_bundling_get_info(asset_system_asset_bundling_handle_t handle, asset_system_asset_bundling_info_t* out_info);
void asset_system_asset_bundling_mark_dirty(asset_system_asset_bundling_handle_t handle);
int asset_system_asset_bundling_process_pending(void);

/* Statistics */
uint32_t asset_system_asset_bundling_get_count(void);
size_t asset_system_asset_bundling_get_memory_usage(void);
void asset_system_asset_bundling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_SYSTEM_ASSET_BUNDLING_H */
