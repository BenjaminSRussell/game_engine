/*
 * memory_pool.h
 * Pooled memory allocation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_MEMORY_POOL_H
#define CORE_MEMORY_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_memory_pool_handle {
    uint32_t id;
} core_memory_pool_handle_t;

typedef struct core_memory_pool_desc {
    uint32_t flags;
    void* user_data;
} core_memory_pool_desc_t;

typedef struct core_memory_pool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_memory_pool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_memory_pool_init(void);
void core_memory_pool_shutdown(void);

/* Lifecycle */
int core_memory_pool_create(core_memory_pool_handle_t* out_handle, const core_memory_pool_desc_t* desc);
void core_memory_pool_destroy(core_memory_pool_handle_t handle);

/* Operations */
void* core_memory_pool_alloc(core_memory_pool_handle_t handle);
void core_memory_pool_free(core_memory_pool_handle_t handle, void* ptr);

int core_memory_pool_update(core_memory_pool_handle_t handle, const void* data, size_t size);
bool core_memory_pool_is_valid(core_memory_pool_handle_t handle);
int core_memory_pool_get_info(core_memory_pool_handle_t handle, core_memory_pool_info_t* out_info);
void core_memory_pool_mark_dirty(core_memory_pool_handle_t handle);
int core_memory_pool_process_pending(void);

/* Statistics */
uint32_t core_memory_pool_get_count(void);
size_t core_memory_pool_get_memory_usage(void);
void core_memory_pool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MEMORY_POOL_H */
