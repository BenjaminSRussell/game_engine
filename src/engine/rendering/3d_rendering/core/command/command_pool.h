/*
 * command_pool.h
 * Command pool allocation and recycling
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_COMMAND_POOL_H
#define CORE_COMMAND_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_command_pool_handle {
    uint32_t id;
} core_command_pool_handle_t;

typedef struct core_command_pool_desc {
    uint32_t flags;
    void* user_data;
} core_command_pool_desc_t;

typedef struct core_command_pool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_command_pool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_command_pool_init(void);
void core_command_pool_shutdown(void);

/* Lifecycle */
int core_command_pool_create(core_command_pool_handle_t* out_handle, const core_command_pool_desc_t* desc);
void core_command_pool_destroy(core_command_pool_handle_t handle);

/* Operations */
int core_command_pool_update(core_command_pool_handle_t handle, const void* data, size_t size);
bool core_command_pool_is_valid(core_command_pool_handle_t handle);
int core_command_pool_get_info(core_command_pool_handle_t handle, core_command_pool_info_t* out_info);
void core_command_pool_mark_dirty(core_command_pool_handle_t handle);
int core_command_pool_process_pending(void);

/* Statistics */
uint32_t core_command_pool_get_count(void);
size_t core_command_pool_get_memory_usage(void);
void core_command_pool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_COMMAND_POOL_H */
