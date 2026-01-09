/*
 * heap_manager.h
 * Memory heap management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_HEAP_MANAGER_H
#define CORE_HEAP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_heap_manager_handle {
    uint32_t id;
} core_heap_manager_handle_t;

typedef struct core_heap_manager_desc {
    uint32_t flags;
    void* user_data;
} core_heap_manager_desc_t;

typedef struct core_heap_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_heap_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_heap_manager_init(void);
void core_heap_manager_shutdown(void);

/* Lifecycle */
int core_heap_manager_create(core_heap_manager_handle_t* out_handle, const core_heap_manager_desc_t* desc);
void core_heap_manager_destroy(core_heap_manager_handle_t handle);

/* Operations */
int core_heap_manager_add_heap(core_heap_manager_handle_t handle, uint64_t size, uint32_t flags);
int core_heap_manager_update(core_heap_manager_handle_t handle, const void* data, size_t size);
bool core_heap_manager_is_valid(core_heap_manager_handle_t handle);
int core_heap_manager_get_info(core_heap_manager_handle_t handle, core_heap_manager_info_t* out_info);
void core_heap_manager_mark_dirty(core_heap_manager_handle_t handle);
int core_heap_manager_process_pending(void);

/* Statistics */
uint32_t core_heap_manager_get_count(void);
size_t core_heap_manager_get_memory_usage(void);
void core_heap_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_HEAP_MANAGER_H */
