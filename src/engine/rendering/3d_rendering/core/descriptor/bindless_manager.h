/*
 * bindless_manager.h
 * Bindless resource indexing
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_BINDLESS_MANAGER_H
#define CORE_BINDLESS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_bindless_manager_handle {
    uint32_t id;
} core_bindless_manager_handle_t;

typedef struct core_bindless_manager_desc {
    uint32_t flags;
    void* user_data;
} core_bindless_manager_desc_t;

typedef struct core_bindless_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_bindless_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_bindless_manager_init(void);
void core_bindless_manager_shutdown(void);

/* Lifecycle */
int core_bindless_manager_create(core_bindless_manager_handle_t* out_handle, const core_bindless_manager_desc_t* desc);
void core_bindless_manager_destroy(core_bindless_manager_handle_t handle);

/* Operations */
int core_bindless_manager_update(core_bindless_manager_handle_t handle, const void* data, size_t size);
bool core_bindless_manager_is_valid(core_bindless_manager_handle_t handle);
int core_bindless_manager_get_info(core_bindless_manager_handle_t handle, core_bindless_manager_info_t* out_info);
void core_bindless_manager_mark_dirty(core_bindless_manager_handle_t handle);
int core_bindless_manager_process_pending(void);

/* Statistics */
uint32_t core_bindless_manager_get_count(void);
size_t core_bindless_manager_get_memory_usage(void);
void core_bindless_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BINDLESS_MANAGER_H */
