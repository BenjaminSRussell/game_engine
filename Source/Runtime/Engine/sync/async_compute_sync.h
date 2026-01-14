/*
 * async_compute_sync.h
 * Async compute synchronization
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_ASYNC_COMPUTE_SYNC_H
#define CORE_ASYNC_COMPUTE_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_async_compute_sync_handle {
    uint32_t id;
} core_async_compute_sync_handle_t;

typedef struct core_async_compute_sync_desc {
    uint32_t flags;
    void* user_data;
} core_async_compute_sync_desc_t;

typedef struct core_async_compute_sync_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_async_compute_sync_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_async_compute_sync_init(void);
void core_async_compute_sync_shutdown(void);

/* Lifecycle */
int core_async_compute_sync_create(core_async_compute_sync_handle_t* out_handle, const core_async_compute_sync_desc_t* desc);
void core_async_compute_sync_destroy(core_async_compute_sync_handle_t handle);

/* Operations */
int core_async_compute_sync_update(core_async_compute_sync_handle_t handle, const void* data, size_t size);
bool core_async_compute_sync_is_valid(core_async_compute_sync_handle_t handle);
int core_async_compute_sync_get_info(core_async_compute_sync_handle_t handle, core_async_compute_sync_info_t* out_info);
void core_async_compute_sync_mark_dirty(core_async_compute_sync_handle_t handle);
int core_async_compute_sync_process_pending(void);

/* Statistics */
uint32_t core_async_compute_sync_get_count(void);
size_t core_async_compute_sync_get_memory_usage(void);
void core_async_compute_sync_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_ASYNC_COMPUTE_SYNC_H */
