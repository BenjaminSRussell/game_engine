/*
 * semaphore_manager.h
 * Timeline semaphore management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_SEMAPHORE_MANAGER_H
#define CORE_SEMAPHORE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_semaphore_manager_handle {
    uint32_t id;
} core_semaphore_manager_handle_t;

typedef struct core_semaphore_manager_desc {
    uint32_t flags;
    void* user_data;
} core_semaphore_manager_desc_t;

typedef struct core_semaphore_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_semaphore_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_semaphore_manager_init(void);
void core_semaphore_manager_shutdown(void);

/* Lifecycle */
int core_semaphore_manager_create(core_semaphore_manager_handle_t* out_handle, const core_semaphore_manager_desc_t* desc);
void core_semaphore_manager_destroy(core_semaphore_manager_handle_t handle);

/* Operations */
int core_semaphore_manager_update(core_semaphore_manager_handle_t handle, const void* data, size_t size);
bool core_semaphore_manager_is_valid(core_semaphore_manager_handle_t handle);
int core_semaphore_manager_get_info(core_semaphore_manager_handle_t handle, core_semaphore_manager_info_t* out_info);
void core_semaphore_manager_mark_dirty(core_semaphore_manager_handle_t handle);
int core_semaphore_manager_process_pending(void);

/* Statistics */
uint32_t core_semaphore_manager_get_count(void);
size_t core_semaphore_manager_get_memory_usage(void);
void core_semaphore_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SEMAPHORE_MANAGER_H */
