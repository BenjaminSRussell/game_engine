/*
 * async_loading.h
 * Async asset loading
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ASSET_SYSTEM_ASYNC_LOADING_H
#define ASSET_SYSTEM_ASYNC_LOADING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_async_loading_handle {
    uint32_t id;
} asset_system_async_loading_handle_t;

typedef struct asset_system_async_loading_desc {
    uint32_t flags;
    void* user_data;
} asset_system_async_loading_desc_t;

typedef struct asset_system_async_loading_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} asset_system_async_loading_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int asset_system_async_loading_init(void);
void asset_system_async_loading_shutdown(void);

/* Lifecycle */
int asset_system_async_loading_create(asset_system_async_loading_handle_t* out_handle, const asset_system_async_loading_desc_t* desc);
void asset_system_async_loading_destroy(asset_system_async_loading_handle_t handle);

/* Operations */
int asset_system_async_loading_update(asset_system_async_loading_handle_t handle, const void* data, size_t size);
bool asset_system_async_loading_is_valid(asset_system_async_loading_handle_t handle);
int asset_system_async_loading_get_info(asset_system_async_loading_handle_t handle, asset_system_async_loading_info_t* out_info);
void asset_system_async_loading_mark_dirty(asset_system_async_loading_handle_t handle);
int asset_system_async_loading_process_pending(void);

/* Statistics */
uint32_t asset_system_async_loading_get_count(void);
size_t asset_system_async_loading_get_memory_usage(void);
void asset_system_async_loading_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_SYSTEM_ASYNC_LOADING_H */
