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
 * CONSTANTS
 * ============================================================================ */

typedef enum core_indirect_commands_error {
    CORE_INDIRECT_COMMANDS_SUCCESS = 0,
    CORE_INDIRECT_COMMANDS_ERROR_INVALID_ARGS = -1,
    CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED = -2,
    CORE_INDIRECT_COMMANDS_ERROR_OUT_OF_MEMORY = -3,
    CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE = -4,
    CORE_INDIRECT_COMMANDS_ERROR_BACKEND_FAILURE = -5,
    CORE_INDIRECT_COMMANDS_ERROR_CAPACITY_REACHED = -6
} core_indirect_commands_error_t;

typedef enum core_indirect_commands_flags {
    CORE_INDIRECT_COMMANDS_FLAG_NONE = 0,
    CORE_INDIRECT_COMMANDS_FLAG_STREAMING = 1 << 0,
    CORE_INDIRECT_COMMANDS_FLAG_LOD = 1 << 1,
    CORE_INDIRECT_COMMANDS_FLAG_CULLING = 1 << 2,
    CORE_INDIRECT_COMMANDS_FLAG_DYNAMIC = 1 << 3,
    CORE_INDIRECT_COMMANDS_FLAG_GPU_WRITABLE = 1 << 4
} core_indirect_commands_flags_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_indirect_commands_handle {
    uint32_t id;
} core_indirect_commands_handle_t;

typedef struct core_indirect_commands_desc {
    uint32_t flags;
    void* user_data;
    uint32_t initial_capacity;
} core_indirect_commands_desc_t;

typedef struct core_indirect_commands_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t lod_level;
    bool visible;
    size_t memory_usage;
} core_indirect_commands_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_indirect_commands_init(void);
void core_indirect_commands_shutdown(void);

/* Lifecycle */
int core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc);
void core_indirect_commands_destroy(core_indirect_commands_handle_t handle);

/* Operations */
int core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size);
bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle);
int core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info);
void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle);
int core_indirect_commands_process_pending(void);

/* Feature specific */
int core_indirect_commands_set_lod(core_indirect_commands_handle_t handle, uint32_t lod_level);
int core_indirect_commands_set_culling_state(core_indirect_commands_handle_t handle, bool visible);

/* Statistics */
uint32_t core_indirect_commands_get_count(void);
size_t core_indirect_commands_get_memory_usage(void);
void core_indirect_commands_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INDIRECT_COMMANDS_H */
