/*
 * asset_loader.h
 * Asset loading system
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ASSET_SYSTEM_ASSET_LOADER_H
#define ASSET_SYSTEM_ASSET_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_asset_loader_handle {
    uint32_t id;
} asset_system_asset_loader_handle_t;

typedef struct asset_system_asset_loader_desc {
    uint32_t flags;
    void* user_data;
} asset_system_asset_loader_desc_t;

typedef struct asset_system_asset_loader_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} asset_system_asset_loader_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int asset_system_asset_loader_init(void);
void asset_system_asset_loader_shutdown(void);

/* Lifecycle */
int asset_system_asset_loader_create(asset_system_asset_loader_handle_t* out_handle, const asset_system_asset_loader_desc_t* desc);
void asset_system_asset_loader_destroy(asset_system_asset_loader_handle_t handle);

/* Operations */
int asset_system_asset_loader_update(asset_system_asset_loader_handle_t handle, const void* data, size_t size);
bool asset_system_asset_loader_is_valid(asset_system_asset_loader_handle_t handle);
int asset_system_asset_loader_get_info(asset_system_asset_loader_handle_t handle, asset_system_asset_loader_info_t* out_info);
void asset_system_asset_loader_mark_dirty(asset_system_asset_loader_handle_t handle);
int asset_system_asset_loader_process_pending(void);

/* Statistics */
uint32_t asset_system_asset_loader_get_count(void);
size_t asset_system_asset_loader_get_memory_usage(void);
void asset_system_asset_loader_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_SYSTEM_ASSET_LOADER_H */
