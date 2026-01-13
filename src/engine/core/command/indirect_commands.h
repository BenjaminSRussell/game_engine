/*
 * indirect_commands.h
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_INDIRECT_COMMANDS_H
#define CORE_INDIRECT_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum CoreIndirectCommandsResult {
    CORE_INDIRECT_COMMANDS_SUCCESS = 0,
    CORE_INDIRECT_COMMANDS_ERROR_UNKNOWN = -1,
    CORE_INDIRECT_COMMANDS_ERROR_INVALID_ARGS = -2,
    CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED = -3,
    CORE_INDIRECT_COMMANDS_ERROR_OUT_OF_MEMORY = -4,
    CORE_INDIRECT_COMMANDS_ERROR_BUFFER_TOO_SMALL = -5,
    CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE = -6,
    CORE_INDIRECT_COMMANDS_ERROR_ALREADY_INITIALIZED = -7,
    CORE_INDIRECT_COMMANDS_ERROR_LIMIT_REACHED = -8,
    CORE_INDIRECT_COMMANDS_ERROR_LOCK_FAILED = -9
} CoreIndirectCommandsResult;

typedef struct core_indirect_commands_handle {
    uint32_t id;
} core_indirect_commands_handle_t;

typedef struct core_indirect_commands_desc {
    uint32_t flags;
    void* user_data;
} core_indirect_commands_desc_t;

typedef struct core_indirect_commands_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_indirect_commands_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
CoreIndirectCommandsResult core_indirect_commands_init(void);
void core_indirect_commands_shutdown(void);

/* Lifecycle */
CoreIndirectCommandsResult core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc);
void core_indirect_commands_destroy(core_indirect_commands_handle_t handle);

/* Operations */
CoreIndirectCommandsResult core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size);
bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle);
CoreIndirectCommandsResult core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info);
void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle);
int core_indirect_commands_process_pending(void);

/* Statistics */
uint32_t core_indirect_commands_get_count(void);
size_t core_indirect_commands_get_memory_usage(void);
void core_indirect_commands_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INDIRECT_COMMANDS_H */
