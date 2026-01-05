/*
 * fence_pool.h
 * Fence allocation and tracking
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_FENCE_POOL_H
#define CORE_FENCE_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_fence_pool_handle {
    uint32_t id;
} core_fence_pool_handle_t;

typedef struct core_fence_pool_desc {
    uint32_t flags;
    void* user_data;
} core_fence_pool_desc_t;

typedef struct core_fence_pool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_fence_pool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_fence_pool_init(void);
void core_fence_pool_shutdown(void);

/* Lifecycle */
int core_fence_pool_create(core_fence_pool_handle_t* out_handle, const core_fence_pool_desc_t* desc);
void core_fence_pool_destroy(core_fence_pool_handle_t handle);

/* Operations */
int core_fence_pool_update(core_fence_pool_handle_t handle, const void* data, size_t size);
bool core_fence_pool_is_valid(core_fence_pool_handle_t handle);
int core_fence_pool_get_info(core_fence_pool_handle_t handle, core_fence_pool_info_t* out_info);
void core_fence_pool_mark_dirty(core_fence_pool_handle_t handle);
int core_fence_pool_process_pending(void);

/* Statistics */
uint32_t core_fence_pool_get_count(void);
size_t core_fence_pool_get_memory_usage(void);
void core_fence_pool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_FENCE_POOL_H */
